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

// smokeStyle is the style a smoke run renders, so it reaches no network.
const smokeStyle = `{"version":8,"sources":{},"layers":[` +
	`{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}]}`

// newRuntimeMapState creates the runtime and its map. The runtime raises
// eventWake when it has events to drain.
func newRuntimeMapState(v viewport, smoke bool, eventWake maplibre.Wake) (*runtimeMapState, error) {
	runtimeOptions := maplibre.DefaultRuntimeOptions()
	cachePath := ":memory:"
	runtimeOptions.CachePath = &cachePath
	runtimeOptions.EventWake = eventWake
	runtimeHandle, err := maplibre.RuntimeCreate(runtimeOptions)
	if err != nil {
		return nil, fmt.Errorf("runtime create failed: %w", err)
	}
	state := &runtimeMapState{runtime: runtimeHandle}
	mapOptions := maplibre.DefaultMapOptions()
	mapOptions.InitialExtent = maplibre.LogicalExtent{Width: v.logicalWidth, Height: v.logicalHeight, ScaleFactor: v.scaleFactor}
	// A map update becomes a frame demand; the frame result's repaint flag
	// covers updates that a rendering frame asks for.
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
	if smoke {
		_, err = mapHandle.SetStyleJson([]byte(smokeStyle))
	} else {
		_, err = mapHandle.SetStyleUrl("https://tiles.openfreemap.org/styles/bright")
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
	_, err = mapHandle.UpdateCamera(maplibre.CameraUpdate{Camera: initialCamera})
	if err != nil {
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

// drainRenderUpdates drains every runtime event, and reports whether the map
// published an update to render.
func (state *runtimeMapState) drainRenderUpdates() (bool, error) {
	batch, err := state.runtime.DrainEvents()
	if err != nil {
		return false, fmt.Errorf("runtime event drain failed: %w", err)
	}
	if batch == nil {
		return false, nil
	}
	defer batch.Close()
	events, err := batch.Get()
	if err != nil {
		return false, err
	}
	for _, event := range events.Events {
		if event.SourceType != maplibre.RuntimeEventSourceTypeMap || event.Source != state.mapID {
			continue
		}
		if event.Type == maplibre.RuntimeEventTypeMapRenderUpdateAvailable {
			return true, nil
		}
	}
	return false, nil
}
