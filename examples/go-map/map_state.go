package main

import (
	"errors"
	"fmt"
	"math"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

type runtimeMapState struct {
	runtime   *maplibre.RuntimeHandle
	mapRef    *maplibre.MapHandle
	mapID     maplibre.MapID
	commandID uint64
}

func newRuntimeMapState(v viewport) (*runtimeMapState, error) {
	runtimeHandle, err := maplibre.NewRuntimeWithOptions(maplibre.NewRuntimeOptions("", ":memory:"))
	if err != nil {
		return nil, fmt.Errorf("runtime create failed: %w", err)
	}
	state := &runtimeMapState{runtime: runtimeHandle}
	mapOptions := maplibre.NewMapOptions(v.logicalWidth, v.logicalHeight, v.scaleFactor)
	mapOptions.EventMask = maplibre.RuntimeEventMaskMapRenderUpdateAvailable |
		maplibre.RuntimeEventMaskMapRenderFrameFinished
	mapHandle, err := runtimeHandle.NewMapWithOptions(mapOptions)
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map create failed: %w", err)
	}
	state.mapRef = mapHandle
	state.mapID, err = mapHandle.ID()
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map identity read failed: %w", err)
	}
	commandID, err := mapHandle.SetStyleURL("https://tiles.openfreemap.org/styles/bright")
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("style load failed: %w", err)
	}
	state.commandID = commandID
	initialCamera := maplibre.CameraOptions{}.
		WithCenter(maplibre.LatLng{Latitude: 37.7749, Longitude: -122.4194}).
		WithZoom(13).
		WithBearing(12).
		WithPitch(30)
	commandID, err = mapHandle.JumpTo(initialCamera)
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("camera jump failed: %w", err)
	}
	state.commandID = commandID
	barrier, err := runtimeHandle.Barrier()
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("initial barrier failed: %w", err)
	}
	if completed, err := barrier.Wait(-1); err != nil || !completed {
		barrier.Release()
		_ = state.Close()
		return nil, fmt.Errorf("initial barrier wait failed: completed=%v: %w", completed, err)
	}
	if err := barrier.Discard(); err != nil {
		barrier.Release()
		_ = state.Close()
		return nil, fmt.Errorf("initial barrier result failed: %w", err)
	}
	barrier.Release()
	commandID, err = mapHandle.RequestRepaint()
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("initial repaint request failed: %w", err)
	}
	state.commandID = commandID
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

func (state *runtimeMapState) applyCommand(command cameraCommand) error {
	camera, err := state.orderedCamera()
	if err != nil {
		return fmt.Errorf("ordered camera read failed: %w", err)
	}
	animation := maplibre.AnimationOptions{}.WithDurationMS(command.durationMS)
	options := maplibre.CameraOptions{}
	mode := maplibre.CameraUpdateModeJump
	gesture := maplibre.GesturePhaseNone

	switch command.kind {
	case commandCancelTransitions:
		gesture = maplibre.GesturePhaseCancel
	case commandSetGestureInProgress:
		if command.inProgress {
			gesture = maplibre.GesturePhaseBegin
		} else {
			gesture = maplibre.GesturePhaseEnd
		}
	case commandMoveBy, commandMoveByAnimated:
		center := maplibre.LatLng{}
		if camera.Center != nil {
			center = *camera.Center
		}
		zoom := 0.0
		if camera.Zoom != nil {
			zoom = *camera.Zoom
		}
		degreesPerPixel := 360 / (256 * math.Exp2(zoom))
		center.Longitude -= command.deltaX * degreesPerPixel
		center.Latitude += command.deltaY * degreesPerPixel
		options = options.WithCenter(center)
		if command.kind == commandMoveByAnimated {
			mode = maplibre.CameraUpdateModeEase
		}
	case commandScaleBy, commandScaleByAnimated:
		zoom := 0.0
		if camera.Zoom != nil {
			zoom = *camera.Zoom
		}
		options = options.WithZoom(zoom + math.Log2(command.scale))
		options.Anchor = &command.anchor
		if command.kind == commandScaleByAnimated {
			mode = maplibre.CameraUpdateModeEase
		}
	case commandPitchBy, commandAdjustPitchAnimated:
		pitch := command.deltaY
		if camera.Pitch != nil {
			pitch += *camera.Pitch
		}
		options = options.WithPitch(clamp(pitch, 0, 60))
		if command.kind == commandAdjustPitchAnimated {
			mode = maplibre.CameraUpdateModeEase
		}
	case commandAdjustBearing, commandAdjustBearingAnimated:
		bearing := command.deltaX
		if camera.Bearing != nil {
			bearing += *camera.Bearing
		}
		options = options.WithBearing(bearing)
		if command.kind == commandAdjustBearingAnimated {
			mode = maplibre.CameraUpdateModeEase
		}
	case commandResetOrientation:
		options = options.WithBearing(0).WithPitch(0)
		mode = maplibre.CameraUpdateModeEase
	default:
		return fmt.Errorf("unknown camera command: %d", command.kind)
	}
	update := maplibre.CameraUpdate{Mode: mode, Camera: options, GesturePhase: gesture}
	if mode != maplibre.CameraUpdateModeJump {
		update.Animation = &animation
	}
	commandID, err := state.mapRef.UpdateCamera(update)
	if err != nil {
		return fmt.Errorf("camera command failed: %w", err)
	}
	state.commandID = commandID
	return nil
}

func (state *runtimeMapState) orderedCamera() (maplibre.CameraOptions, error) {
	operation, err := state.mapRef.QueryCamera()
	if err != nil {
		return maplibre.CameraOptions{}, err
	}
	defer operation.Release()
	if completed, err := operation.Wait(-1); err != nil {
		return maplibre.CameraOptions{}, err
	} else if !completed {
		return maplibre.CameraOptions{}, errors.New("camera query returned before completion")
	}
	result, err := operation.Take()
	return result.Camera, err
}

func drainEvents(runtimeHandle *maplibre.RuntimeHandle, mapID maplibre.MapID) (bool, error) {
	renderRequested := false
	batch, err := runtimeHandle.DrainEvents(0)
	if err != nil {
		return false, fmt.Errorf("runtime event drain failed: %w", err)
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

func (state *renderMapState) driveFrame() (bool, error) {
	if state.target == nil {
		return false, nil
	}
	return state.target.DriveFrame()
}
