//go:build darwin && cgo

package maplibre

import (
	"errors"
	"testing"
)

// A session-owned texture session is the only session this suite can create, so
// it covers only the target kinds SetTarget rejects; replacing a host-owned
// target would need a host-allocated surface or texture.
func TestSetTargetRejectsOtherTargetKindsDarwin(t *testing.T) {
	if !SupportedRenderBackends().Has(RenderBackendMetal) {
		t.Skip("Metal texture sessions are not supported by this build")
	}

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
	extent := RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1}
	session, err := m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{
		Extent:  extent,
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

	if err := session.SetMetalSurfaceTarget(MetalSurfaceDescriptor{
		Extent:  extent,
		Context: MetalContextDescriptor{Device: NativePointer(device)},
	}); !errors.Is(err, ErrUnsupported) {
		t.Fatalf("SetMetalSurfaceTarget() on a texture session error = %v, want ErrUnsupported", err)
	}
	if err := session.SetMetalBorrowedTextureTarget(MetalBorrowedTextureDescriptor{
		Extent:         extent,
		PhysicalWidth:  64,
		PhysicalHeight: 64,
	}); !errors.Is(err, ErrUnsupported) {
		t.Fatalf("SetMetalBorrowedTextureTarget() on a session-owned texture session error = %v, want ErrUnsupported", err)
	}

	// Both rejections leave the original target usable. Autonomous map work may
	// already have produced a render update, so only the successful call is
	// invariant here.
	if _, err := session.RenderUpdate(); err != nil {
		t.Fatalf("RenderUpdate() after rejected target replacement: %v", err)
	}
}
