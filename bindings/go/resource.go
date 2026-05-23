package maplibre

import (
	stdruntime "runtime"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

// ResourceKind identifies the native resource category for a request.
type ResourceKind uint32

const (
	ResourceKindUnknown     ResourceKind = ResourceKind(capi.ResourceKindUnknown)
	ResourceKindStyle       ResourceKind = ResourceKind(capi.ResourceKindStyle)
	ResourceKindSource      ResourceKind = ResourceKind(capi.ResourceKindSource)
	ResourceKindTile        ResourceKind = ResourceKind(capi.ResourceKindTile)
	ResourceKindGlyphs      ResourceKind = ResourceKind(capi.ResourceKindGlyphs)
	ResourceKindSpriteImage ResourceKind = ResourceKind(capi.ResourceKindSpriteImage)
	ResourceKindSpriteJSON  ResourceKind = ResourceKind(capi.ResourceKindSpriteJSON)
	ResourceKindImage       ResourceKind = ResourceKind(capi.ResourceKindImage)
)

// ResourceLoadingMethod identifies native cache/network loading policy.
type ResourceLoadingMethod uint32

const (
	ResourceLoadingMethodAll         ResourceLoadingMethod = ResourceLoadingMethod(capi.ResourceLoadingMethodAll)
	ResourceLoadingMethodCacheOnly   ResourceLoadingMethod = ResourceLoadingMethod(capi.ResourceLoadingMethodCacheOnly)
	ResourceLoadingMethodNetworkOnly ResourceLoadingMethod = ResourceLoadingMethod(capi.ResourceLoadingMethodNetworkOnly)
)

// ResourcePriority identifies native request priority.
type ResourcePriority uint32

const (
	ResourcePriorityRegular ResourcePriority = ResourcePriority(capi.ResourcePriorityRegular)
	ResourcePriorityLow     ResourcePriority = ResourcePriority(capi.ResourcePriorityLow)
)

// ResourceUsage identifies online or offline request use.
type ResourceUsage uint32

const (
	ResourceUsageOnline  ResourceUsage = ResourceUsage(capi.ResourceUsageOnline)
	ResourceUsageOffline ResourceUsage = ResourceUsage(capi.ResourceUsageOffline)
)

// ResourceStoragePolicy identifies native cache persistence policy.
type ResourceStoragePolicy uint32

const (
	ResourceStoragePolicyPermanent ResourceStoragePolicy = ResourceStoragePolicy(capi.ResourceStoragePolicyPermanent)
	ResourceStoragePolicyVolatile  ResourceStoragePolicy = ResourceStoragePolicy(capi.ResourceStoragePolicyVolatile)
)

// ResourceResponseStatus identifies provider response status.
type ResourceResponseStatus uint32

const (
	ResourceResponseStatusOK          ResourceResponseStatus = ResourceResponseStatus(capi.ResourceResponseStatusOK)
	ResourceResponseStatusError       ResourceResponseStatus = ResourceResponseStatus(capi.ResourceResponseStatusError)
	ResourceResponseStatusNoContent   ResourceResponseStatus = ResourceResponseStatus(capi.ResourceResponseStatusNoContent)
	ResourceResponseStatusNotModified ResourceResponseStatus = ResourceResponseStatus(capi.ResourceResponseStatusNotModified)
)

// ResourceErrorReason identifies provider error categories.
type ResourceErrorReason uint32

const (
	ResourceErrorReasonNone       ResourceErrorReason = ResourceErrorReason(capi.ResourceErrorReasonNone)
	ResourceErrorReasonNotFound   ResourceErrorReason = ResourceErrorReason(capi.ResourceErrorReasonNotFound)
	ResourceErrorReasonServer     ResourceErrorReason = ResourceErrorReason(capi.ResourceErrorReasonServer)
	ResourceErrorReasonConnection ResourceErrorReason = ResourceErrorReason(capi.ResourceErrorReasonConnection)
	ResourceErrorReasonRateLimit  ResourceErrorReason = ResourceErrorReason(capi.ResourceErrorReasonRateLimit)
	ResourceErrorReasonOther      ResourceErrorReason = ResourceErrorReason(capi.ResourceErrorReasonOther)
)

// ResourceProviderDecision selects whether native networking or the Go provider
// handles a request.
type ResourceProviderDecision uint32

const (
	ResourceProviderDecisionPassThrough ResourceProviderDecision = ResourceProviderDecision(capi.ResourceProviderDecisionPassThrough)
	ResourceProviderDecisionHandle      ResourceProviderDecision = ResourceProviderDecision(capi.ResourceProviderDecisionHandle)
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
// an empty URL to keep the original URL. Panics or invalid replacement URLs are
// treated as no rewrite.
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
	return checkNative(func() capi.Status {
		return handle.state.Complete(callback.ResourceResponse{
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
		})
	})
}

// Cancelled reports whether native code cancelled the provider request.
func (handle *ResourceRequestHandle) Cancelled() (bool, error) {
	if handle == nil || handle.state == nil {
		return false, newBindingError(ErrInvalidArgument, "ResourceRequestHandle is nil")
	}
	var cancelled bool
	if err := checkNative(func() capi.Status {
		status, value := handle.state.Cancelled()
		cancelled = value
		return status
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
