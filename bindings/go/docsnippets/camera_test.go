// Snippets for the docs site's Camera page. They compile with the tests,
// which never run them.
package docsnippets

import (
	"context"
	"errors"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func useCamera(camera maplibre.Camera) {}

func flyEnded(completed bool) {}

// #region gesture
func beginGesture(m *maplibre.MapHandle) error {
	if err := m.Discarding().CancelTransitions(); err != nil {
		return err
	}
	return m.Discarding().ApplyCameraDelta(maplibre.CameraDelta{
		GesturePhase: maplibre.GesturePhaseBegin,
	})
}

func applyDrag(m *maplibre.MapHandle, movement maplibre.ScreenPoint) error {
	return m.Discarding().ApplyCameraDelta(maplibre.CameraDelta{
		Offset:       &movement,
		GesturePhase: maplibre.GesturePhaseUpdate,
	})
}

func applyPinch(m *maplibre.MapHandle, scale float64, center maplibre.ScreenPoint) error {
	return m.Discarding().ApplyCameraDelta(maplibre.CameraDelta{
		Scale:        &scale,
		Anchor:       &center,
		GesturePhase: maplibre.GesturePhaseUpdate,
	})
}

func endGesture(m *maplibre.MapHandle) error {
	return m.Discarding().ApplyCameraDelta(maplibre.CameraDelta{
		GesturePhase: maplibre.GesturePhaseEnd,
	})
}

// #endregion gesture

func camera(ctx context.Context, m *maplibre.MapHandle) error {
	// #region read
	snapshot, err := m.Snapshot()
	if err != nil {
		return err
	}
	useCamera(snapshot.View.Camera)

	query, err := m.GetCamera()
	if err != nil {
		return err
	}
	result, err := query.Await(ctx)
	if err != nil {
		return err
	}
	useCamera(result.Camera)
	// #endregion read

	// #region jump
	zoom := 12.0
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode: maplibre.CameraUpdateModeJump,
		Camera: maplibre.CameraOptions{
			Center: &maplibre.LatLng{Latitude: 48.8566, Longitude: 2.3522},
			Zoom:   &zoom,
		},
	})
	if err != nil {
		return err
	}
	// #endregion jump

	// #region animate
	flyZoom, flyDuration := 10.0, 3000.0
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode: maplibre.CameraUpdateModeFly,
		Camera: maplibre.CameraOptions{
			Center: &maplibre.LatLng{Latitude: 40.7128, Longitude: -74.006},
			Zoom:   &flyZoom,
		},
		Animation: maplibre.AnimationOptions{
			DurationMs: &flyDuration,
			EndHandler: maplibre.CameraTransitionHandler{
				// Runs on a library thread.
				Callback: func(end maplibre.CameraTransitionEnd) {
					flyEnded(end.Outcome == maplibre.CameraTransitionOutcomeCompleted)
				},
			},
		},
	})
	if err != nil {
		return err
	}
	// #endregion animate

	// #region fit
	bounds := maplibre.LatLngBounds{
		Southwest: maplibre.LatLng{Latitude: 45.8, Longitude: 5.9},
		Northeast: maplibre.LatLng{Latitude: 47.8, Longitude: 10.5},
	}
	// nil fit options use the defaults.
	fit, err := m.CameraForLatLngBounds(bounds, nil)
	if err != nil {
		return err
	}
	fitted, err := fit.Await(ctx)
	if err != nil {
		return err
	}

	easeDuration := 500.0
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode:      maplibre.CameraUpdateModeEase,
		Camera:    fitted,
		Animation: maplibre.AnimationOptions{DurationMs: &easeDuration},
	})
	if err != nil {
		return err
	}
	// #endregion fit

	// #region limit
	minZoom, maxZoom := 5.0, 16.0
	err = m.Discarding().SetBounds(maplibre.BoundOptions{
		Bounds:  &bounds,
		MinZoom: &minZoom,
		MaxZoom: &maxZoom,
	})
	if err != nil {
		return err
	}
	// #endregion limit

	err = errors.Join(
		beginGesture(m),
		applyDrag(m, maplibre.ScreenPoint{X: 12, Y: -4}),
		applyPinch(m, 1.1, maplibre.ScreenPoint{X: 200, Y: 150}),
		endGesture(m),
	)
	if err != nil {
		return err
	}

	// #region convert
	current, err := m.Snapshot()
	if err != nil {
		return err
	}
	projection, err := maplibre.NewMapProjection(current.View)
	if err != nil {
		return err
	}
	defer projection.Close()

	coordinate, err := projection.LatLngForPixel(maplibre.ScreenPoint{X: 100, Y: 200})
	if err != nil {
		return err
	}
	point, err := projection.PixelForLatLng(coordinate)
	if err != nil {
		return err
	}
	// #endregion convert
	_ = point
	return nil
}

// An example without an output comment compiles but never runs.
func Example_camera() {
	_ = camera(context.Background(), nil)
}
