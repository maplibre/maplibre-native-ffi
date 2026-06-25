package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
)

func TestMapProjectionCameraAndVisibleCoordinates(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	projection, err := m.NewProjection()
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
func TestMapProjectionSnapshotOutlivesMap(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	projection, err := m.NewProjection()
	if err != nil {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("NewProjection(): %v", err)
	}
	if err := m.Close(); err != nil {
		_ = projection.Close()
		_ = runtime.Close()
		t.Fatalf("Map Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		_ = projection.Close()
		t.Fatalf("Runtime Close(): %v", err)
	}

	coordinate := LatLng{Latitude: 0, Longitude: 0}
	point, err := projection.PixelForLatLng(coordinate)
	if err != nil {
		_ = projection.Close()
		t.Fatalf("PixelForLatLng(): %v", err)
	}
	roundTripped, err := projection.LatLngForPixel(point)
	if err != nil {
		_ = projection.Close()
		t.Fatalf("LatLngForPixel(): %v", err)
	}
	if diff := roundTripped.Latitude - coordinate.Latitude; diff < -1e-7 || diff > 1e-7 {
		t.Fatalf("latitude round trip = %f, want %f", roundTripped.Latitude, coordinate.Latitude)
	}
	if diff := roundTripped.Longitude - coordinate.Longitude; diff < -1e-7 || diff > 1e-7 {
		t.Fatalf("longitude round trip = %f, want %f", roundTripped.Longitude, coordinate.Longitude)
	}
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

func TestMapProjectionWrongThreadReturnsWrongThread(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	projection, err := m.NewProjection()
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

	errCh := make(chan error, 1)
	go func() {
		_, err := projection.Camera()
		errCh <- err
	}()
	if err := <-errCh; !errors.Is(err, ErrWrongThread) {
		t.Fatalf("Projection Camera() from another thread error = %v, want ErrWrongThread", err)
	}
	if _, err := projection.Camera(); err != nil {
		t.Fatalf("Projection Camera() on owner thread after wrong-thread call: %v", err)
	}
}
