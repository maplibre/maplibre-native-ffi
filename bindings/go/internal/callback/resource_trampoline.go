package callback

/*
#include <stdint.h>
#include "maplibre_native_c.h"
*/
import "C"
import (
	"runtime/cgo"
	"unsafe"
)

//export goMaplibreResourceTransform
func goMaplibreResourceTransform(userData unsafe.Pointer, kind C.uint32_t, url *C.char, outResponse *C.mln_resource_transform_response) (status C.mln_status) {
	defer func() {
		if recover() != nil {
			status = C.mln_status(C.MLN_STATUS_NATIVE_ERROR)
		}
	}()

	if outResponse == nil || url == nil {
		return C.mln_status(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	outResponse.size = C.uint32_t(unsafe.Sizeof(C.mln_resource_transform_response{}))
	outResponse.url = nil

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*ResourceTransformState)
	if !ok || state == nil {
		return C.mln_status(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	pointer, invokeStatus := state.invoke(uint32(kind), C.GoString(url))
	if invokeStatus != int32(C.MLN_STATUS_OK) {
		return C.mln_status(invokeStatus)
	}
	if pointer != nil {
		outResponse.url = (*C.char)(pointer)
	}
	return C.mln_status(int32(C.MLN_STATUS_OK))
}

//export goMaplibreResourceProvider
func goMaplibreResourceProvider(userData unsafe.Pointer, request *C.mln_resource_request, rawHandle *C.mln_resource_request_handle) (decision C.uint32_t) {
	defer func() {
		if recover() != nil {
			decision = C.uint32_t(^uint32(0))
		}
	}()

	handle, status := newResourceRequestHandle(rawHandle)
	if status != int32(C.MLN_STATUS_OK) {
		return C.uint32_t(^uint32(0))
	}
	stateHandle := cgo.Handle(uintptr(userData))
	state, ok := stateHandle.Value().(*ResourceProviderState)
	if !ok || state == nil {
		return C.uint32_t(handle.finishProviderException())
	}
	return C.uint32_t(handle.invokeProvider(state, request))
}
