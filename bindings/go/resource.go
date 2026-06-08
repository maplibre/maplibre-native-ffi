package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	stdruntime "runtime"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
)

// ResourceKind identifies the native resource category for a request.
type ResourceKind uint32

const (
	ResourceKindUnknown     ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_UNKNOWN)
	ResourceKindStyle       ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_STYLE)
	ResourceKindSource      ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_SOURCE)
	ResourceKindTile        ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_TILE)
	ResourceKindGlyphs      ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_GLYPHS)
	ResourceKindSpriteImage ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_SPRITE_IMAGE)
	ResourceKindSpriteJSON  ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_SPRITE_JSON)
	ResourceKindImage       ResourceKind = ResourceKind(C.MLN_RESOURCE_KIND_IMAGE)
)

// ResourceLoadingMethod identifies native cache/network loading policy.
type ResourceLoadingMethod uint32

const (
	ResourceLoadingMethodAll         ResourceLoadingMethod = ResourceLoadingMethod(C.MLN_RESOURCE_LOADING_METHOD_ALL)
	ResourceLoadingMethodCacheOnly   ResourceLoadingMethod = ResourceLoadingMethod(C.MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY)
	ResourceLoadingMethodNetworkOnly ResourceLoadingMethod = ResourceLoadingMethod(C.MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY)
)

// ResourcePriority identifies native request priority.
type ResourcePriority uint32

const (
	ResourcePriorityRegular ResourcePriority = ResourcePriority(C.MLN_RESOURCE_PRIORITY_REGULAR)
	ResourcePriorityLow     ResourcePriority = ResourcePriority(C.MLN_RESOURCE_PRIORITY_LOW)
)

// ResourceUsage identifies online or offline request use.
type ResourceUsage uint32

const (
	ResourceUsageOnline  ResourceUsage = ResourceUsage(C.MLN_RESOURCE_USAGE_ONLINE)
	ResourceUsageOffline ResourceUsage = ResourceUsage(C.MLN_RESOURCE_USAGE_OFFLINE)
)

// ResourceStoragePolicy identifies native cache persistence policy.
type ResourceStoragePolicy uint32

const (
	ResourceStoragePolicyPermanent ResourceStoragePolicy = ResourceStoragePolicy(C.MLN_RESOURCE_STORAGE_POLICY_PERMANENT)
	ResourceStoragePolicyVolatile  ResourceStoragePolicy = ResourceStoragePolicy(C.MLN_RESOURCE_STORAGE_POLICY_VOLATILE)
)

// ResourceResponseStatus identifies provider response status.
type ResourceResponseStatus uint32

const (
	ResourceResponseStatusOK          ResourceResponseStatus = ResourceResponseStatus(C.MLN_RESOURCE_RESPONSE_STATUS_OK)
	ResourceResponseStatusError       ResourceResponseStatus = ResourceResponseStatus(C.MLN_RESOURCE_RESPONSE_STATUS_ERROR)
	ResourceResponseStatusNoContent   ResourceResponseStatus = ResourceResponseStatus(C.MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT)
	ResourceResponseStatusNotModified ResourceResponseStatus = ResourceResponseStatus(C.MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED)
)

// ResourceErrorReason identifies provider error categories.
type ResourceErrorReason uint32

const (
	ResourceErrorReasonNone       ResourceErrorReason = ResourceErrorReason(C.MLN_RESOURCE_ERROR_REASON_NONE)
	ResourceErrorReasonNotFound   ResourceErrorReason = ResourceErrorReason(C.MLN_RESOURCE_ERROR_REASON_NOT_FOUND)
	ResourceErrorReasonServer     ResourceErrorReason = ResourceErrorReason(C.MLN_RESOURCE_ERROR_REASON_SERVER)
	ResourceErrorReasonConnection ResourceErrorReason = ResourceErrorReason(C.MLN_RESOURCE_ERROR_REASON_CONNECTION)
	ResourceErrorReasonRateLimit  ResourceErrorReason = ResourceErrorReason(C.MLN_RESOURCE_ERROR_REASON_RATE_LIMIT)
	ResourceErrorReasonOther      ResourceErrorReason = ResourceErrorReason(C.MLN_RESOURCE_ERROR_REASON_OTHER)
)

// ResourceProviderDecision selects whether native networking or the Go provider
// handles a request.
type ResourceProviderDecision uint32

