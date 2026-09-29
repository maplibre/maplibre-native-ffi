package main

import (
	"context"
	"errors"
	"fmt"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

type runtimeMapState struct {
	runtime *maplibre.RuntimeHandle
	mapRef  *maplibre.MapHandle
	mapID   uint64
}

func newRuntimeMapState(v viewport) (*runtimeMapState, error) {
	runtimeOptions := maplibre.DefaultRuntimeOptions()
	cachePath := ":memory:"
	runtimeOptions.CachePath = &cachePath
	runtimeHandle, err := maplibre.RuntimeCreate(runtimeOptions)
	if err != nil {
		return nil, fmt.Errorf("runtime create failed: %w", err)
	}
	state := &runtimeMapState{runtime: runtimeHandle}
	mapOptions := maplibre.DefaultMapOptions()
	mapOptions.InitialExtent = maplibre.LogicalExtent{Width: v.logicalWidth, Height: v.logicalHeight, ScaleFactor: v.scaleFactor}
	// The render loop re-arms from the frame result's repaint flag, so the map
	// only has to report updates that arrive between frames.
	mapOptions.EventMask = maplibre.RuntimeEventMaskMapRenderUpdateAvailable
	mapFuture, err := runtimeHandle.MapCreate(mapOptions)
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map create failed: %w", err)
	}
	mapHandle, err := mapFuture.Await(context.Background())
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map create failed: %w", err)
	}
	state.mapRef = mapHandle
	state.mapID, err = mapHandle.Id()
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map identity read failed: %w", err)
	}
	_, err = mapHandle.SetStyleUrl("https://tiles.openfreemap.org/styles/bright")
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("style load failed: %w", err)
	}
	zoom, bearing, pitch := 13.0, 12.0, 30.0
	initialCamera := maplibre.CameraOptions{
		Center: &maplibre.LatLng{Latitude: 37.7749, Longitude: -122.4194},
		Zoom:   &zoom, Bearing: &bearing, Pitch: &pitch,
	}
	_, err = mapHandle.UpdateCamera(maplibre.CameraUpdate{Camera: initialCamera})
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("camera jump failed: %w", err)
	}
	_, err = mapHandle.RequestRepaint()
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("initial repaint request failed: %w", err)
	}
	return state, nil
}

func (state *runtimeMapState) Close() error {
	var result error
	// Awaiting both release completions keeps process exit ordered after
	// native teardown.
	if state.mapRef != nil {
		teardown, err := state.mapRef.Close()
		result = errors.Join(result, err)
		if err == nil {
			_, waitErr := teardown.Await(context.Background())
			result = errors.Join(result, waitErr)
		}
		state.mapRef = nil
	}
	if state.runtime != nil {
		teardown, err := state.runtime.Close()
		result = errors.Join(result, err)
		if err == nil {
			_, waitErr := teardown.Await(context.Background())
			result = errors.Join(result, waitErr)
		}
		state.runtime = nil
	}
	return result
}

// cancelTransitions ends any running camera transition, so a starting gesture
// takes over from it rather than fighting it.
func (state *runtimeMapState) cancelTransitions() error {
	if _, err := state.mapRef.CancelTransitions(); err != nil {
		return fmt.Errorf("camera transition cancel failed: %w", err)
	}
	return nil
}

func (state *runtimeMapState) setGestureInProgress(inProgress bool) error {
	phase := maplibre.GesturePhaseEnd
	if inProgress {
		phase = maplibre.GesturePhaseBegin
	}
	return state.updateCamera(maplibre.CameraUpdate{GesturePhase: phase})
}

func (state *runtimeMapState) moveBy(dx, dy float64, durationMS *float64) error {
	_, err := state.mapRef.ApplyCameraDelta(maplibre.CameraDelta{
		Offset: maplibre.ScreenPoint{X: dx, Y: dy}, Animation: animationOptions(durationMS),
	})
	return err
}

func (state *runtimeMapState) scaleBy(scale float64, anchor maplibre.ScreenPoint, durationMS *float64) error {
	_, err := state.mapRef.ApplyCameraDelta(maplibre.CameraDelta{
		Kind: maplibre.CameraDeltaKindScale, Amount: scale, Anchor: &anchor, Animation: animationOptions(durationMS),
	})
	return err
}

func (state *runtimeMapState) adjustPitch(delta float64, durationMS *float64) error {
	_, err := state.mapRef.ApplyCameraDelta(maplibre.CameraDelta{
		Kind: maplibre.CameraDeltaKindPitch, Amount: delta, Animation: animationOptions(durationMS),
	})
	return err
}

func (state *runtimeMapState) adjustBearing(delta float64, durationMS *float64) error {
	_, err := state.mapRef.ApplyCameraDelta(maplibre.CameraDelta{
		Kind: maplibre.CameraDeltaKindBearing, Amount: delta, Animation: animationOptions(durationMS),
	})
	return err
}

func (state *runtimeMapState) resetOrientation(durationMS float64) error {
	zero := 0.0
	return state.updateCamera(cameraUpdate(maplibre.CameraOptions{Bearing: &zero, Pitch: &zero}, &durationMS))
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
		animation := maplibre.AnimationOptions{DurationMs: durationMS}
		update.Mode = maplibre.CameraUpdateModeEase
		update.Animation = animation
	}
	return update
}

func animationOptions(durationMS *float64) maplibre.AnimationOptions {
	return maplibre.AnimationOptions{DurationMs: durationMS}
}

func drainEvents(runtimeHandle *maplibre.RuntimeHandle, mapID uint64) (bool, error) {
	batch, err := runtimeHandle.DrainEvents()
	if err != nil {
		return false, fmt.Errorf("runtime event drain failed: %w", err)
	}
	defer batch.Close()
	events, err := batch.Get()
	if err != nil {
		return false, err
	}
	for _, event := range events.Events {
		if event.SourceType != maplibre.RuntimeEventSourceTypeMap || event.Source != mapID {
			continue
		}
		if event.Type == maplibre.RuntimeEventTypeMapRenderUpdateAvailable {
			return true, nil
		}
	}
	return false, nil
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

func (state *renderMapState) pollPending() (bool, error) {
	if state.target == nil {
		return false, nil
	}
	return state.target.PollPending()
}

func (state *renderMapState) driveFrame() (frameOutcome, error) {
	if state.target == nil {
		return frameOutcome{}, nil
	}
	return state.target.DriveFrame()
}
