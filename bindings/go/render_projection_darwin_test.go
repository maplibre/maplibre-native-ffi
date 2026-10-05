//go:build darwin && cgo

package maplibre

import (
	"errors"
	"testing"
	"time"
)

func TestRenderedProjectionDarwin(t *testing.T) {
	if !SupportedRenderBackends().Has(RenderBackendMetal) {
		t.Skip("Metal is not supported by this build")
	}
	lockOSThreadForTest(t)
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatal(err)
	}
	defer runtime.Close()
	m, err := runtime.NewMapWithOptions(NewMapOptions(64, 64, 1))
	if err != nil {
		t.Fatal(err)
	}
	defer m.Close()
	session, err := m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{
		Extent:  RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
		Context: MetalContextDescriptor{Device: NativePointer(defaultMetalDeviceForTest())},
	})
	if err != nil {
		t.Fatal(err)
	}
	defer session.Close()
	if _, err := session.NewProjection(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("before render: %v", err)
	}
	if err := m.JumpTo(CameraOptions{}.WithZoom(3)); err != nil {
		t.Fatal(err)
	}
	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatal(err)
	}
	deadline := time.Now().Add(10 * time.Second)
	for {
		if err := runtime.Pump(0, 0); err != nil {
			t.Fatal(err)
		}
		update, err := session.RenderUpdate()
		if err != nil {
			t.Fatal(err)
		}
		if update.Result == RenderResultRendered {
			break
		}
		if time.Now().After(deadline) {
			t.Fatal("render timed out")
		}
		time.Sleep(time.Millisecond)
	}
	frame, err := session.AcquireMetalTextureFrame()
	if err != nil {
		t.Fatal(err)
	}
	defer frame.Close()
	if err := m.JumpTo(CameraOptions{}.WithZoom(6)); err != nil {
		t.Fatal(err)
	}
	if err := runtime.Pump(0, 0); err != nil {
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
	current, err := m.Camera()
	if err != nil || current.Zoom == nil || *current.Zoom != 6 {
		t.Fatalf("map camera: %+v, %v", current, err)
	}
	if err := frame.Close(); err != nil {
		t.Fatal(err)
	}
	if err := session.Resize(RenderTargetExtent{Width: 32, Height: 32, ScaleFactor: 1}); err != nil {
		t.Fatal(err)
	}
	if _, err := session.NewProjection(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("after resize: %v", err)
	}
	if err := session.Close(); err != nil {
		t.Fatal(err)
	}
	if err := m.Close(); err != nil {
		t.Fatal(err)
	}
	result := make(chan error, 1)
	go func() {
		camera, err := projection.Camera()
		if err == nil && (camera.Zoom == nil || *camera.Zoom != 3) {
			err = errors.New("snapshot changed")
		}
		if err == nil {
			err = projection.Close()
		}
		result <- err
	}()
	if err := <-result; err != nil {
		t.Fatal(err)
	}
}
