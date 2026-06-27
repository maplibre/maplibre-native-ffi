package maplibre

import (
	"errors"
	"testing"
	"time"
)

func waitForRuntimeEvent(t *testing.T, runtime *RuntimeHandle, eventType RuntimeEventType) *RuntimeEvent {
	t.Helper()
	for range make([]struct{}, 5000) {
		if err := runtime.RunOnce(); err != nil {
			t.Fatalf("RunOnce(): %v", err)
		}
		event, err := runtime.PollEvent()
		if err != nil {
			t.Fatalf("PollEvent(): %v", err)
		}
		if event != nil && event.Type == eventType {
			return event
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("timed out waiting for runtime event %v", eventType)
	return nil
}
func TestStyleImageCopiesPixelsAndMetadata(t *testing.T) {
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	pixels := []byte{255, 0, 0, 255}
	pixelRatio := float32(2)
	sdf := true
	if err := m.SetStyleImage("marker", PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: pixels}, StyleImageOptions{PixelRatio: &pixelRatio, SDF: &sdf}); err != nil {
		t.Fatalf("SetStyleImage(): %v", err)
	}
	pixels[0] = 0
	exists, err := m.StyleImageExists("marker")
	if err != nil {
		t.Fatalf("StyleImageExists(marker): %v", err)
	}
	if !exists {
		t.Fatal("StyleImageExists(marker)=false, want true")
	}
	info, found, err := m.StyleImageInfo("marker")
	if err != nil {
		t.Fatalf("StyleImageInfo(marker): %v", err)
	}
	if !found || info.Width != 1 || info.Height != 1 || info.Stride != 4 || info.ByteLength != 4 || info.PixelRatio != pixelRatio || info.SDF != sdf {
		t.Fatalf("StyleImageInfo(marker) = (%+v, %v), want copied 1x1 image metadata", info, found)
	}
	copied, found, err := m.StyleImagePremultipliedRGBA8("marker")
	if err != nil {
		t.Fatalf("StyleImagePremultipliedRGBA8(marker): %v", err)
	}
	if !found || len(copied) != 4 || copied[0] != 255 || copied[1] != 0 || copied[2] != 0 || copied[3] != 255 {
		t.Fatalf("StyleImagePremultipliedRGBA8(marker) = (%v, %v), want original copied pixels", copied, found)
	}
	removed, err := m.RemoveStyleImage("marker")
	if err != nil {
		t.Fatalf("RemoveStyleImage(marker): %v", err)
	}
	if !removed {
		t.Fatal("RemoveStyleImage(marker)=false, want true")
	}
	if err := m.SetStyleImage("bad-marker", PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4}, StyleImageOptions{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleImage(empty pixels) error = %v, want ErrInvalidArgument", err)
	}
}
func TestImageSourceCopiesPixelsAndCoordinates(t *testing.T) {
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	coordinates := []LatLng{
		{Latitude: 1, Longitude: 1},
		{Latitude: 1, Longitude: 2},
		{Latitude: 0, Longitude: 2},
		{Latitude: 0, Longitude: 1},
	}
	pixels := []byte{0, 255, 0, 255}
	if err := m.AddImageSourceImage("image-source", coordinates, PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: pixels}); err != nil {
		t.Fatalf("AddImageSourceImage(): %v", err)
	}
	coordinates[0] = LatLng{Latitude: 9, Longitude: 9}
	pixels[1] = 0
	gotCoordinates, found, err := m.ImageSourceCoordinates("image-source")
	if err != nil {
		t.Fatalf("ImageSourceCoordinates(image-source): %v", err)
	}
	if !found || len(gotCoordinates) != 4 || gotCoordinates[0] != (LatLng{Latitude: 1, Longitude: 1}) {
		t.Fatalf("ImageSourceCoordinates(image-source) = (%v, %v), want original copied coordinates", gotCoordinates, found)
	}
	updatedCoordinates := []LatLng{
		{Latitude: 2, Longitude: 2},
		{Latitude: 2, Longitude: 3},
		{Latitude: 1, Longitude: 3},
		{Latitude: 1, Longitude: 2},
	}
	if err := m.SetImageSourceCoordinates("image-source", updatedCoordinates); err != nil {
		t.Fatalf("SetImageSourceCoordinates(): %v", err)
	}
	if err := m.SetImageSourceImage("image-source", PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: []byte{0, 0, 255, 255}}); err != nil {
		t.Fatalf("SetImageSourceImage(): %v", err)
	}
	gotCoordinates, found, err = m.ImageSourceCoordinates("image-source")
	if err != nil {
		t.Fatalf("ImageSourceCoordinates(image-source after update): %v", err)
	}
	if !found || gotCoordinates[0] != updatedCoordinates[0] {
		t.Fatalf("ImageSourceCoordinates(image-source after update) = (%v, %v), want updated coordinates", gotCoordinates, found)
	}
	if err := m.AddImageSourceURL("image-url-source", updatedCoordinates, "asset://fixtures/image.png"); err != nil {
		t.Fatalf("AddImageSourceURL(): %v", err)
	}
	if err := m.SetImageSourceURL("image-url-source", "asset://fixtures/image-2.png"); err != nil {
		t.Fatalf("SetImageSourceURL(): %v", err)
	}
	if err := m.AddImageSourceImage("bad-image-source", updatedCoordinates[:3], PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: []byte{0, 0, 0, 0}}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddImageSourceImage(3 coordinates) error = %v, want ErrInvalidArgument", err)
	}
}
