package maplibre

import (
	"errors"
	"testing"
)

func TestMapCameraGeometryAndCoordinateConversions(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(512, 512, 1)))
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := closeMapForTest(m); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	geometry := []byte(`{"type":"LineString","coordinates":[[0,0],[1,1]]}`)
	camera, err := awaitForTest(m.CameraForGeometry(geometry, nil))
	if err != nil {
		t.Fatalf("CameraForGeometry(): %v", err)
	}
	if camera.Center == nil || camera.Zoom == nil {
		t.Fatalf("CameraForGeometry() = %+v, want center and zoom", camera)
	}
	point, err := awaitForTest(m.PixelForLatLng(LatLng{Latitude: 0, Longitude: 0}))
	if err != nil {
		t.Fatalf("PixelForLatLng(): %v", err)
	}
	coordinate, err := awaitForTest(m.LatLngForPixel(point))
	if err != nil {
		t.Fatalf("LatLngForPixel(): %v", err)
	}
	if coordinate.Latitude < -90 || coordinate.Latitude > 90 || coordinate.Longitude < -180 || coordinate.Longitude > 180 {
		t.Fatalf("LatLngForPixel(PixelForLatLng()) = %+v, want valid coordinate", coordinate)
	}
	points, err := awaitForTest(m.PixelsForLatLngs([]LatLng{{Latitude: 0, Longitude: 0}, {Latitude: 1, Longitude: 1}}))
	if err != nil {
		t.Fatalf("PixelsForLatLngs(): %v", err)
	}
	if len(points) != 2 {
		t.Fatalf("PixelsForLatLngs() length = %d, want 2", len(points))
	}
	coordinates, err := awaitForTest(m.LatLngsForPixels(points))
	if err != nil {
		t.Fatalf("LatLngsForPixels(): %v", err)
	}
	if len(coordinates) != 2 {
		t.Fatalf("LatLngsForPixels() length = %d, want 2", len(coordinates))
	}
	projection, err := awaitForTest(m.NewProjection())
	if err != nil {
		t.Fatalf("Projection(): %v", err)
	}
	defer func() {
		if err := projection.Close(); err != nil {
			t.Errorf("Projection Close(): %v", err)
		}
	}()
	if err := projection.SetVisibleGeometry(geometry, EdgeInsets{}); err != nil {
		t.Fatalf("Projection SetVisibleGeometry(): %v", err)
	}
	if err := projection.SetVisibleGeometry([]byte(`{"type":"Unsupported"}`), EdgeInsets{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Projection SetVisibleGeometry(invalid) error = %v, want ErrInvalidArgument", err)
	}
}

func TestUnwrappedCoordinateConversionsPreserveVisibleWorldCopies(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(1024, 512, 1)))
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := closeMapForTest(m); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := awaitForTest(m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 0, Longitude: 180}).WithZoom(0))); err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	points := []ScreenPoint{{X: 0, Y: 256}, {X: 1024, Y: 256}}
	wrapped, err := awaitForTest(m.LatLngsForPixels(points))
	if err != nil {
		t.Fatalf("LatLngsForPixels(): %v", err)
	}
	unwrapped, err := awaitForTest(m.LatLngsForPixelsUnwrapped(points))
	if err != nil {
		t.Fatalf("LatLngsForPixelsUnwrapped(): %v", err)
	}
	for _, coordinate := range wrapped {
		if coordinate.Longitude < -180 || coordinate.Longitude > 180 {
			t.Fatalf("wrapped longitude = %f, want -180 to 180", coordinate.Longitude)
		}
	}
	if unwrapped[1].Longitude-unwrapped[0].Longitude <= 360 {
		t.Fatalf("unwrapped span = %f, want greater than 360", unwrapped[1].Longitude-unwrapped[0].Longitude)
	}
	wrappedRight, err := awaitForTest(m.LatLngForPixel(points[1]))
	if err != nil {
		t.Fatalf("LatLngForPixel(): %v", err)
	}
	if wrappedRight.Longitude < -180 || wrappedRight.Longitude > 180 {
		t.Fatalf("wrapped longitude = %f, want -180 to 180", wrappedRight.Longitude)
	}
	right, err := awaitForTest(m.LatLngForPixelUnwrapped(points[1]))
	if err != nil {
		t.Fatalf("LatLngForPixelUnwrapped(): %v", err)
	}
	if right.Longitude != unwrapped[1].Longitude {
		t.Fatalf("LatLngForPixelUnwrapped() longitude = %f, want %f", right.Longitude, unwrapped[1].Longitude)
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
	projectedWrappedRight, err := projection.LatLngForPixel(points[1])
	if err != nil {
		t.Fatalf("projection LatLngForPixel(): %v", err)
	}
	if projectedWrappedRight.Longitude < -180 || projectedWrappedRight.Longitude > 180 {
		t.Fatalf("projection wrapped longitude = %f, want -180 to 180", projectedWrappedRight.Longitude)
	}
	projectedRight, err := projection.LatLngForPixelUnwrapped(points[1])
	if err != nil {
		t.Fatalf("projection LatLngForPixelUnwrapped(): %v", err)
	}
	if projectedRight.Longitude != right.Longitude {
		t.Fatalf("projection longitude = %f, want %f", projectedRight.Longitude, right.Longitude)
	}
}
