package main

import (
	"errors"
	"fmt"
	"runtime"
	"time"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

const runtimeParkTimeout = 100 * time.Millisecond

// runtimeLoopHandles is published once after the runtime loop creates the map.
// The render loop uses the map only to attach its own render session and uses
// the wake source to release a parked pump after queuing work.
type runtimeLoopHandles struct {
	mapRef *maplibre.MapHandle
	wake   *maplibre.WakeSource
}

// runRuntimeLoop owns the runtime, map, pump, event queue, and every camera
// mutation on one stable native thread for their whole lifetime.
func runRuntimeLoop(v viewport, commands *commandQueue, published chan<- runtimeLoopHandles, shared *sharedState) {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()
	defer close(published)

	state, err := newRuntimeMapState(v)
	if err != nil {
		shared.fail(err)
		return
	}
	defer func() { shared.fail(state.Close()) }()

	wake, err := state.runtime.WakeSource()
	if err != nil {
		shared.fail(fmt.Errorf("wake source acquire failed: %w", err))
		return
	}
	defer wake.Close()
	published <- runtimeLoopHandles{mapRef: state.mapRef, wake: wake}

	for !shared.shutdownRequested() {
		if err := state.applyCommands(commands); err != nil {
			shared.fail(err)
			break
		}
		if err := state.runtime.Pump(runtimeParkTimeout); err != nil {
			shared.fail(fmt.Errorf("runtime pump failed: %w", err))
			break
		}
		renderRequested, err := drainEvents(state.runtime, state.mapID)
		if err != nil {
			shared.fail(err)
			break
		}
		if renderRequested {
			shared.requestRender()
		}
	}
	// Destroying a map with an attached session is invalid, so keep the map
	// alive until the render loop signals shutdown.
	for !shared.shutdownRequested() {
		time.Sleep(time.Millisecond)
	}
}

type runtimeMapState struct {
	runtime *maplibre.RuntimeHandle
	mapRef  *maplibre.MapHandle
	mapID   maplibre.MapID
	batch   []cameraCommand
}

func newRuntimeMapState(v viewport) (*runtimeMapState, error) {
	runtimeHandle, err := maplibre.NewRuntimeWithOptions(maplibre.NewRuntimeOptions("", ":memory:"))
	if err != nil {
		return nil, fmt.Errorf("runtime create failed: %w", err)
	}
	state := &runtimeMapState{runtime: runtimeHandle}

	mapOptions := maplibre.NewMapOptions(v.logicalWidth, v.logicalHeight, v.scaleFactor)
	// The two event types the runtime loop reads. A map queues no event of an
	// unselected type, so the first style load already queues nothing else.
	mapOptions.EventMask = maplibre.RuntimeEventMaskMapRenderUpdateAvailable |
		maplibre.RuntimeEventMaskMapRenderFrameFinished
	mapHandle, err := runtimeHandle.NewMapWithOptions(mapOptions)
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map create failed: %w", err)
	}
	state.mapRef = mapHandle
	mapID, err := mapHandle.ID()
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map identity read failed: %w", err)
	}
	state.mapID = mapID

	if err := mapHandle.SetStyleURL("https://tiles.openfreemap.org/styles/bright"); err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("style load failed: %w", err)
	}
	initialCamera := maplibre.CameraOptions{}.
		WithCenter(maplibre.LatLng{Latitude: 37.7749, Longitude: -122.4194}).
		WithZoom(13).
		WithBearing(12).
		WithPitch(30)
	if err := mapHandle.JumpTo(initialCamera); err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("camera jump failed: %w", err)
	}
	if err := mapHandle.RequestRepaint(); err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("initial repaint request failed: %w", err)
	}
	return state, nil
}

func (state *runtimeMapState) Close() error {
	var result error
	if state.mapRef != nil {
		result = errors.Join(result, state.mapRef.Close())
		state.mapRef = nil
	}
	if state.runtime != nil {
		result = errors.Join(result, state.runtime.Close())
		state.runtime = nil
	}
	return result
}

func (state *runtimeMapState) applyCommands(commands *commandQueue) error {
	state.batch = commands.drain(state.batch)
	for _, command := range state.batch {
		if err := state.applyCommand(command); err != nil {
			return err
		}
	}
	return nil
}

