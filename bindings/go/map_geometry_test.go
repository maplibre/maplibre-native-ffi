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

	geometry := LineStringGeometry([]LatLng{{Latitude: 0, Longitude: 0}, {Latitude: 1, Longitude: 1}})
	camera, err := m.CameraForGeometry(geometry, nil)
	if err != nil {
		t.Fatalf("CameraForGeometry(): %v", err)
	}
	if camera.Center == nil || camera.Zoom == nil {
		t.Fatalf("CameraForGeometry() = %+v, want center and zoom", camera)
	}
	point, err := m.PixelForLatLng(LatLng{Latitude: 0, Longitude: 0})
	if err != nil {
		t.Fatalf("PixelForLatLng(): %v", err)
	}
	coordinate, err := m.LatLngForPixel(point)
	if err != nil {
		t.Fatalf("LatLngForPixel(): %v", err)
	}
	if coordinate.Latitude < -90 || coordinate.Latitude > 90 || coordinate.Longitude < -180 || coordinate.Longitude > 180 {
		t.Fatalf("LatLngForPixel(PixelForLatLng()) = %+v, want valid coordinate", coordinate)
	}
	points, err := m.PixelsForLatLngs([]LatLng{{Latitude: 0, Longitude: 0}, {Latitude: 1, Longitude: 1}})
	if err != nil {
		t.Fatalf("PixelsForLatLngs(): %v", err)
	}
	if len(points) != 2 {
		t.Fatalf("PixelsForLatLngs() length = %d, want 2", len(points))
	}
	coordinates, err := m.LatLngsForPixels(points)
	if err != nil {
		t.Fatalf("LatLngsForPixels(): %v", err)
	}
	if len(coordinates) != 2 {
		t.Fatalf("LatLngsForPixels() length = %d, want 2", len(coordinates))
	}
	projection, err := m.NewProjection()
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
	if err := projection.SetVisibleGeometry(Geometry{Type: GeometryType(999)}, EdgeInsets{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Projection SetVisibleGeometry(invalid) error = %v, want ErrInvalidArgument", err)
	}
}
