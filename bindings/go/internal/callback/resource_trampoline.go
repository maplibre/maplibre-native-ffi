package callback

/*
#include <stdint.h>
#include "maplibre_native_c.h"
*/
import "C"
import (
	"runtime/cgo"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

//export goMaplibreResourceTransform
func goMaplibreResourceTransform(userData unsafe.Pointer, kind C.uint32_t, url *C.char, outResponse *C.mln_resource_transform_response) (status C.mln_status) {
	defer func() {
		if recover() != nil {
			status = C.mln_status(capi.StatusNativeError)
		}
	}()

	if outResponse == nil || url == nil {
		return C.mln_status(capi.StatusInvalidArgument)
	}
	outResponse.size = C.uint32_t(unsafe.Sizeof(C.mln_resource_transform_response{}))
	outResponse.url = nil

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*ResourceTransformState)
	if !ok || state == nil {
		return C.mln_status(capi.StatusInvalidArgument)
	}
	pointer, invokeStatus := state.invoke(uint32(kind), C.GoString(url))
	if invokeStatus != capi.StatusOK {
		return C.mln_status(invokeStatus)
	}
	if pointer != nil {
		outResponse.url = (*C.char)(pointer)
	}
	return C.mln_status(capi.StatusOK)
}

//export goMaplibreResourceProvider
func goMaplibreResourceProvider(userData unsafe.Pointer, request *C.mln_resource_request, rawHandle *C.mln_resource_request_handle) (decision C.uint32_t) {
	defer func() {
		if recover() != nil {
			decision = C.uint32_t(capi.ResourceProviderDecisionUnknown)
		}
	}()

	handle, status := newResourceRequestHandle(rawHandle)
	if status != capi.StatusOK {
		return C.uint32_t(capi.ResourceProviderDecisionUnknown)
	}
	stateHandle := cgo.Handle(uintptr(userData))
	state, ok := stateHandle.Value().(*ResourceProviderState)
	if !ok || state == nil {
		return C.uint32_t(handle.finishProviderException())
	}
	return C.uint32_t(handle.invokeProvider(state, request))
}
