package callback

/*
#cgo CFLAGS: -std=c2x
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "maplibre_native_c.h"

extern mln_status goMaplibreResourceTransform(void* user_data, uint32_t kind, const char* url, mln_resource_transform_response* out_response);
extern uint32_t goMaplibreResourceProvider(void* user_data, const mln_resource_request* request, mln_resource_request_handle* handle);

static inline void* mln_go_resource_handle_to_pointer(uintptr_t handle) {
  return (void*)handle;
}

// Native copies out_response->url immediately after the callback returns. The
// C bridge keeps one replacement URL per callback thread so Go can free its
// call-scoped C string before returning to native without exposing a dangling
// pointer to the native copy step.
static _Thread_local char* mln_go_resource_transform_thread_url;

static inline char* mln_go_copy_c_string(const char* value) {
  if (value == NULL) {
    return NULL;
  }
  size_t size = strlen(value) + 1;
  char* copy = (char*)malloc(size);
  if (copy != NULL) {
    memcpy(copy, value, size);
  }
  return copy;
}

static mln_status mln_go_resource_transform_callback(void* user_data, uint32_t kind, const char* url, mln_resource_transform_response* out_response) {
  free(mln_go_resource_transform_thread_url);
  mln_go_resource_transform_thread_url = NULL;

  if (out_response == NULL) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_response->size = sizeof(mln_resource_transform_response);
  out_response->url = NULL;

  mln_resource_transform_response temporary = {
    .size = sizeof(mln_resource_transform_response), .url = NULL
  };
  mln_status status = goMaplibreResourceTransform(user_data, kind, url, &temporary);
  if (status != MLN_STATUS_OK || temporary.url == NULL || temporary.url[0] == '\0') {
    free((void*)temporary.url);
    return status;
  }

  char* copied = mln_go_copy_c_string(temporary.url);
  free((void*)temporary.url);
  if (copied == NULL) {
    return MLN_STATUS_NATIVE_ERROR;
  }

  mln_go_resource_transform_thread_url = copied;
  out_response->url = mln_go_resource_transform_thread_url;
  return MLN_STATUS_OK;
}

static inline mln_resource_transform_callback mln_go_resource_transform_callback_pointer(void) {
  return (mln_resource_transform_callback)mln_go_resource_transform_callback;
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

	once sync.Once
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
		callback:  C.mln_go_resource_transform_callback_pointer(),
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
	return unsafe.Pointer(cString), capi.StatusOK
}

func invokeResourceTransformForTest(state *ResourceTransformState, kind uint32, url string) (string, bool, capi.Status) {
	pointer, status := state.invoke(kind, url)
	if pointer == nil || status != capi.StatusOK {
		return "", false, status
	}
	defer C.free(pointer)
	return C.GoString((*C.char)(pointer)), true, status
}

func invokeResourceTransformTrampolineForTest(state *ResourceTransformState, kind uint32, url string) capi.Status {
	_, _, status := invokeResourceTransformTrampolineReplacementForTest(state, kind, url)
	return status
}

func invokeResourceTransformTrampolineReplacementForTest(state *ResourceTransformState, kind uint32, url string) (string, bool, capi.Status) {
	rawURL := C.CString(url)
	defer C.free(unsafe.Pointer(rawURL))
	var response C.mln_resource_transform_response
	status := capi.Status(C.mln_go_resource_transform_callback(
		C.mln_go_resource_handle_to_pointer(C.uintptr_t(state.handle)),
		C.uint32_t(kind),
		rawURL,
		&response,
	))
	if response.url == nil || status != capi.StatusOK {
		return "", false, status
	}
	return C.GoString(response.url), true, status
}

// ResourceRequest is the copied internal shape for provider callbacks.
type ResourceRequest struct {
	URL                 string
	Kind                uint32
	LoadingMethod       uint32
	Priority            uint32
	Usage               uint32
	StoragePolicy       uint32
	HasRange            bool
	RangeStart          uint64
	RangeEnd            uint64
	HasPriorModified    bool
	PriorModifiedUnixMS int64
	HasPriorExpires     bool
	PriorExpiresUnixMS  int64
	PriorETag           string
	PriorData           []byte
}

// ResourceResponse is the internal response shape copied during completion.
type ResourceResponse struct {
	Status           uint32
	ErrorReason      uint32
	Bytes            []byte
	ErrorMessage     string
	MustRevalidate   bool
	HasModified      bool
	ModifiedUnixMS   int64
	HasExpires       bool
	ExpiresUnixMS    int64
	ETag             string
	HasRetryAfter    bool
	RetryAfterUnixMS int64
}

// ResourceProviderCallback is the internal shape for resource providers.
type ResourceProviderCallback func(ResourceRequest, *ResourceRequestHandle) uint32

// ResourceProviderState owns a runtime-scoped resource provider callback.
type ResourceProviderState struct {
	callback ResourceProviderCallback
	handle   cgo.Handle
	once     sync.Once
}

// ResourceRequestHandle owns a provider request handle selected for handling.
type ResourceRequestHandle struct {
	mu                sync.Mutex
	handle            *C.mln_resource_request_handle
	decisionFinalized bool
	providerOwned     bool
	releaseAccounted  bool
	closed            bool
	completed         bool
}

// SetResourceProvider installs or replaces a runtime-scoped resource provider.
func SetResourceProvider(runtime *capi.Runtime, callback ResourceProviderCallback) (*ResourceProviderState, capi.Status) {
	if callback == nil {
		return nil, capi.StatusInvalidArgument
	}
	state := &ResourceProviderState{callback: callback}
	state.handle = cgo.NewHandle(state)
	descriptor := C.mln_resource_provider{
		size:      C.uint32_t(unsafe.Sizeof(C.mln_resource_provider{})),
		callback:  (C.mln_resource_provider_callback)(C.goMaplibreResourceProvider),
		user_data: C.mln_go_resource_handle_to_pointer(C.uintptr_t(state.handle)),
	}
	status := capi.Status(C.mln_runtime_set_resource_provider(
		(*C.mln_runtime)(unsafe.Pointer(runtime)),
		&descriptor,
	))
	if status != capi.StatusOK {
		state.Release()
		return nil, status
	}
	return state, capi.StatusOK
}

// Release frees provider callback state after native no longer references it.
func (state *ResourceProviderState) Release() {
	if state == nil {
		return
	}
	state.once.Do(func() {
		state.handle.Delete()
	})
}

func newResourceRequestHandle(handle *C.mln_resource_request_handle) (*ResourceRequestHandle, capi.Status) {
	if handle == nil {
		return nil, capi.StatusInvalidArgument
	}
	return &ResourceRequestHandle{handle: handle}, capi.StatusOK
}

// Complete completes the request with copied response data.
func (handle *ResourceRequestHandle) Complete(response ResourceResponse) capi.Status {
	raw, allocations := resourceResponseToC(response)
	defer freeAllocations(allocations)

	handle.mu.Lock()
	defer handle.mu.Unlock()
	if handle.completed {
		return capi.StatusInvalidState
	}
	if handle.closed {
		return capi.StatusInvalidArgument
	}
	status := capi.Status(C.mln_resource_request_complete(handle.handle, &raw))
	if status != capi.StatusOK {
		return status
	}
	handle.completed = true
	handle.closed = true
	if handle.decisionFinalized && handle.providerOwned {
		handle.releaseIfOwnedLocked()
	}
	return capi.StatusOK
}

// Cancelled reports whether native cancelled the request.
func (handle *ResourceRequestHandle) Cancelled() (capi.Status, bool) {
	handle.mu.Lock()
	defer handle.mu.Unlock()
	if handle.closed {
		return capi.StatusInvalidArgument, false
	}
	var cancelled C.bool
	status := capi.Status(C.mln_resource_request_cancelled(handle.handle, &cancelled))
	return status, bool(cancelled)
}

// Close releases the provider-owned handle without completing it.
func (handle *ResourceRequestHandle) Close() {
	if handle == nil {
		return
	}
	handle.mu.Lock()
	defer handle.mu.Unlock()
	if handle.closed {
		return
	}
	handle.closed = true
	if handle.decisionFinalized && handle.providerOwned {
		handle.releaseIfOwnedLocked()
	}
}

func (handle *ResourceRequestHandle) finishProviderDecision(decision uint32) uint32 {
	handle.mu.Lock()
	defer handle.mu.Unlock()
	if handle.decisionFinalized {
		if handle.providerOwned {
			return capi.ResourceProviderDecisionHandle
		}
		return capi.ResourceProviderDecisionPassThrough
	}
	if handle.completed || decision == capi.ResourceProviderDecisionHandle {
		handle.decisionFinalized = true
		handle.providerOwned = true
		if handle.closed {
			handle.releaseIfOwnedLocked()
		}
		return capi.ResourceProviderDecisionHandle
	}
	handle.decisionFinalized = true
	handle.releaseAccounted = true
	handle.closed = true
	return capi.ResourceProviderDecisionPassThrough
}

func (handle *ResourceRequestHandle) finishProviderException() uint32 {
	handle.mu.Lock()
	completed := handle.completed
	if !completed {
		handle.decisionFinalized = true
		handle.releaseAccounted = true
		handle.closed = true
	}
	handle.mu.Unlock()
	if completed {
		return handle.finishProviderDecision(capi.ResourceProviderDecisionHandle)
	}
	return capi.ResourceProviderDecisionUnknown
}

func (handle *ResourceRequestHandle) releaseIfOwnedLocked() {
	if handle.releaseAccounted {
		return
	}
	handle.releaseAccounted = true
	C.mln_resource_request_release(handle.handle)
}

func (handle *ResourceRequestHandle) invokeProvider(state *ResourceProviderState, request *C.mln_resource_request) (decision uint32) {
	defer func() {
		if recover() != nil {
			decision = handle.finishProviderException()
		}
	}()
	if state == nil || state.callback == nil || request == nil {
		return handle.finishProviderException()
	}
	copied := resourceRequestFromC(request)
	return handle.finishProviderDecision(state.callback(copied, handle))
}

func resourceRequestFromC(request *C.mln_resource_request) ResourceRequest {
	copied := ResourceRequest{
		URL:                 C.GoString(request.url),
		Kind:                uint32(request.kind),
		LoadingMethod:       uint32(request.loading_method),
		Priority:            uint32(request.priority),
		Usage:               uint32(request.usage),
		StoragePolicy:       uint32(request.storage_policy),
		HasRange:            bool(request.has_range),
		RangeStart:          uint64(request.range_start),
		RangeEnd:            uint64(request.range_end),
		HasPriorModified:    bool(request.has_prior_modified),
		PriorModifiedUnixMS: int64(request.prior_modified_unix_ms),
		HasPriorExpires:     bool(request.has_prior_expires),
		PriorExpiresUnixMS:  int64(request.prior_expires_unix_ms),
		PriorETag:           C.GoString(request.prior_etag),
	}
	if request.prior_data != nil && request.prior_data_size > 0 {
		copied.PriorData = C.GoBytes(unsafe.Pointer(request.prior_data), C.int(request.prior_data_size))
	}
	return copied
}

func invokeResourceProviderTrampolineForTest(state *ResourceProviderState) uint32 {
	rawURL := C.CString("https://example.com/style.json")
	defer C.free(unsafe.Pointer(rawURL))
	rawHandle := C.malloc(1)
	defer C.free(rawHandle)
	rawRequest := C.mln_resource_request{
		size: C.uint32_t(unsafe.Sizeof(C.mln_resource_request{})),
		url:  rawURL,
		kind: C.uint32_t(capi.ResourceKindStyle),
	}
	return uint32(goMaplibreResourceProvider(
		C.mln_go_resource_handle_to_pointer(C.uintptr_t(state.handle)),
		&rawRequest,
		(*C.mln_resource_request_handle)(rawHandle),
	))
}

func resourceResponseToC(response ResourceResponse) (C.mln_resource_response, []unsafe.Pointer) {
	allocations := make([]unsafe.Pointer, 0, 4)
	raw := C.mln_resource_response{
		size:                C.uint32_t(unsafe.Sizeof(C.mln_resource_response{})),
		status:              C.uint32_t(response.Status),
		error_reason:        C.uint32_t(response.ErrorReason),
		must_revalidate:     C.bool(response.MustRevalidate),
		has_modified:        C.bool(response.HasModified),
		modified_unix_ms:    C.int64_t(response.ModifiedUnixMS),
		has_expires:         C.bool(response.HasExpires),
		expires_unix_ms:     C.int64_t(response.ExpiresUnixMS),
		has_retry_after:     C.bool(response.HasRetryAfter),
		retry_after_unix_ms: C.int64_t(response.RetryAfterUnixMS),
	}
	if len(response.Bytes) > 0 {
		bytes := C.CBytes(response.Bytes)
		allocations = append(allocations, bytes)
		raw.bytes = (*C.uint8_t)(bytes)
		raw.byte_count = C.size_t(len(response.Bytes))
	}
	if response.ErrorMessage != "" {
		message := C.CString(response.ErrorMessage)
		allocations = append(allocations, unsafe.Pointer(message))
		raw.error_message = message
	}
	if response.ETag != "" {
		etag := C.CString(response.ETag)
		allocations = append(allocations, unsafe.Pointer(etag))
		raw.etag = etag
	}
	return raw, allocations
}

func freeAllocations(allocations []unsafe.Pointer) {
	for _, allocation := range allocations {
		C.free(allocation)
	}
}