const (
	ResourceProviderDecisionPassThrough ResourceProviderDecision = ResourceProviderDecision(C.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH)
	ResourceProviderDecisionHandle      ResourceProviderDecision = ResourceProviderDecision(C.MLN_RESOURCE_PROVIDER_DECISION_HANDLE)
)

// ResourceRequest is a copied native resource request.
type ResourceRequest struct {
	URL                 string
	Kind                ResourceKind
	RawKind             uint32
	LoadingMethod       ResourceLoadingMethod
	Priority            ResourcePriority
	Usage               ResourceUsage
	StoragePolicy       ResourceStoragePolicy
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

// ResourceResponse is copied into native memory during request completion.
type ResourceResponse struct {
	Status           ResourceResponseStatus
	ErrorReason      ResourceErrorReason
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

// ResourceTransformRequest describes a URL transform request copied for Go.
type ResourceTransformRequest struct {
	Kind    ResourceKind
	RawKind uint32
	URL     string
}

// ResourceTransformCallback rewrites network resource URLs. Native code may
// invoke it on worker or network threads. The request data is copied for Go; do
// not call MapLibre map/runtime APIs from the callback. Return replace=false or
// an empty URL to keep the original URL. Panics become native callback errors,
// and replacement URLs containing embedded NUL are rejected.
type ResourceTransformCallback func(ResourceTransformRequest) (replacementURL string, replace bool)

// ResourceProviderCallback intercepts network resource requests. Native code may
// invoke it on worker or network threads. The request data is copied for Go; do
// not call MapLibre map/runtime APIs from the callback. If it returns
// ResourceProviderDecisionHandle, complete or close the provided handle from a
// C-permitted thread. Panics return an unknown decision unless the handle was
// already completed.
type ResourceProviderCallback func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision

// ResourceRequestHandle owns a provider-selected native request handle.
type ResourceRequestHandle struct {
	state *callback.ResourceRequestHandle
}

func newResourceRequestHandle(state *callback.ResourceRequestHandle) *ResourceRequestHandle {
	handle := &ResourceRequestHandle{state: state}
	stdruntime.SetFinalizer(handle, func(handle *ResourceRequestHandle) { handle.Close() })
	return handle
}

// Complete sends a resource response to native code and releases the handle
// when native ownership has been finalized.
func (handle *ResourceRequestHandle) Complete(response ResourceResponse) error {
	if handle == nil || handle.state == nil {
		return newBindingError(ErrInvalidArgument, "ResourceRequestHandle is nil")
	}
	if err := validateResourceResponse(response); err != nil {
		return err
	}
	return checkNative(func() int32 {
		return int32(handle.state.Complete(callback.ResourceResponse{
			Status:           uint32(response.Status),
			ErrorReason:      uint32(response.ErrorReason),
			Bytes:            response.Bytes,
			ErrorMessage:     response.ErrorMessage,
			MustRevalidate:   response.MustRevalidate,
			HasModified:      response.HasModified,
			ModifiedUnixMS:   response.ModifiedUnixMS,
			HasExpires:       response.HasExpires,
			ExpiresUnixMS:    response.ExpiresUnixMS,
			ETag:             response.ETag,
			HasRetryAfter:    response.HasRetryAfter,
			RetryAfterUnixMS: response.RetryAfterUnixMS,
		}))
	})
}

// Cancelled reports whether native code cancelled the provider request.
func (handle *ResourceRequestHandle) Cancelled() (bool, error) {
	if handle == nil || handle.state == nil {
		return false, newBindingError(ErrInvalidArgument, "ResourceRequestHandle is nil")
	}
	var cancelled bool
	if err := checkNative(func() int32 {
		status, value := handle.state.Cancelled()
		cancelled = value
		return int32(status)
	}); err != nil {
		return false, err
	}
	return cancelled, nil
}

// Close releases the provider-owned request handle without completing it.
func (handle *ResourceRequestHandle) Close() {
	if handle == nil || handle.state == nil {
		return
	}
	handle.state.Close()
}

func validateResourceResponse(response ResourceResponse) error {
	if err := validateCStringArgument("resource response error message", response.ErrorMessage); err != nil {
		return err
	}
	if err := validateCStringArgument("resource response ETag", response.ETag); err != nil {
		return err
	}
	return nil
}
