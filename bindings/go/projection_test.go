package maplibre

import (
	"errors"
	"math"
	"testing"
)

func TestMapProjectionCameraAndVisibleCoordinates(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(512, 512, 1)))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	projection, err := awaitForTest(m.NewProjection())
	if err != nil {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("NewProjection(): %v", err)
	}
	defer func() {
		if err := projection.Close(); err != nil {
			t.Errorf("Projection Close(): %v", err)
		}
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	// A setter is synchronous, so the conversions right after it observe it.
	before, err := projection.PixelForLatLng(LatLng{Latitude: 2, Longitude: 3})
	if err != nil {
		t.Fatalf("PixelForLatLng(): %v", err)
	}
	camera := CameraOptions{}.
		WithCenter(LatLng{Latitude: 2, Longitude: 3}).
		WithZoom(2)
	if err := projection.SetCamera(camera); err != nil {
		t.Fatalf("SetCamera(): %v", err)
	}
	gotCamera, err := projection.Camera()
	if err != nil {
		t.Fatalf("Camera(): %v", err)
	}
	if gotCamera.Center == nil || gotCamera.Zoom == nil {
		t.Fatalf("Camera() missing expected fields: %#v", gotCamera)
	}
	after, err := projection.PixelForLatLng(LatLng{Latitude: 2, Longitude: 3})
	if err != nil {
		t.Fatalf("PixelForLatLng() after SetCamera(): %v", err)
	}
	if math.Abs(after.X-before.X) < 1e-9 && math.Abs(after.Y-before.Y) < 1e-9 {
		t.Fatalf("PixelForLatLng() = %#v before and after SetCamera(), want the setter to move the conversion", after)
	}
	// The centered coordinate lands at the middle of the 512x512 viewport.
	if math.Abs(after.X-256) > 1e-6 || math.Abs(after.Y-256) > 1e-6 {
		t.Fatalf("PixelForLatLng(center) = %#v, want the viewport center", after)
	}
	if err := projection.SetVisibleCoordinates([]LatLng{{Latitude: -1, Longitude: -1}, {Latitude: 1, Longitude: 1}}, EdgeInsets{}); err != nil {
		t.Fatalf("SetVisibleCoordinates(): %v", err)
	}
	if _, err := projection.Camera(); err != nil {
		t.Fatalf("Camera() after SetVisibleCoordinates(): %v", err)
	}
	if err := projection.SetVisibleCoordinates(nil, EdgeInsets{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetVisibleCoordinates(nil) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapProjectionObservesEarlierMapCommands(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(512, 512, 1)))
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

	if _, err := m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 10, Longitude: 20}).WithZoom(3)); err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	projection, err := awaitForTest(m.NewProjection())
	if err != nil {
		t.Fatalf("NewProjection(): %v", err)
	}
	defer func() {
		if err := projection.Close(); err != nil {
			t.Errorf("Projection Close(): %v", err)
		}
	}()
	camera, err := projection.Camera()
	if err != nil {
		t.Fatalf("Camera(): %v", err)
	}
	if camera.Center == nil ||
		math.Abs(camera.Center.Latitude-10) > 1e-9 ||
		math.Abs(camera.Center.Longitude-20) > 1e-9 {
		t.Fatalf("Camera() center = %#v, want the camera the map committed before creation", camera.Center)
	}
}

func TestMapProjectionOutlivesMapAndRuntime(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(512, 512, 1)))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	projection, err := awaitForTest(m.NewProjection())
	if err != nil {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("NewProjection(): %v", err)
	}
	coordinate := LatLng{Latitude: 0, Longitude: 0}
	if err := m.Close(); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Runtime Close(): %v", err)
	}
	point, err := projection.PixelForLatLng(coordinate)
	if err != nil {
		t.Fatalf("PixelForLatLng(): %v", err)
	}
	roundTripped, err := projection.LatLngForPixel(point)
	if err != nil {
		t.Fatalf("LatLngForPixel(): %v", err)
	}
	if diff := roundTripped.Latitude - coordinate.Latitude; diff < -1e-7 || diff > 1e-7 {
		t.Fatalf("latitude round trip = %f, want %f", roundTripped.Latitude, coordinate.Latitude)
	}
	if diff := roundTripped.Longitude - coordinate.Longitude; diff < -1e-7 || diff > 1e-7 {
		t.Fatalf("longitude round trip = %f, want %f", roundTripped.Longitude, coordinate.Longitude)
	}
	// Close is synchronous: when it returns, the independent handle is retired.
	if err := projection.Close(); err != nil {
		t.Fatalf("Projection Close(): %v", err)
	}
	if err := projection.Close(); err != nil {
		t.Fatalf("second Projection Close(): %v", err)
	}
	if _, err := projection.PixelForLatLng(coordinate); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("PixelForLatLng() after close error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapProjectionCanMigrateAcrossGoroutines(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(512, 512, 1)))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	projection, err := awaitForTest(m.NewProjection())
	if err != nil {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("NewProjection(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()
	defer projection.Close()

	// A setter on this goroutine is observed by a conversion on another one.
	if err := projection.SetCamera(CameraOptions{}.WithCenter(LatLng{Latitude: 5, Longitude: 6}).WithZoom(2)); err != nil {
		t.Fatalf("SetCamera(): %v", err)
	}
	type conversion struct {
		point ScreenPoint
		err   error
	}
	result := make(chan conversion, 1)
	go func() {
		point, err := projection.PixelForLatLng(LatLng{Latitude: 5, Longitude: 6})
		result <- conversion{point: point, err: err}
	}()
	got := <-result
	if got.err != nil {
		t.Fatalf("Projection PixelForLatLng() from another goroutine: %v", got.err)
	}
	if math.Abs(got.point.X-256) > 1e-6 || math.Abs(got.point.Y-256) > 1e-6 {
		t.Fatalf("PixelForLatLng(center) from another goroutine = %#v, want the viewport center", got.point)
	}

	// A close on another goroutine retires the handle for every goroutine.
	errCh := make(chan error, 1)
	go func() {
		errCh <- projection.Close()
	}()
	if err := <-errCh; err != nil {
		t.Fatalf("Projection Close() from another goroutine: %v", err)
	}
	if _, err := projection.Camera(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Projection Camera() after cross-goroutine close error = %v, want ErrInvalidArgument", err)
	}
}
