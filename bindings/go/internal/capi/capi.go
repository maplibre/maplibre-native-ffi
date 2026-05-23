// Package capi contains the private cgo boundary for the public MapLibre
// Native C API.
package capi

/*
#cgo CFLAGS: -std=c2x
#include "maplibre_native_c.h"
*/
import "C"

// Status is the raw C status value returned by fallible C API calls.
type Status int32

const (
	StatusOK              Status = Status(C.MLN_STATUS_OK)
	StatusInvalidArgument Status = Status(C.MLN_STATUS_INVALID_ARGUMENT)
	StatusInvalidState    Status = Status(C.MLN_STATUS_INVALID_STATE)
	StatusWrongThread     Status = Status(C.MLN_STATUS_WRONG_THREAD)
	StatusUnsupported     Status = Status(C.MLN_STATUS_UNSUPPORTED)
	StatusNativeError     Status = Status(C.MLN_STATUS_NATIVE_ERROR)
)

const (
	RenderBackendFlagMetal  uint32 = uint32(C.MLN_RENDER_BACKEND_FLAG_METAL)
	RenderBackendFlagVulkan uint32 = uint32(C.MLN_RENDER_BACKEND_FLAG_VULKAN)
)

const (
	NetworkStatusOnline  uint32 = uint32(C.MLN_NETWORK_STATUS_ONLINE)
	NetworkStatusOffline uint32 = uint32(C.MLN_NETWORK_STATUS_OFFLINE)
)

// CVersion returns the linked native C ABI contract version.
func CVersion() uint32 {
	return uint32(C.mln_c_version())
}

// SupportedRenderBackendMask returns the raw backend support mask.
func SupportedRenderBackendMask() uint32 {
	return uint32(C.mln_supported_render_backend_mask())
}

// NetworkStatusGet reads MapLibre Native's process-global network status.
func NetworkStatusGet(out *uint32) Status {
	var raw C.uint32_t
	status := Status(C.mln_network_status_get(&raw))
	if status == StatusOK {
		*out = uint32(raw)
	}
	return status
}

// NetworkStatusSet sets MapLibre Native's process-global network status.
func NetworkStatusSet(status uint32) Status {
	return Status(C.mln_network_status_set(C.uint32_t(status)))
}

// ThreadLastErrorMessage copies the current thread-local C diagnostic.
func ThreadLastErrorMessage() string {
	return C.GoString(C.mln_thread_last_error_message())
}
