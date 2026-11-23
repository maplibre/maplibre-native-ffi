// Snippets for the docs site's Camera page. They compile with the tests,
// which never run them.
package docsnippets

import (
	"context"
	"errors"

	"maplibre.org/maplibre-native-ffi/bindings/go/maplibre"
)

func useCamera(camera maplibre.Camera) {}

func flyEnded(completed bool) {}

// #region gesture
func createRecognizer(
	m *maplibre.MapHandle,
) (*maplibre.GestureRecognizerHandle, error) {
	return m.GestureRecognizerCreate(nil)
}

func scheduleTick(result maplibre.GestureResult) {
	// On the next frame, or at the result's NextTickMs.
}

func pointerDown(
	recognizer *maplibre.GestureRecognizerHandle,
	id uint32, position maplibre.ScreenPoint, timeMs float64,
) error {
	result, err := recognizer.Handle(maplibre.GestureInputPointerDownVariant{
		Value: maplibre.GesturePointerDown{
			ID: id, Kind: maplibre.PointerKindTouch, Position: position,
		},
	}, timeMs)
	if err != nil {
		return err
	}
	scheduleTick(result)
	return nil
}

func pointerMove(
	recognizer *maplibre.GestureRecognizerHandle,
	id uint32, position maplibre.ScreenPoint, timeMs float64,
) error {
	result, err := recognizer.Handle(maplibre.GestureInputPointerMoveVariant{
		Value: maplibre.GesturePointer{ID: id, Position: position},
	}, timeMs)
	if err != nil {
		return err
	}
	scheduleTick(result)
	return nil
}

func tick(
	recognizer *maplibre.GestureRecognizerHandle, timeMs float64,
) error {
	result, err := recognizer.Tick(timeMs)
	if err != nil {
		return err
	}
	scheduleTick(result)
	return nil
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
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode: maplibre.CameraUpdateModeJump,
		Camera: maplibre.CameraOptions{
			Center: &maplibre.LatLng{Latitude: 48.8566, Longitude: 2.3522},
			Zoom:   new(12.0),
		},
	})
	if err != nil {
		return err
	}
	// #endregion jump

	// #region animate
	err = m.Discarding().UpdateCamera(maplibre.CameraUpdate{
		Mode: maplibre.CameraUpdateModeFly,
		Camera: maplibre.CameraOptions{
			Center: &maplibre.LatLng{Latitude: 40.7128, Longitude: -74.006},
			Zoom:   new(10.0),
		},
		Animation: maplibre.AnimationOptions{
			DurationMs: new(3000.0),
			EndHandler: maplibre.CameraTransitionHandler{
				// Runs on a library thread.
				Callback: func(end maplibre.CameraTransitionEnd) {
					flyEnded(
						end.Outcome == maplibre.CameraTransitionOutcomeCompleted,
					)
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
	err = m.Discarding().FitCameraToLatLngBounds(bounds, maplibre.CameraFitUpdate{
		Mode:      maplibre.CameraUpdateModeEase,
		Animation: maplibre.AnimationOptions{DurationMs: new(500.0)},
	})
	if err != nil {
		return err
	}
	// #endregion fit

	// #region limit
	err = m.Discarding().SetBounds(maplibre.BoundOptions{
		Bounds:  &bounds,
		MinZoom: new(5.0),
		MaxZoom: new(16.0),
	})
	if err != nil {
		return err
	}
	// #endregion limit

	recognizer, err := createRecognizer(m)
	if err != nil {
		return err
	}
	err = errors.Join(
		pointerDown(recognizer, 1, maplibre.ScreenPoint{X: 100, Y: 100}, 0),
		pointerMove(recognizer, 1, maplibre.ScreenPoint{X: 112, Y: 96}, 16),
		tick(recognizer, 32),
		recognizer.Release(),
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
	defer projection.Release()

	coordinate, err := projection.LatLngForPixel(
		maplibre.ScreenPoint{X: 100, Y: 200},
	)
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
