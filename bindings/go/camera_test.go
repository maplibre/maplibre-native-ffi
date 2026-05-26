package maplibre

import (
	"errors"
	"testing"
)

func TestMapCameraCommandsUseNativeABI(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	camera := CameraOptions{}.
		WithCenter(LatLng{Latitude: 10, Longitude: 20}).
		WithZoom(2).
		WithBearing(15).
		WithPitch(20)
	if err := m.JumpTo(camera); err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	gotCamera, err := m.Camera()
	if err != nil {
		t.Fatalf("Camera(): %v", err)
	}
	if gotCamera.Center == nil || gotCamera.Zoom == nil {
		t.Fatalf("Camera() missing expected fields: %#v", gotCamera)
	}
	if err := m.MoveBy(ScreenPoint{X: 1, Y: 2}); err != nil {
		t.Fatalf("MoveBy(): %v", err)
	}
	anchor := ScreenPoint{X: 256, Y: 256}
	if err := m.ScaleBy(1.1, &anchor); err != nil {
		t.Fatalf("ScaleBy(): %v", err)
	}
	if err := m.RotateBy(ScreenPoint{X: 100, Y: 100}, ScreenPoint{X: 120, Y: 110}); err != nil {
		t.Fatalf("RotateBy(): %v", err)
	}
	if err := m.PitchBy(1); err != nil {
		t.Fatalf("PitchBy(): %v", err)
	}
	if err := m.CancelTransitions(); err != nil {
		t.Fatalf("CancelTransitions(): %v", err)
	}
}
func TestMapAnimatedCameraCommandsUseOptionalAnimationOptions(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.CancelTransitions(); err != nil {
			t.Errorf("CancelTransitions(): %v", err)
		}
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	camera := CameraOptions{}.
		WithCenter(LatLng{Latitude: 1, Longitude: 2}).
		WithZoom(1)
	animation := AnimationOptions{}.
		WithDurationMS(0).
		WithVelocity(1.2).
		WithMinZoom(0).
		WithEasing(UnitBezier{X1: 0.25, Y1: 0.1, X2: 0.25, Y2: 1})
	if err := m.EaseTo(camera, &animation); err != nil {
		t.Fatalf("EaseTo(): %v", err)
	}
	if err := m.FlyTo(camera, nil); err != nil {
		t.Fatalf("FlyTo(nil animation): %v", err)
	}
	if err := m.MoveByAnimated(ScreenPoint{X: 1, Y: 1}, &animation); err != nil {
		t.Fatalf("MoveByAnimated(): %v", err)
	}
	anchor := ScreenPoint{X: 256, Y: 256}
	if err := m.ScaleByAnimated(1.01, &anchor, nil); err != nil {
		t.Fatalf("ScaleByAnimated(nil animation): %v", err)
	}
	if err := m.RotateByAnimated(ScreenPoint{X: 100, Y: 100}, ScreenPoint{X: 110, Y: 100}, &animation); err != nil {
		t.Fatalf("RotateByAnimated(): %v", err)
	}
	if err := m.PitchByAnimated(0.5, &animation); err != nil {
		t.Fatalf("PitchByAnimated(): %v", err)
	}
}
func TestMapCameraFitAndBoundsHelpers(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	bounds := LatLngBounds{
		Southwest: LatLng{Latitude: -10, Longitude: -20},
		Northeast: LatLng{Latitude: 10, Longitude: 20},
	}
	fitOptions := CameraFitOptions{}.
		WithPadding(EdgeInsets{Top: 4, Left: 3, Bottom: 2, Right: 1}).
		WithBearing(0).
		WithPitch(0)
	camera, err := m.CameraForLatLngBounds(bounds, &fitOptions)
	if err != nil {
		t.Fatalf("CameraForLatLngBounds(): %v", err)
	}
	if camera.Center == nil || camera.Zoom == nil {
		t.Fatalf("CameraForLatLngBounds() missing expected fields: %#v", camera)
	}
	camera, err = m.CameraForLatLngs([]LatLng{bounds.Southwest, bounds.Northeast}, nil)
	if err != nil {
		t.Fatalf("CameraForLatLngs(): %v", err)
	}
	wrapped, err := m.LatLngBoundsForCamera(camera)
	if err != nil {
		t.Fatalf("LatLngBoundsForCamera(): %v", err)
	}
	if wrapped.Southwest.Latitude > wrapped.Northeast.Latitude {
		t.Fatalf("LatLngBoundsForCamera() inverted latitude bounds: %#v", wrapped)
	}
	if _, err := m.LatLngBoundsForCameraUnwrapped(camera); err != nil {
		t.Fatalf("LatLngBoundsForCameraUnwrapped(): %v", err)
	}

	constraints := BoundOptions{}.
		WithBounds(bounds).
		WithMinZoom(0).
		WithMaxZoom(20).
		WithMinPitch(0).
		WithMaxPitch(60)
	if err := m.SetBounds(constraints); err != nil {
		t.Fatalf("SetBounds(): %v", err)
	}
	gotConstraints, err := m.Bounds()
	if err != nil {
		t.Fatalf("Bounds(): %v", err)
	}
	if gotConstraints.Bounds == nil || *gotConstraints.Bounds != bounds {
		t.Fatalf("Bounds().Bounds = %#v, want %#v", gotConstraints.Bounds, bounds)
	}
}
func TestMapFreeCameraOptionsRoundTripCurrentValues(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	freeCamera, err := m.FreeCameraOptions()
	if err != nil {
		t.Fatalf("FreeCameraOptions(): %v", err)
	}
	if freeCamera.Position == nil || freeCamera.Orientation == nil {
		t.Fatalf("FreeCameraOptions() missing expected fields: %#v", freeCamera)
	}
	if err := m.SetFreeCameraOptions(FreeCameraOptions{}.
		WithPosition(*freeCamera.Position).
		WithOrientation(*freeCamera.Orientation)); err != nil {
		t.Fatalf("SetFreeCameraOptions(current values): %v", err)
	}
}
func TestMapCameraCommandsReportNativeValidation(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if err := m.ScaleBy(0, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ScaleBy(0) error = %v, want ErrInvalidArgument", err)
	}
	invalidAnimation := AnimationOptions{}.WithDurationMS(-1)
	if err := m.MoveByAnimated(ScreenPoint{}, &invalidAnimation); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("MoveByAnimated(invalid animation) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.CameraForLatLngs(nil, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("CameraForLatLngs(nil) error = %v, want ErrInvalidArgument", err)
	}
	invalidBounds := BoundOptions{}.WithMinZoom(3).WithMaxZoom(2)
	if err := m.SetBounds(invalidBounds); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetBounds(invalid min/max) error = %v, want ErrInvalidArgument", err)
	}
	invalidFreeCamera := FreeCameraOptions{}.WithOrientation(Quaternion{})
	if err := m.SetFreeCameraOptions(invalidFreeCamera); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetFreeCameraOptions(invalid orientation) error = %v, want ErrInvalidArgument", err)
	}
}
func TestMapViewportTileAndProjectionOptionsRoundTrip(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	viewport := ViewportOptions{}.
		WithNorthOrientation(NorthOrientationUp).
		WithConstrainMode(ConstrainModeWidthAndHeight).
		WithViewportMode(ViewportModeDefault).
		WithFrustumOffset(EdgeInsets{Top: 1, Left: 2, Bottom: 3, Right: 4})
	if err := m.SetViewportOptions(viewport); err != nil {
		t.Fatalf("SetViewportOptions(): %v", err)
	}
	gotViewport, err := m.ViewportOptions()
	if err != nil {
		t.Fatalf("ViewportOptions(): %v", err)
	}
	if gotViewport.NorthOrientation == nil || *gotViewport.NorthOrientation != NorthOrientationUp {
		t.Fatalf("ViewportOptions().NorthOrientation = %v", gotViewport.NorthOrientation)
	}
	if gotViewport.FrustumOffset == nil || *gotViewport.FrustumOffset != (EdgeInsets{Top: 1, Left: 2, Bottom: 3, Right: 4}) {
		t.Fatalf("ViewportOptions().FrustumOffset = %#v", gotViewport.FrustumOffset)
	}

	tileOptions := TileOptions{}.
		WithPrefetchZoomDelta(2).
		WithLODMode(TileLODModeDefault)
	if err := m.SetTileOptions(tileOptions); err != nil {
		t.Fatalf("SetTileOptions(): %v", err)
	}
	gotTileOptions, err := m.TileOptions()
	if err != nil {
		t.Fatalf("TileOptions(): %v", err)
	}
	if gotTileOptions.PrefetchZoomDelta == nil || *gotTileOptions.PrefetchZoomDelta != 2 {
		t.Fatalf("TileOptions().PrefetchZoomDelta = %v", gotTileOptions.PrefetchZoomDelta)
	}

	projectionMode := ProjectionModeOptions{}.
		WithAxonometric(true).
		WithSkew(0.5, 0.25)
	if err := m.SetProjectionMode(projectionMode); err != nil {
		t.Fatalf("SetProjectionMode(): %v", err)
	}
	gotProjectionMode, err := m.ProjectionMode()
	if err != nil {
		t.Fatalf("ProjectionMode(): %v", err)
	}
	if gotProjectionMode.Axonometric == nil || !*gotProjectionMode.Axonometric {
		t.Fatalf("ProjectionMode().Axonometric = %v", gotProjectionMode.Axonometric)
	}
}
func TestTileOptionsRejectInvalidPrefetchZoomDelta(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if err := m.SetTileOptions(TileOptions{}.WithPrefetchZoomDelta(256)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetTileOptions(invalid prefetch) error = %v, want ErrInvalidArgument", err)
	}
}
