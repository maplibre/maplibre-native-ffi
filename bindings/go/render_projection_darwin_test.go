//go:build darwin && cgo

package maplibre

import (
	"errors"
	"testing"
)

func TestRenderedProjectionDarwin(t *testing.T) {
	_, m, session, attached := newMetalOwnedTextureSession(t, RenderDriverCoreWorker)
	awaitWithDeadline(t, attached)
	if _, err := session.NewProjection(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("before render: %v", err)
	}
	update := CameraUpdate{Camera: CameraOptions{}.WithZoom(3)}
	if _, err := awaitForTest(m.UpdateCamera(update)); err != nil {
		t.Fatal(err)
	}
	if _, err := awaitForTest(m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`))); err != nil {
		t.Fatal(err)
	}
	awaitRenderedMetalFrame(t, session)
	frame, err := session.AcquireFrame()
	if err != nil {
		t.Fatal(err)
	}
	update.Camera = CameraOptions{}.WithZoom(6)
	if _, err := awaitForTest(m.UpdateCamera(update)); err != nil {
		t.Fatal(err)
	}
	projection, err := session.NewProjection()
	if err != nil {
		t.Fatal(err)
	}
	defer projection.Close()
	camera, err := projection.Camera()
	if err != nil || camera.Zoom == nil || *camera.Zoom != 3 {
		t.Fatalf("rendered camera: %+v, %v", camera, err)
	}
	if err := frame.Release(GPUSync{}); err != nil {
		t.Fatal(err)
	}
	if _, err := awaitForTest(session.Resize(RenderTargetExtent{Width: 16, Height: 16, ScaleFactor: 1})); err != nil {
		t.Fatal(err)
	}
	if _, err := session.NewProjection(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("after resize: %v", err)
	}
	if _, err := awaitForTest(session.Detach()); err != nil {
		t.Fatal(err)
	}
	if err := session.Close(); err != nil {
		t.Fatal(err)
	}
	if err := closeMapForTest(m); err != nil {
		t.Fatal(err)
	}
	result := make(chan error, 1)
	go func() {
		camera, err := projection.Camera()
		if err == nil && (camera.Zoom == nil || *camera.Zoom != 3) {
			err = errors.New("snapshot changed")
		}
		result <- err
	}()
	if err := <-result; err != nil {
		t.Fatal(err)
	}
}
