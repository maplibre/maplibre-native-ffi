package main

import (
	"context"
	"errors"
	"fmt"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

type runtimeMapState struct {
	runtime *maplibre.RuntimeHandle
	mapRef  *maplibre.MapHandle
}

// smokeStyle is the style a smoke run renders, so it reaches no network.
const smokeStyle = `{"version":8,"sources":{},"layers":[` +
	`{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}]}`

// newRuntimeMapState creates the runtime and its map.
func newRuntimeMapState(v viewport, smoke bool) (*runtimeMapState, error) {
	runtimeOptions := maplibre.DefaultRuntimeOptions()
	runtimeOptions.CachePath = ":memory:"
	runtimeHandle, err := maplibre.RuntimeCreate(runtimeOptions)
	if err != nil {
		return nil, fmt.Errorf("runtime create failed: %w", err)
	}
	state := &runtimeMapState{runtime: runtimeHandle}
	mapOptions := maplibre.DefaultMapOptions()
	mapOptions.InitialExtent = v.extent()
	mapOptions.EventMask = 0
	mapHandle, ready, err := runtimeHandle.CreateMap(mapOptions)
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map create failed: %w", err)
	}
	// The map is ours from the call, so Close releases it even when its
	// creation fails.
	state.mapRef = mapHandle
	if _, err = ready.Await(context.Background()); err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map create failed: %w", err)
	}
	// A style that fails to apply leaves nothing to render, so its command is
	// awaited. The camera commands are fire-and-forget, and native logs any
	// that fails.
	var loaded *maplibre.Future[maplibre.CommandCompletion]
	if smoke {
		loaded, err = mapHandle.SetStyleJson(smokeStyle)
	} else {
		loaded, err = mapHandle.SetStyleUrl("https://tiles.openfreemap.org/styles/bright")
	}
	if err == nil {
		_, err = loaded.Await(context.Background())
	}
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("style load failed: %w", err)
	}
	zoom, bearing, pitch := 13.0, 12.0, 30.0
	initialCamera := maplibre.CameraOptions{
		Center: &maplibre.LatLng{Latitude: 37.7749, Longitude: -122.4194},
		Zoom:   &zoom, Bearing: &bearing, Pitch: &pitch,
	}
	if err = mapHandle.Discarding().UpdateCamera(maplibre.CameraUpdate{Camera: initialCamera}); err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("camera jump failed: %w", err)
	}
	return state, nil
}

func (state *runtimeMapState) Close() error {
	var result error
	// Awaiting both release completions lets native teardown finish before
	// the app tears down state that the callbacks use.
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
	if err := state.mapRef.Discarding().CancelTransitions(); err != nil {
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
	return state.mapRef.Discarding().ApplyCameraDelta(maplibre.CameraDelta{
		Offset: &maplibre.ScreenPoint{X: dx, Y: dy}, Animation: animationOptions(durationMS),
	})
}

func (state *runtimeMapState) scaleBy(scale float64, anchor maplibre.ScreenPoint, durationMS *float64) error {
	return state.mapRef.Discarding().ApplyCameraDelta(maplibre.CameraDelta{
		Scale: &scale, Anchor: &anchor, Animation: animationOptions(durationMS),
	})
}

// adjustOrientation adds bearing and pitch degrees in one delta. A nil value
// leaves that component unchanged.
func (state *runtimeMapState) adjustOrientation(bearing, pitch, durationMS *float64) error {
	return state.mapRef.Discarding().ApplyCameraDelta(maplibre.CameraDelta{
		Bearing: bearing, Pitch: pitch, Animation: animationOptions(durationMS),
	})
}

func (state *runtimeMapState) resetOrientation(durationMS float64) error {
	zero := 0.0
	return state.updateCamera(cameraUpdate(maplibre.CameraOptions{Bearing: &zero, Pitch: &zero}, &durationMS))
}

func (state *runtimeMapState) updateCamera(update maplibre.CameraUpdate) error {
	if err := state.mapRef.Discarding().UpdateCamera(update); err != nil {
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
