package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

// RenderBackendMask preserves the render backend bits reported by the native
// library. Unknown future bits remain in the mask.
type RenderBackendMask uint32

const (
	RenderBackendMetal  RenderBackendMask = RenderBackendMask(capi.RenderBackendFlagMetal)
	RenderBackendVulkan RenderBackendMask = RenderBackendMask(capi.RenderBackendFlagVulkan)
)

// Has reports whether all backend bits in backend are present.
func (mask RenderBackendMask) Has(backend RenderBackendMask) bool {
	return mask&backend == backend
}

// NativePointer is a borrowed opaque backend-native address.
//
// It grants no memory access and transfers no ownership. The binding converts
// it to unsafe.Pointer only at cgo boundaries for C APIs that accept opaque
// backend handles.
type NativePointer uintptr