func (state *runtimeMapState) applyCommand(command cameraCommand) error {
	m := state.mapRef
	animation := maplibre.AnimationOptions{}.WithDurationMS(command.durationMS)
	var err error
	switch command.kind {
	case commandCancelTransitions:
		err = m.CancelTransitions()
	case commandSetGestureInProgress:
		err = m.SetGestureInProgress(command.inProgress)
	case commandMoveBy:
		err = m.MoveBy(maplibre.ScreenPoint{X: command.deltaX, Y: command.deltaY})
	case commandMoveByAnimated:
		err = m.MoveByAnimated(maplibre.ScreenPoint{X: command.deltaX, Y: command.deltaY}, &animation)
	case commandScaleBy:
		err = m.ScaleBy(command.scale, &command.anchor)
	case commandScaleByAnimated:
		err = m.ScaleByAnimated(command.scale, &command.anchor, &animation)
	case commandPitchBy:
		err = m.PitchBy(command.deltaY)
	case commandAdjustBearing:
		err = state.adjustBearing(command.deltaX, nil)
	case commandAdjustBearingAnimated:
		err = state.adjustBearing(command.deltaX, &animation)
	case commandAdjustPitchAnimated:
		err = state.adjustPitch(command.deltaY, &animation)
	case commandResetOrientation:
		err = m.EaseTo(maplibre.CameraOptions{}.WithBearing(0).WithPitch(0), &animation)
	default:
		err = fmt.Errorf("unknown camera command: %d", command.kind)
	}
	if err != nil {
		return fmt.Errorf("camera command failed: %w", err)
	}
	return nil
}

func (state *runtimeMapState) adjustBearing(delta float64, animation *maplibre.AnimationOptions) error {
	camera, err := state.mapRef.Camera()
	if err != nil {
		return err
	}
	bearing := delta
	if camera.Bearing != nil {
		bearing += *camera.Bearing
	}
	options := maplibre.CameraOptions{}.WithBearing(bearing)
	if animation == nil {
		return state.mapRef.JumpTo(options)
	}
	return state.mapRef.EaseTo(options, animation)
}

func (state *runtimeMapState) adjustPitch(delta float64, animation *maplibre.AnimationOptions) error {
	camera, err := state.mapRef.Camera()
	if err != nil {
		return err
	}
	pitch := delta
	if camera.Pitch != nil {
		pitch += *camera.Pitch
	}
	return state.mapRef.EaseTo(maplibre.CameraOptions{}.WithPitch(clamp(pitch, 0, 60)), animation)
}

func drainEvents(runtimeHandle *maplibre.RuntimeHandle, mapID maplibre.MapID) (bool, error) {
	renderRequested := false
	// One drain takes every event the pump produced.
	batch, err := runtimeHandle.DrainEvents(0)
	if err != nil {
		return renderRequested, fmt.Errorf("runtime event drain failed: %w", err)
	}
	for _, event := range batch.Events {
		if event.Source.Type != maplibre.RuntimeEventSourceMap || event.Source.MapID != mapID {
			continue
		}
		switch event.Type {
		case maplibre.RuntimeEventMapRenderUpdateAvailable:
			renderRequested = true
		case maplibre.RuntimeEventMapRenderFrameFinished:
			payload, ok := event.Payload.(maplibre.RuntimeEventRenderFramePayload)
			if ok && payload.NeedsRepaint {
				renderRequested = true
			}
		}
	}
	return renderRequested, nil
}

// renderMapState owns the render target on the SDL render loop thread.
type renderMapState struct {
	target renderTarget
}

func newRenderMapState(graphics *openGLContext, mapRef *maplibre.MapHandle, v viewport, mode renderTargetMode) (*renderMapState, error) {
	target, err := newOpenGLRenderTarget(graphics, v, mode, mapRef)
	if err != nil {
		return nil, err
	}
	return &renderMapState{target: target}, nil
}

func (state *renderMapState) closeTarget() error {
	if state.target == nil {
		return nil
	}
	err := state.target.Close()
	state.target = nil
	return err
}

func (state *renderMapState) resize(v viewport) error {
	if state.target == nil {
		return errors.New("render target is not attached")
	}
	return state.target.Resize(v)
}

func (state *renderMapState) finishFrame() error {
	if state.target == nil {
		return nil
	}
	return state.target.FinishFrame()
}

func (state *renderMapState) renderUpdate() (bool, error) {
	if state.target == nil {
		return false, nil
	}
	return state.target.RenderUpdate()
}
