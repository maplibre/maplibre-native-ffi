package callback

/*
#include <stdint.h>
#include "maplibre_native_c.h"
*/
import "C"

import (
	stdruntime "runtime"
	"runtime/cgo"
	"strings"
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

//export goMaplibreHttpHeaderTransform
func goMaplibreHttpHeaderTransform(userData unsafe.Pointer, kind C.uint32_t, url *C.char, outResponse *C.mln_http_header_transform_response) (status C.mln_status) {
	defer func() {
		if recover() != nil {
			status = C.mln_status(C.MLN_STATUS_NATIVE_ERROR)
		}
	}()
	if userData == nil || outResponse == nil || url == nil {
		return C.mln_status(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	outResponse.size = C.uint32_t(unsafe.Sizeof(C.mln_http_header_transform_response{}))
	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*HttpHeaderTransformState)
	if !ok || state == nil || state.callback == nil {
		return C.mln_status(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	headers := state.callback(uint32(kind), C.GoString(url))
	names := make([]string, 0, len(headers))
	for _, header := range headers {
		for _, name := range names {
			if strings.EqualFold(name, header.Name) {
				return C.mln_status(C.MLN_STATUS_INVALID_ARGUMENT)
			}
		}
		names = append(names, header.Name)
		nameBytes := []byte(header.Name)
		valueBytes := []byte(header.Value)
		var namePointer *C.char
		var valuePointer *C.char
		if len(nameBytes) > 0 {
			namePointer = (*C.char)(unsafe.Pointer(&nameBytes[0]))
		}
		if len(valueBytes) > 0 {
			valuePointer = (*C.char)(unsafe.Pointer(&valueBytes[0]))
		}
		status = C.mln_http_header_transform_response_set(
			outResponse, namePointer, C.size_t(len(nameBytes)),
			valuePointer, C.size_t(len(valueBytes)),
		)
		stdruntime.KeepAlive(nameBytes)
		stdruntime.KeepAlive(valueBytes)
		if status != C.mln_status(C.MLN_STATUS_OK) {
			return status
		}
	}
	return C.mln_status(C.MLN_STATUS_OK)
}

//export goMaplibreResourceProvider
func goMaplibreResourceProvider(userData unsafe.Pointer, request *C.mln_resource_request, rawHandle C.mln_resource_request_handle) (decision C.uint32_t) {
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

// goMaplibreResourceRequestCancel reports one cancelled provider request. The C
// API invokes it at most once per request, on the thread that discards the
// request, with no native lock held. The token resolves through a weak registry
// entry, so a request handle Go already collected is a no-op here, and the
// callback runs with no binding lock held because it may close or complete the
// same request.
//
//export goMaplibreResourceRequestCancel
func goMaplibreResourceRequestCancel(userData unsafe.Pointer) {
	handle := lookupCancelToken(uint64(uintptr(userData)))
	if handle == nil {
		return
	}
	if callback := handle.takeCancelCallback(); callback != nil {
		runCancelCallback(callback)
	}
}
