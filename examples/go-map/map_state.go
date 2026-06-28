package main

import (
	"errors"
	"fmt"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

type mapState struct {
	runtime *maplibre.RuntimeHandle
	mapRef  *maplibre.MapHandle
	target  renderTarget
}

func newMapState(window *sdl.Window, v viewport, mode renderTargetMode) (*mapState, error) {
	runtimeHandle, err := maplibre.NewRuntimeWithOptions(maplibre.RuntimeOptions{CachePath: ":memory:"})
	if err != nil {
		return nil, fmt.Errorf("runtime create failed: %w", err)
	}
	state := &mapState{runtime: runtimeHandle}

	mapHandle, err := runtimeHandle.NewMapWithOptions(maplibre.NewMapOptions(v.logicalWidth, v.logicalHeight, v.scaleFactor))
	if err != nil {
		_ = state.Close()
		return nil, fmt.Errorf("map create failed: %w", err)
	}
	state.mapRef = mapHandle

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

	target, err := newOpenGLRenderTarget(window, v, mode, mapHandle)
	if err != nil {
		_ = state.Close()
		return nil, err
	}
	state.target = target
	return state, nil
}

func (state *mapState) Close() error {
	var result error
	if state.target != nil {
		result = errors.Join(result, state.target.Close())
		state.target = nil
	}
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

func (state *mapState) resize(window *sdl.Window, v viewport, mode renderTargetMode) error {
	if state.target == nil {
		return errors.New("render target is not attached")
	}
	if state.target.NeedsReattachOnResize() {
		if err := state.target.Close(); err != nil {
			return err
		}
		state.target = nil
		target, err := newOpenGLRenderTarget(window, v, mode, state.mapRef)
		if err != nil {
			return err
		}
		state.target = target
		return nil
	}
	return state.target.Resize(v)
}

func (state *mapState) finishFrame() error {
	if state.target == nil {
		return nil
	}
	return state.target.FinishFrame()
}

func (state *mapState) renderUpdate() (bool, error) {
	if state.target == nil {
		return false, nil
	}
	return state.target.RenderUpdate()
}

func drainEvents(runtimeHandle *maplibre.RuntimeHandle) (bool, error) {
	renderPending := false
	for {
		event, err := runtimeHandle.PollEvent()
		if err != nil {
			return renderPending, err
		}
		if event == nil {
			return renderPending, nil
		}
		if event.SourceType != maplibre.RuntimeEventSourceMap {
			continue
		}
		switch event.Type {
		case maplibre.RuntimeEventMapRenderUpdateAvailable:
			renderPending = true
		case maplibre.RuntimeEventMapRenderFrameFinished:
			payload, ok := event.Payload.(maplibre.RuntimeEventRenderFramePayload)
			if ok && payload.NeedsRepaint {
				renderPending = true
			}
		}
	}
}
