// Package maplibre provides low-level Go bindings for the MapLibre Native C
// API.
//
// Runtime, map, and projection handles are usable from any goroutine.
// Render-session and acquired-frame graphics calls retain their native thread
// affinity. OperationHandle methods are usable from any goroutine.
package maplibre

/*
#cgo pkg-config: maplibre-native-c
#include "maplibre_native_c.h"
*/
import "C"

// ExpectedCABIVersion is the C ABI contract version supported by this Go
// binding.
const ExpectedCABIVersion uint32 = 0

// CVersion returns the native C ABI contract version.
func CVersion() uint32 {
	return uint32(C.mln_c_version())
}

func checkCompatibleCABI(actualVersion uint32) error {
	if actualVersion == ExpectedCABIVersion {
		return nil
	}
	return newABIVersionMismatchError(ExpectedCABIVersion, actualVersion)
}

// SupportedRenderBackends returns the render backends compiled into the linked
// native library.
func SupportedRenderBackends() RenderBackendMask {
	return RenderBackendMask(C.mln_supported_render_backend_mask())
}

// SupportedOpenGLContextProviders returns the OpenGL context providers compiled
// into the linked native library.
func SupportedOpenGLContextProviders() OpenGLContextProviderMask {
	return OpenGLContextProviderMask(C.mln_opengl_supported_context_provider_mask())
}
