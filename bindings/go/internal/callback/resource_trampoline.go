package callback

/*
#include <stdint.h>
#include "maplibre_native_c.h"
*/
import "C"
import (
	stdruntime "runtime"
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

	if userData == nil || outResponse == nil || url == nil {
		return C.mln_status(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	outResponse.size = C.uint32_t(unsafe.Sizeof(C.mln_resource_transform_response{}))
	outResponse.url = nil

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*ResourceTransformState)
	if !ok || state == nil {
		return C.mln_status(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	replacement, replace, invokeStatus := state.invoke(uint32(kind), C.GoString(url))
	if invokeStatus != int32(C.MLN_STATUS_OK) {
		return C.mln_status(invokeStatus)
	}
	if !replace || replacement == "" {
		return C.mln_status(C.MLN_STATUS_OK)
	}

	bytes := []byte(replacement)
	status = C.mln_resource_transform_response_set_url(
		outResponse,
		(*C.char)(unsafe.Pointer(&bytes[0])),
		C.size_t(len(bytes)),
	)
	stdruntime.KeepAlive(bytes)
	return status
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
