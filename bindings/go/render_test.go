package maplibre

import (
	"errors"
	"sync/atomic"
	"testing"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

func TestRenderSessionNilHandleAndInvalidSurfaceDescriptor(t *testing.T) {
	lockOSThreadForTest(t)

	var nilSession *RenderSessionHandle
	if err := nilSession.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil RenderSessionHandle Close() error = %v, want ErrInvalidArgument", err)
	}
	if err := nilSession.RenderUpdate(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil RenderSessionHandle RenderUpdate() error = %v, want ErrInvalidArgument", err)
	}
	if _, err := nilSession.ReadPremultipliedRGBA8Into(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil RenderSessionHandle ReadPremultipliedRGBA8Into() error = %v, want ErrInvalidArgument", err)
	}
	var nilMetalFrame *MetalOwnedTextureFrame
	if err := nilMetalFrame.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil MetalOwnedTextureFrame Close() error = %v, want ErrInvalidArgument", err)
	}
	var nilVulkanFrame *VulkanOwnedTextureFrame
	if err := nilVulkanFrame.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil VulkanOwnedTextureFrame Close() error = %v, want ErrInvalidArgument", err)
	}
	var nilOpenGLFrame *OpenGLOwnedTextureFrame
	if err := nilOpenGLFrame.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil OpenGLOwnedTextureFrame Close() error = %v, want ErrInvalidArgument", err)
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
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	_, err = m.AttachMetalSurface(MetalSurfaceDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachMetalSurface(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachMetalOwnedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachMetalBorrowedTexture(MetalBorrowedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachMetalBorrowedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachVulkanOwnedTexture(VulkanOwnedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachVulkanOwnedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachVulkanBorrowedTexture(VulkanBorrowedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachVulkanBorrowedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachOpenGLSurface(OpenGLSurfaceDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AttachOpenGLSurface(invalid descriptor) error = %v, want ErrInvalidArgument", err)
	}
	_, err = m.AttachOpenGLOwnedTexture(OpenGLOwnedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AttachOpenGLOwnedTexture(invalid descriptor) error = %v, want ErrInvalidArgument", err)
	}
	_, err = m.AttachOpenGLBorrowedTexture(OpenGLBorrowedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AttachOpenGLBorrowedTexture(invalid descriptor) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapCloseFailsWhileRenderSessionIsLive(t *testing.T) {
	mapState, err := handle.New(&nativeMap{}, "MapHandle")
	if err != nil {
		t.Fatal(err)
	}
	m := &MapHandle{state: mapState}
	session, err := newRenderSessionHandle(m, &nativeRenderSession{})
	if err != nil {
		t.Fatalf("newRenderSessionHandle(): %v", err)
	}

	oldDestroyMap := destroyMapHandle
	oldDestroySession := destroyRenderSessionHandle
	defer func() {
		destroyMapHandle = oldDestroyMap
		destroyRenderSessionHandle = oldDestroySession
	}()
	var mapDestroyCalls atomic.Int32
	var sessionDestroyCalls atomic.Int32
	destroyMapHandle = func(*nativeMap) int32 {
		mapDestroyCalls.Add(1)
		return 0
	}
	destroyRenderSessionHandle = func(*nativeRenderSession) int32 {
		sessionDestroyCalls.Add(1)
		return 0
	}

	if err := m.Close(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("Map Close() with live render session error = %v, want ErrInvalidState", err)
	}
	if got := mapDestroyCalls.Load(); got != 0 {
		t.Fatalf("map destroy calls before session close = %d, want 0", got)
	}
	if err := session.Close(); err != nil {
		t.Fatalf("RenderSession Close(): %v", err)
	}
	if err := m.Close(); err != nil {
		t.Fatalf("Map Close() after render session close: %v", err)
	}
	if got := sessionDestroyCalls.Load(); got != 1 {
		t.Fatalf("render session destroy calls = %d, want 1", got)
	}
	if got := mapDestroyCalls.Load(); got != 1 {
		t.Fatalf("map destroy calls = %d, want 1", got)
	}
}

func newActiveFrameTestSession(t *testing.T) *RenderSessionHandle {
	t.Helper()

	mapState, err := handle.New(&nativeMap{}, "MapHandle")
	if err != nil {
		t.Fatal(err)
	}
	m := &MapHandle{state: mapState}
	session, err := newRenderSessionHandle(m, &nativeRenderSession{})
	if err != nil {
		t.Fatalf("newRenderSessionHandle(): %v", err)
	}
	session.frame = true

	oldDestroyMap := destroyMapHandle
	oldDestroySession := destroyRenderSessionHandle
	destroyMapHandle = func(*nativeMap) int32 { return 0 }
	destroyRenderSessionHandle = func(*nativeRenderSession) int32 { return 0 }
	t.Cleanup(func() {
		session.frame = false
		if err := session.Close(); err != nil {
			t.Errorf("RenderSession Close(): %v", err)
		}
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		destroyMapHandle = oldDestroyMap
		destroyRenderSessionHandle = oldDestroySession
	})

	return session
}

func TestRenderSessionOperationsRejectActiveFrame(t *testing.T) {
	session := newActiveFrameTestSession(t)

	for _, tt := range []struct {
		name string
		call func() error
	}{
		{name: "Resize", call: func() error { return session.Resize(RenderTargetExtent{Width: 1, Height: 1, ScaleFactor: 1}) }},
		{name: "RenderUpdate", call: session.RenderUpdate},
		{name: "Detach", call: session.Detach},
		{name: "ReduceMemoryUse", call: session.ReduceMemoryUse},
		{name: "ClearData", call: session.ClearData},
		{name: "DumpDebugLogs", call: session.DumpDebugLogs},
		{name: "ReadPremultipliedRGBA8", call: func() error {
			_, _, err := session.ReadPremultipliedRGBA8()
			return err
		}},
		{name: "ReadPremultipliedRGBA8Into", call: func() error {
			_, err := session.ReadPremultipliedRGBA8Into(make([]byte, 4))
			return err
		}},
		{name: "Close", call: session.Close},
	} {
		if err := tt.call(); !errors.Is(err, ErrInvalidState) {
			t.Fatalf("%s() error = %v, want ErrInvalidState", tt.name, err)
		}
	}
}
