package main

import (
	"errors"
	"fmt"
	"math"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

type runtimeMapState struct {
	runtime *maplibre.RuntimeHandle
	mapRef  *maplibre.MapHandle
	mapID   maplibre.MapID
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
	_, err = mapHandle.SetStyleURL("https://tiles.openfreemap.org/styles/bright")
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("style load failed: %w", err)
	}
	initialCamera := maplibre.CameraOptions{}.
		WithCenter(maplibre.LatLng{Latitude: 37.7749, Longitude: -122.4194}).
		WithZoom(13).
		WithBearing(12).
		WithPitch(30)
	_, err = mapHandle.JumpTo(initialCamera)
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("camera jump failed: %w", err)
	}
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
	_, err = mapHandle.RequestRepaint()
	if err != nil {
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

func (state *runtimeMapState) cancelTransitions() error {
	return state.updateCamera(maplibre.CameraUpdate{GesturePhase: maplibre.GesturePhaseCancel})
}

func (state *runtimeMapState) setGestureInProgress(inProgress bool) error {
	phase := maplibre.GesturePhaseEnd
	if inProgress {
		phase = maplibre.GesturePhaseBegin
	}
	return state.updateCamera(maplibre.CameraUpdate{GesturePhase: phase})
}

func (state *runtimeMapState) moveBy(dx, dy float64, durationMS *float64) error {
	camera, err := state.orderedCamera()
	if err != nil {
		return fmt.Errorf("ordered camera read failed: %w", err)
	}
	center := maplibre.LatLng{}
	if camera.Center != nil {
		center = *camera.Center
	}
	zoom := 0.0
	if camera.Zoom != nil {
		zoom = *camera.Zoom
	}
	degreesPerPixel := 360 / (256 * math.Exp2(zoom))
	center.Longitude -= dx * degreesPerPixel
	center.Latitude += dy * degreesPerPixel
	return state.updateCamera(cameraUpdate(maplibre.CameraOptions{}.WithCenter(center), durationMS))
}

func (state *runtimeMapState) scaleBy(scale float64, anchor maplibre.ScreenPoint, durationMS *float64) error {
	camera, err := state.orderedCamera()
	if err != nil {
		return fmt.Errorf("ordered camera read failed: %w", err)
	}
	zoom := 0.0
	if camera.Zoom != nil {
		zoom = *camera.Zoom
	}
	options := maplibre.CameraOptions{}.WithZoom(zoom + math.Log2(scale))
	options.Anchor = &anchor
	return state.updateCamera(cameraUpdate(options, durationMS))
}

func (state *runtimeMapState) adjustPitch(delta float64, durationMS *float64) error {
	camera, err := state.orderedCamera()
	if err != nil {
		return fmt.Errorf("ordered camera read failed: %w", err)
	}
	pitch := delta
	if camera.Pitch != nil {
		pitch += *camera.Pitch
	}
	return state.updateCamera(cameraUpdate(maplibre.CameraOptions{}.WithPitch(clamp(pitch, 0, 60)), durationMS))
}

func (state *runtimeMapState) adjustBearing(delta float64, durationMS *float64) error {
	camera, err := state.orderedCamera()
	if err != nil {
		return fmt.Errorf("ordered camera read failed: %w", err)
	}
	bearing := delta
	if camera.Bearing != nil {
		bearing += *camera.Bearing
	}
	return state.updateCamera(cameraUpdate(maplibre.CameraOptions{}.WithBearing(bearing), durationMS))
}

func (state *runtimeMapState) resetOrientation(durationMS float64) error {
	return state.updateCamera(cameraUpdate(maplibre.CameraOptions{}.WithBearing(0).WithPitch(0), &durationMS))
}

func (state *runtimeMapState) updateCamera(update maplibre.CameraUpdate) error {
	if _, err := state.mapRef.UpdateCamera(update); err != nil {
		return fmt.Errorf("camera update failed: %w", err)
	}
	return nil
}

func cameraUpdate(options maplibre.CameraOptions, durationMS *float64) maplibre.CameraUpdate {
	update := maplibre.CameraUpdate{Camera: options}
	if durationMS != nil {
		animation := maplibre.AnimationOptions{}.WithDurationMS(*durationMS)
		update.Mode = maplibre.CameraUpdateModeEase
		update.Animation = &animation
	}
	return update
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
	batch, err := runtimeHandle.DrainEvents()
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
