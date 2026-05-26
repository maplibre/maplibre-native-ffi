// Package maplibre provides low-level Go bindings for the MapLibre Native C
// API.
package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

// CVersion returns the native C ABI contract version.
func CVersion() uint32 {
	return uint32(C.mln_c_version())
}

// SupportedRenderBackends returns the render backends compiled into the linked
// native library.
func SupportedRenderBackends() RenderBackendMask {
	return RenderBackendMask(C.mln_supported_render_backend_mask())
}
