package maplibre

import (
	"errors"
	"math"
	"testing"
)

func TestRenderBackendAndOpenGLProviderMasks(t *testing.T) {
	backends := RenderBackendMetal | RenderBackendVulkan | RenderBackendOpenGL | RenderBackendMask(1<<30)
	if !backends.Has(RenderBackendOpenGL) {
		t.Fatalf("backend mask %08x should include OpenGL", uint32(backends))
	}
	if !backends.Has(RenderBackendMask(1 << 30)) {
		t.Fatalf("backend mask should preserve unknown future bits")
	}

	providers := OpenGLContextProviderWGL | OpenGLContextProviderEGL | OpenGLContextProviderMask(1<<30)
	if !providers.Has(OpenGLContextProviderWGL) || !providers.Has(OpenGLContextProviderEGL) {
		t.Fatalf("provider mask %08x should include WGL and EGL", uint32(providers))
	}
	if !providers.Has(OpenGLContextProviderMask(1 << 30)) {
		t.Fatalf("provider mask should preserve unknown future bits")
	}
}

func TestVulkanContextDescriptorMaterialization(t *testing.T) {
	descriptor := VulkanContextDescriptor{
		Instance:                 0x101,
		PhysicalDevice:           0x202,
		Device:                   0x303,
		GraphicsQueue:            0x404,
		GraphicsQueueFamilyIndex: 7,
		GetInstanceProcAddr:      0x505,
		GetDeviceProcAddr:        0x606,
	}

	fields := testVulkanContextDescriptorFields(descriptor)
	if fields.Size != uint32(testVulkanContextDescriptorSize()) {
		t.Fatalf("size = %d, want %d", fields.Size, testVulkanContextDescriptorSize())
	}
	if fields.Instance != uintptr(descriptor.Instance) ||
		fields.PhysicalDevice != uintptr(descriptor.PhysicalDevice) ||
		fields.Device != uintptr(descriptor.Device) ||
		fields.GraphicsQueue != uintptr(descriptor.GraphicsQueue) ||
		fields.GraphicsQueueFamilyIndex != descriptor.GraphicsQueueFamilyIndex ||
		fields.GetInstanceProcAddr != uintptr(descriptor.GetInstanceProcAddr) ||
		fields.GetDeviceProcAddr != uintptr(descriptor.GetDeviceProcAddr) {
		t.Fatalf("Vulkan context descriptor fields = %#v", fields)
	}
}

func TestOpenGLContextDescriptorMaterialization(t *testing.T) {
	wglDescriptor := OpenGLContextDescriptor{WGL: &WGLContextDescriptor{
		DeviceContext:  0x111,
		ShareContext:   0x222,
		GetProcAddress: 0x333,
	}}
	wgl := testWGLContextDescriptorFields(wglDescriptor)
	if wgl.OpenGLSize != uint32(testOpenGLContextDescriptorSize()) {
		t.Fatalf("OpenGL size = %d, want %d", wgl.OpenGLSize, testOpenGLContextDescriptorSize())
	}
	if wgl.Platform != testOpenGLContextPlatformWGL() {
		t.Fatalf("WGL platform = %d, want %d", wgl.Platform, testOpenGLContextPlatformWGL())
	}
	if wgl.WGLSize != uint32(testWGLContextDescriptorSize()) ||
		wgl.DeviceContext != uintptr(wglDescriptor.WGL.DeviceContext) ||
		wgl.ShareContext != uintptr(wglDescriptor.WGL.ShareContext) ||
		wgl.GetProcAddress != uintptr(wglDescriptor.WGL.GetProcAddress) {
		t.Fatalf("WGL context descriptor fields = %#v", wgl)
	}

	eglDescriptor := OpenGLContextDescriptor{EGL: &EGLContextDescriptor{
		Display:        0x444,
		Config:         0x555,
		ShareContext:   0x666,
		GetProcAddress: 0x777,
	}}
	egl := testEGLContextDescriptorFields(eglDescriptor)
	if egl.OpenGLSize != uint32(testOpenGLContextDescriptorSize()) {
		t.Fatalf("OpenGL size = %d, want %d", egl.OpenGLSize, testOpenGLContextDescriptorSize())
	}
	if egl.Platform != testOpenGLContextPlatformEGL() {
		t.Fatalf("EGL platform = %d, want %d", egl.Platform, testOpenGLContextPlatformEGL())
	}
	if egl.EGLSize != uint32(testEGLContextDescriptorSize()) ||
		egl.Display != uintptr(eglDescriptor.EGL.Display) ||
		egl.Config != uintptr(eglDescriptor.EGL.Config) ||
		egl.ShareContext != uintptr(eglDescriptor.EGL.ShareContext) ||
		egl.GetProcAddress != uintptr(eglDescriptor.EGL.GetProcAddress) {
		t.Fatalf("EGL context descriptor fields = %#v", egl)
	}
}

func TestRenderDescriptorValidation(t *testing.T) {
	for _, extent := range []RenderTargetExtent{
		{Width: 1, Height: 1, ScaleFactor: 0},
		{Width: 1, Height: 1, ScaleFactor: -1},
		{Width: 1, Height: 1, ScaleFactor: math.NaN()},
		{Width: 1, Height: 1, ScaleFactor: math.Inf(1)},
	} {
		if err := extent.validate(); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("extent.validate() error = %v, want ErrInvalidArgument", err)
		}
	}
	if err := (RenderTargetExtent{Width: 1, Height: 1, ScaleFactor: 1}).validate(); err != nil {
		t.Fatalf("valid extent.validate() error = %v", err)
	}

	if err := (OpenGLContextDescriptor{}).validate(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("empty OpenGL context validate error = %v, want ErrInvalidArgument", err)
	}
	if err := (OpenGLContextDescriptor{WGL: &WGLContextDescriptor{}, EGL: &EGLContextDescriptor{}}).validate(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ambiguous OpenGL context validate error = %v, want ErrInvalidArgument", err)
	}
	if err := (OpenGLContextDescriptor{WGL: &WGLContextDescriptor{}}).validate(); err != nil {
		t.Fatalf("valid WGL context validate error = %v", err)
	}
	if err := (OpenGLContextDescriptor{EGL: &EGLContextDescriptor{}}).validate(); err != nil {
		t.Fatalf("valid EGL context validate error = %v", err)
	}
}

func TestOpenGLFrameNilClose(t *testing.T) {
	var nilFrame *OpenGLOwnedTextureFrame
	if err := nilFrame.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil OpenGLOwnedTextureFrame Close() error = %v, want ErrInvalidArgument", err)
	}
}
