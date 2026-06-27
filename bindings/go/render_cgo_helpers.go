package maplibre

/*
#include "maplibre_native_c.h"
#include "internal/cgo_shim.h"
*/
import "C"

import "unsafe"

// Descriptor inspection helpers live outside _test.go because Go test files
// cannot import C directly. They stay unexported and support ABI materialization
// tests for cgo-only struct and union fields.
type vulkanContextDescriptorFields struct {
	Size                     uint32
	Instance                 uintptr
	PhysicalDevice           uintptr
	Device                   uintptr
	GraphicsQueue            uintptr
	GraphicsQueueFamilyIndex uint32
	GetInstanceProcAddr      uintptr
	GetDeviceProcAddr        uintptr
}

type wglContextDescriptorFields struct {
	OpenGLSize     uint32
	Platform       uint32
	WGLSize        uint32
	DeviceContext  uintptr
	ShareContext   uintptr
	GetProcAddress uintptr
}

type eglContextDescriptorFields struct {
	OpenGLSize     uint32
	Platform       uint32
	EGLSize        uint32
	Display        uintptr
	Config         uintptr
	ShareContext   uintptr
	GetProcAddress uintptr
}

func testVulkanContextDescriptorFields(descriptor VulkanContextDescriptor) vulkanContextDescriptorFields {
	raw := descriptor.toC()
	return vulkanContextDescriptorFields{
		Size:                     uint32(raw.size),
		Instance:                 uintptr(raw.instance),
		PhysicalDevice:           uintptr(raw.physical_device),
		Device:                   uintptr(raw.device),
		GraphicsQueue:            uintptr(raw.graphics_queue),
		GraphicsQueueFamilyIndex: uint32(raw.graphics_queue_family_index),
		GetInstanceProcAddr:      uintptr(raw.get_instance_proc_addr),
		GetDeviceProcAddr:        uintptr(raw.get_device_proc_addr),
	}
}

func testVulkanContextDescriptorSize() uintptr {
	return unsafe.Sizeof(C.mln_vulkan_context_descriptor{})
}

func testWGLContextDescriptorFields(descriptor OpenGLContextDescriptor) wglContextDescriptorFields {
	raw := descriptor.toC()
	wgl := C.mln_go_opengl_context_wgl(&raw)
	return wglContextDescriptorFields{
		OpenGLSize:     uint32(raw.size),
		Platform:       uint32(raw.platform),
		WGLSize:        uint32(wgl.size),
		DeviceContext:  uintptr(wgl.device_context),
		ShareContext:   uintptr(wgl.share_context),
		GetProcAddress: uintptr(wgl.get_proc_address),
	}
}

func testEGLContextDescriptorFields(descriptor OpenGLContextDescriptor) eglContextDescriptorFields {
	raw := descriptor.toC()
	egl := C.mln_go_opengl_context_egl(&raw)
	return eglContextDescriptorFields{
		OpenGLSize:     uint32(raw.size),
		Platform:       uint32(raw.platform),
		EGLSize:        uint32(egl.size),
		Display:        uintptr(egl.display),
		Config:         uintptr(egl.config),
		ShareContext:   uintptr(egl.share_context),
		GetProcAddress: uintptr(egl.get_proc_address),
	}
}

func testOpenGLContextDescriptorSize() uintptr {
	return unsafe.Sizeof(C.mln_opengl_context_descriptor{})
}

func testWGLContextDescriptorSize() uintptr {
	return unsafe.Sizeof(C.mln_wgl_context_descriptor{})
}

func testEGLContextDescriptorSize() uintptr {
	return unsafe.Sizeof(C.mln_egl_context_descriptor{})
}

func testOpenGLContextPlatformWGL() uint32 {
	return uint32(C.MLN_OPENGL_CONTEXT_PLATFORM_WGL)
}

func testOpenGLContextPlatformEGL() uint32 {
	return uint32(C.MLN_OPENGL_CONTEXT_PLATFORM_EGL)
}
