package maplibre

import (
	"errors"
	"math"
	"testing"
)

// The binding validates a render target extent before it reaches native, so a
// scale factor that cannot describe a device-pixel size is rejected without a
// backend.
func TestRenderTargetExtentRejectsUnusableScaleFactors(t *testing.T) {
	for _, testCase := range []struct {
		name        string
		scaleFactor float64
	}{
		{name: "zero", scaleFactor: 0},
		{name: "negative", scaleFactor: -1},
		{name: "not a number", scaleFactor: math.NaN()},
		{name: "infinite", scaleFactor: math.Inf(1)},
	} {
		t.Run(testCase.name, func(t *testing.T) {
			extent := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: testCase.scaleFactor}
			if err := extent.validate(); !errors.Is(err, ErrInvalidArgument) {
				t.Fatalf("validate() error = %v, want ErrInvalidArgument", err)
			}
			if _, _, err := extent.PhysicalSize(); err == nil {
				t.Fatal("PhysicalSize() accepted an unusable scale factor")
			}
		})
	}

	extent := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 2}
	if err := extent.validate(); err != nil {
		t.Fatalf("validate() on a usable extent: %v", err)
	}
	width, height, err := extent.PhysicalSize()
	if err != nil || width != 128 || height != 64 {
		t.Fatalf("PhysicalSize() = (%d, %d, %v), want 128, 64", width, height, err)
	}
}

// An OpenGL context descriptor names exactly one platform, because the session
// takes its context from that platform's provider data.
func TestOpenGLContextDescriptorNamesExactlyOnePlatform(t *testing.T) {
	wgl := &WGLContextDescriptor{DeviceContext: NativePointer(0x10)}
	egl := &EGLContextDescriptor{Display: NativePointer(0x20)}

	for _, testCase := range []struct {
		name    string
		context OpenGLContextDescriptor
		wantErr bool
	}{
		{name: "empty", context: OpenGLContextDescriptor{}, wantErr: true},
		{name: "two platforms", context: OpenGLContextDescriptor{WGL: wgl, EGL: egl}, wantErr: true},
		{name: "wgl", context: OpenGLContextDescriptor{WGL: wgl}},
		{name: "egl", context: OpenGLContextDescriptor{EGL: egl}},
		{name: "webgl", context: OpenGLContextDescriptor{WebGL: &WebGLContextDescriptor{}}},
	} {
		t.Run(testCase.name, func(t *testing.T) {
			err := testCase.context.validate()
			if testCase.wantErr && !errors.Is(err, ErrInvalidArgument) {
				t.Fatalf("validate() error = %v, want ErrInvalidArgument", err)
			}
			if !testCase.wantErr && err != nil {
				t.Fatalf("validate() error = %v, want nil", err)
			}
		})
	}
}

// Every attach entry point validates its descriptor before it submits, so a
// bad extent is rejected on any platform and against any backend.
func TestRenderAttachRejectsAnUnusableExtent(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	bad := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 0}
	options := NewRenderSessionAttachOptions()
	attaches := map[string]func() (*RenderSessionHandle, *Future[struct{}], error){
		"metal surface": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachMetalSurface(MetalSurfaceDescriptor{Extent: bad}, options)
		},
		"vulkan surface": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachVulkanSurface(VulkanSurfaceDescriptor{Extent: bad}, options)
		},
		"opengl surface": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachOpenGLSurface(OpenGLSurfaceDescriptor{
				Extent:  bad,
				Context: OpenGLContextDescriptor{EGL: &EGLContextDescriptor{}},
			}, options)
		},
		"webgpu surface": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachWebGPUSurface(WebGPUSurfaceDescriptor{Extent: bad}, options)
		},
		"metal owned texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{Extent: bad}, options)
		},
		"metal borrowed texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachMetalBorrowedTexture(MetalBorrowedTextureDescriptor{Extent: bad}, options)
		},
		"vulkan owned texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachVulkanOwnedTexture(VulkanOwnedTextureDescriptor{Extent: bad}, options)
		},
		"vulkan borrowed texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachVulkanBorrowedTexture(VulkanBorrowedTextureDescriptor{Extent: bad}, options)
		},
		"opengl owned texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachOpenGLOwnedTexture(OpenGLOwnedTextureDescriptor{
				Extent:  bad,
				Context: OpenGLContextDescriptor{EGL: &EGLContextDescriptor{}},
			}, options)
		},
		"opengl borrowed texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachOpenGLBorrowedTexture(OpenGLBorrowedTextureDescriptor{
				Extent:  bad,
				Context: OpenGLContextDescriptor{EGL: &EGLContextDescriptor{}},
			}, options)
		},
		"webgpu owned texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachWebGPUOwnedTexture(WebGPUOwnedTextureDescriptor{Extent: bad}, options)
		},
		"webgpu borrowed texture": func() (*RenderSessionHandle, *Future[struct{}], error) {
			return m.AttachWebGPUBorrowedTexture(WebGPUBorrowedTextureDescriptor{Extent: bad}, options)
		},
	}
	for name, attach := range attaches {
		t.Run(name, func(t *testing.T) {
			session, completion, err := attach()
			if !errors.Is(err, ErrInvalidArgument) {
				t.Fatalf("attach error = %v, want ErrInvalidArgument", err)
			}
			if session != nil || completion != nil {
				t.Fatalf("rejected attach published session %v and completion %v", session, completion)
			}
		})
	}
}

