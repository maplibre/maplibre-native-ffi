//go:build darwin && cgo

package testsupport

// The render tests run on darwin, so only darwin links tests/graphics. A target
// whose runner pushes test binaries to a device, such as Android, also has to
// push libmln_test_graphics beside them before this file builds there.

/*
#cgo pkg-config: mln-test-graphics
#include <mln_test_graphics.h>
*/
import "C"

import (
	"errors"
	"unsafe"
)

// Backend names a graphics API that tests/graphics drives.
type Backend uint32

const (
	BackendMetal  Backend = C.MLN_TEST_GRAPHICS_BACKEND_METAL
	BackendVulkan Backend = C.MLN_TEST_GRAPHICS_BACKEND_VULKAN
	BackendEGL    Backend = C.MLN_TEST_GRAPHICS_BACKEND_EGL
)

// Context holds the handles that a backend's context descriptor takes. Only
// the fields of the graphics object's backend are set.
type Context struct {
	Backend                   Backend
	MetalDevice               uintptr
	VulkanInstance            uintptr
	VulkanPhysicalDevice      uintptr
	VulkanDevice              uintptr
	VulkanQueue               uintptr
	VulkanQueueFamilyIndex    uint32
	VulkanGetInstanceProcAddr uintptr
	VulkanGetDeviceProcAddr   uintptr
	EGLDisplay                uintptr
	EGLConfig                 uintptr
	EGLContext                uintptr
}

// Graphics is a device or context that stands in for the host's. The handles
// in Context stay valid until Close.
type Graphics struct {
	handle  *C.mln_test_graphics
	Context Context
}

func lastError() error {
	return errors.New(C.GoString(C.mln_test_graphics_last_error()))
}

// NewGraphics creates a device or context for backend.
func NewGraphics(backend Backend) (*Graphics, error) {
	handle := C.mln_test_graphics_create(C.uint32_t(backend))
	if handle == nil {
		return nil, lastError()
	}
	var context C.mln_test_graphics_context
	if !C.mln_test_graphics_get_context(handle, &context) {
		err := lastError()
		C.mln_test_graphics_destroy(handle)
		return nil, err
	}
	return &Graphics{
		handle: handle,
		Context: Context{
			Backend:                   Backend(context.backend),
			MetalDevice:               uintptr(unsafe.Pointer(context.metal_device)),
			VulkanInstance:            uintptr(unsafe.Pointer(context.vulkan_instance)),
			VulkanPhysicalDevice:      uintptr(unsafe.Pointer(context.vulkan_physical_device)),
			VulkanDevice:              uintptr(unsafe.Pointer(context.vulkan_device)),
			VulkanQueue:               uintptr(unsafe.Pointer(context.vulkan_queue)),
			VulkanQueueFamilyIndex:    uint32(context.vulkan_queue_family_index),
			VulkanGetInstanceProcAddr: uintptr(unsafe.Pointer(context.vulkan_get_instance_proc_addr)),
			VulkanGetDeviceProcAddr:   uintptr(unsafe.Pointer(context.vulkan_get_device_proc_addr)),
			EGLDisplay:                uintptr(unsafe.Pointer(context.egl_display)),
			EGLConfig:                 uintptr(unsafe.Pointer(context.egl_config)),
			EGLContext:                uintptr(unsafe.Pointer(context.egl_context)),
		},
	}, nil
}

// MakeCurrent makes an EGL context current on the calling OS thread, which the
// caller locks for as long as it drives sessions there.
func (g *Graphics) MakeCurrent() error {
	if !C.mln_test_graphics_make_current(g.handle) {
		return lastError()
	}
	return nil
}

// Close destroys the device or context. Detach every session that borrows it
// first. A second call does nothing.
func (g *Graphics) Close() {
	if g.handle != nil {
		C.mln_test_graphics_destroy(g.handle)
		g.handle = nil
	}
}
