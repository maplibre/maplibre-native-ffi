//go:build darwin && cgo

package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
)

func TestRenderSessionWrongThreadReturnsWrongThreadDarwin(t *testing.T) {
	if !SupportedRenderBackends().Has(RenderBackendMetal) {
		t.Skip("Metal texture sessions are not supported by this build")
	}

	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	device := defaultMetalDeviceForTest()
	if device == 0 {
		t.Skip("Metal system default device is unavailable")
	}

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(64, 64, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	session, err := m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{
		Extent:  RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
		Context: MetalContextDescriptor{Device: NativePointer(device)},
	})
	if err != nil {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("AttachMetalOwnedTexture(): %v", err)
	}
	defer func() {
		if err := session.Close(); err != nil {
			t.Errorf("RenderSession Close(): %v", err)
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
		errCh <- session.RenderUpdate()
	}()
	if err := <-errCh; !errors.Is(err, ErrWrongThread) {
		t.Fatalf("RenderUpdate() from another thread error = %v, want ErrWrongThread", err)
	}
}