// An OpenGL attach that names no platform is rejected the same way, so the
// context descriptor never reaches native half-filled.
func TestOpenGLRenderAttachRejectsAnAmbiguousContext(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	extent := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 1}
	options := NewRenderSessionAttachOptions()
	if _, _, err := m.AttachOpenGLSurface(OpenGLSurfaceDescriptor{Extent: extent}, options); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AttachOpenGLSurface(no platform) error = %v, want ErrInvalidArgument", err)
	}
	ambiguous := OpenGLContextDescriptor{
		WGL: &WGLContextDescriptor{},
		EGL: &EGLContextDescriptor{},
	}
	if _, _, err := m.AttachOpenGLOwnedTexture(
		OpenGLOwnedTextureDescriptor{Extent: extent, Context: ambiguous}, options,
	); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AttachOpenGLOwnedTexture(two platforms) error = %v, want ErrInvalidArgument", err)
	}
}

// Every render-session entry point rejects a handle that owns no session,
// rather than passing the null handle to native.
func TestRenderSessionHandleGuardsAHandleWithoutASession(t *testing.T) {
	extent := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 1}
	for _, session := range []*RenderSessionHandle{nil, {}} {
		if _, err := session.Capabilities(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Capabilities() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.Snapshot(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Snapshot() error = %v, want ErrInvalidArgument", err)
		}
		if err := session.RequestFrame(NewFrameDemand()); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("RequestFrame() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.DrainFrameResults(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("DrainFrameResults() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.AcquireFrame(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("AcquireFrame() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.ServiceDriverWork(1); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("ServiceDriverWork() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.Resize(extent); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Resize() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.Barrier(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Barrier() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.Detach(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Detach() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.ReadPremultipliedRGBA8(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("ReadPremultipliedRGBA8() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.QueryRenderedFeatures(RenderedQueryPoint(ScreenPoint{}), nil); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("QueryRenderedFeatures() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := session.Abandon(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Abandon() error = %v, want ErrInvalidArgument", err)
		}
		if err := session.Close(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Close() error = %v, want ErrInvalidArgument", err)
		}
	}
}

// Reading a frame that leases no ring slot reports an argument error for every
// backend accessor, so a released or unset frame never reaches native.
func TestAcquiredFrameGuardsAFrameWithoutALease(t *testing.T) {
	for _, frame := range []*AcquiredFrame{nil, {}} {
		if _, err := frame.Result(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Result() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := frame.ProducerSync(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("ProducerSync() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := frame.MetalTexture(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("MetalTexture() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := frame.VulkanTexture(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("VulkanTexture() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := frame.OpenGLTexture(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("OpenGLTexture() error = %v, want ErrInvalidArgument", err)
		}
		if _, err := frame.WebGPUTexture(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("WebGPUTexture() error = %v, want ErrInvalidArgument", err)
		}
		if err := frame.Release(GPUSync{Kind: GPUSyncCPUComplete}); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("Release() error = %v, want ErrInvalidArgument", err)
		}
	}
}
