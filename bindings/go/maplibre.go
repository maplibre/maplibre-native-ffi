// Package maplibre provides low-level Go bindings for the MapLibre Native C
// API.
package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

// CVersion returns the native C ABI contract version.
func CVersion() uint32 {
	return capi.CVersion()
}

// SupportedRenderBackends returns the render backends compiled into the linked
// native library.
func SupportedRenderBackends() RenderBackendMask {
	return RenderBackendMask(capi.SupportedRenderBackendMask())
}
