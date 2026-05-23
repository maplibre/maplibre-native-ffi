package callback

/*
#cgo CFLAGS: -std=c2x
#include <stdlib.h>
#include <stdint.h>
#include "maplibre_native_c.h"

extern mln_status goMaplibreResourceTransform(void* user_data, uint32_t kind, const char* url, mln_resource_transform_response* out_response);

static inline void* mln_go_resource_handle_to_pointer(uintptr_t handle) {
  return (void*)handle;
}
*/
import "C"
import (
	"runtime/cgo"
	"strings"
	"sync"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

// ResourceTransformCallback is the internal shape for resource URL transforms.
type ResourceTransformCallback func(kind uint32, url string) (replacementURL string, replace bool)

// ResourceTransformState owns a runtime-scoped resource transform callback.
type ResourceTransformState struct {
	callback ResourceTransformCallback
	handle   cgo.Handle

	mu      sync.Mutex
	strings []unsafe.Pointer
	once    sync.Once
}

func newResourceTransformState(callback ResourceTransformCallback) *ResourceTransformState {
	state := &ResourceTransformState{callback: callback}
	state.handle = cgo.NewHandle(state)
	return state
}

// SetResourceTransform installs or replaces a runtime-scoped resource transform.
func SetResourceTransform(runtime *capi.Runtime, callback ResourceTransformCallback) (*ResourceTransformState, capi.Status) {
	if callback == nil {
		return nil, capi.StatusInvalidArgument
	}
	state := newResourceTransformState(callback)
	descriptor := C.mln_resource_transform{
		size:      C.uint32_t(unsafe.Sizeof(C.mln_resource_transform{})),
		callback:  (C.mln_resource_transform_callback)(C.goMaplibreResourceTransform),
		user_data: C.mln_go_resource_handle_to_pointer(C.uintptr_t(state.handle)),
	}
	status := capi.Status(C.mln_runtime_set_resource_transform(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		&descriptor,
	))
	if status != capi.StatusOK {
		state.Release()
		return nil, status
	}
	return state, capi.StatusOK
}

// ClearResourceTransform clears the runtime-scoped resource transform.
func ClearResourceTransform(runtime *capi.Runtime) capi.Status {
	return capi.Status(C.mln_runtime_clear_resource_transform((*C.mln_runtime)(unsafe.Pointer(runtime))))
}

// Release frees callback state after native no longer references it.
func (state *ResourceTransformState) Release() {
	if state == nil {
		return
	}
	state.once.Do(func() {
		state.handle.Delete()
		state.mu.Lock()
		defer state.mu.Unlock()
		for _, pointer := range state.strings {
			C.free(pointer)
		}
		state.strings = nil
	})
}

func (state *ResourceTransformState) invoke(kind uint32, url string) (unsafe.Pointer, capi.Status) {
	if state == nil || state.callback == nil {
		return nil, capi.StatusInvalidArgument
	}

	replacement, replace := state.callback(kind, url)
	if !replace || replacement == "" {
		return nil, capi.StatusOK
	}
	if strings.ContainsRune(replacement, '\x00') {
		return nil, capi.StatusInvalidArgument
	}

	cString := C.CString(replacement)
	pointer := unsafe.Pointer(cString)
	state.mu.Lock()
	state.strings = append(state.strings, pointer)
	state.mu.Unlock()
	return pointer, capi.StatusOK
}

func invokeResourceTransformForTest(state *ResourceTransformState, kind uint32, url string) (string, bool, capi.Status) {
	pointer, status := state.invoke(kind, url)
	if pointer == nil || status != capi.StatusOK {
		return "", false, status
	}
	return C.GoString((*C.char)(pointer)), true, status
}
