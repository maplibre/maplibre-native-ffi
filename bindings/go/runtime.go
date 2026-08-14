package maplibre

/*
#include <stdlib.h>

#include "internal/cgo_runtime_shim.h"
*/
import "C"

import (
	"errors"
	stdruntime "runtime"
	"sync"
	"time"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/memory"
)

// NetworkStatus is MapLibre Native's process-global network reachability mode.
type NetworkStatus uint32

const (
	NetworkStatusOnline  NetworkStatus = NetworkStatus(C.MLN_NETWORK_STATUS_ONLINE)
	NetworkStatusOffline NetworkStatus = NetworkStatus(C.MLN_NETWORK_STATUS_OFFLINE)
)

// AmbientCacheOperation selects a native ambient cache maintenance operation.
type AmbientCacheOperation uint32

const (
	AmbientCacheOperationResetDatabase AmbientCacheOperation = AmbientCacheOperation(C.MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE)
	AmbientCacheOperationPackDatabase  AmbientCacheOperation = AmbientCacheOperation(C.MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE)
	AmbientCacheOperationInvalidate    AmbientCacheOperation = AmbientCacheOperation(C.MLN_AMBIENT_CACHE_OPERATION_INVALIDATE)
	AmbientCacheOperationClear         AmbientCacheOperation = AmbientCacheOperation(C.MLN_AMBIENT_CACHE_OPERATION_CLEAR)
)

// OfflineOperationKind identifies a native offline operation kind.
type OfflineOperationKind uint32

const (
	OfflineOperationAmbientCache               OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_AMBIENT_CACHE)
	OfflineOperationRegionCreate               OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_CREATE)
	OfflineOperationRegionGet                  OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_GET)
	OfflineOperationRegionsList                OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGIONS_LIST)
	OfflineOperationRegionsMergeDatabase       OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE)
	OfflineOperationRegionUpdateMetadata       OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA)
	OfflineOperationRegionGetStatus            OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_GET_STATUS)
	OfflineOperationRegionSetObserved          OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED)
	OfflineOperationRegionSetDownloadState     OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE)
	OfflineOperationRegionInvalidate           OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_INVALIDATE)
	OfflineOperationRegionDelete               OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_REGION_DELETE)
	OfflineOperationSetMaximumAmbientCacheSize OfflineOperationKind = OfflineOperationKind(C.MLN_OFFLINE_OPERATION_SET_MAXIMUM_AMBIENT_CACHE_SIZE)
)

// OfflineOperationResultKind identifies the expected result shape for an
// offline operation.
type OfflineOperationResultKind uint32

const (
	OfflineOperationResultNone           OfflineOperationResultKind = OfflineOperationResultKind(C.MLN_OFFLINE_OPERATION_RESULT_NONE)
	OfflineOperationResultRegion         OfflineOperationResultKind = OfflineOperationResultKind(C.MLN_OFFLINE_OPERATION_RESULT_REGION)
	OfflineOperationResultOptionalRegion OfflineOperationResultKind = OfflineOperationResultKind(C.MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION)
	OfflineOperationResultRegionList     OfflineOperationResultKind = OfflineOperationResultKind(C.MLN_OFFLINE_OPERATION_RESULT_REGION_LIST)
	OfflineOperationResultRegionStatus   OfflineOperationResultKind = OfflineOperationResultKind(C.MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS)
)

// OfflineOperationHandle owns a runtime-scoped offline operation token.
type OfflineOperationHandle[T any] struct {
	runtime    *RuntimeHandle
	child      *handle.Child
	id         uint64
	kind       OfflineOperationKind
	resultKind OfflineOperationResultKind
	mu         sync.Mutex
	live       bool
	discarded  bool
}

func newOfflineOperationHandle[T any](runtime *RuntimeHandle, id uint64, kind OfflineOperationKind, resultKind OfflineOperationResultKind) *OfflineOperationHandle[T] {
	var child *handle.Child
	if runtime != nil && runtime.state != nil {
		child = runtime.state.AddChild()
	}
	return &OfflineOperationHandle[T]{runtime: runtime, child: child, id: id, kind: kind, resultKind: resultKind, live: true}
}

// ID returns the native offline operation ID.
func (operation *OfflineOperationHandle[T]) ID() uint64 {
	if operation == nil {
		return 0
	}
	operation.mu.Lock()
	defer operation.mu.Unlock()
	return operation.id
}

// Kind returns the native offline operation kind.
func (operation *OfflineOperationHandle[T]) Kind() OfflineOperationKind {
	if operation == nil {
		return 0
	}
	operation.mu.Lock()
	defer operation.mu.Unlock()
	return operation.kind
}

// ResultKind returns the expected native result shape.
func (operation *OfflineOperationHandle[T]) ResultKind() OfflineOperationResultKind {
	if operation == nil {
		return 0
	}
	operation.mu.Lock()
	defer operation.mu.Unlock()
	return operation.resultKind
}

// Discard drops runtime-owned state for this operation. The operation remains
// retryable when native discard fails.
func (operation *OfflineOperationHandle[T]) Discard() error {
	if operation == nil || operation.runtime == nil {
		return newBindingError(ErrInvalidArgument, "OfflineOperationHandle is nil")
	}
	operation.mu.Lock()
	if !operation.live {
		discarded := operation.discarded
		operation.mu.Unlock()
		if discarded {
			return nil
		}
		return newBindingError(ErrInvalidArgument, "OfflineOperationHandle is closed")
	}
	id := operation.id

	ptr, release, err := operation.runtime.ptr()
	if err != nil {
		operation.mu.Unlock()
		return err
	}
	defer release()
	defer operation.runtime.state.KeepAlive()
	if err := checkNative(func() int32 { return offlineOperationDiscard(ptr, id) }); err != nil {
		operation.mu.Unlock()
		return err
	}
	operation.live = false
	operation.discarded = true
	child := operation.child
	operation.child = nil
	operation.mu.Unlock()
	child.Release()
	return nil
}

// RuntimeOptions configures runtime creation.
type RuntimeOptions struct {
	AssetPath string
	CachePath string
	// EventMask selects the runtime-originated event types this runtime queues.
	// NewRuntimeOptions sets it to the native default, which selects every type.
	// See RuntimeHandle.SetEventMask.
	EventMask RuntimeEventMask
}

// NewRuntimeOptions returns runtime creation options for an asset root and a
// cache path, either of which may be empty to keep the native default. The
// returned options select every runtime-originated event type.
func NewRuntimeOptions(assetPath, cachePath string) RuntimeOptions {
	return RuntimeOptions{
		AssetPath: assetPath,
		CachePath: cachePath,
		EventMask: defaultRuntimeEventMask(),
	}
}

// defaultRuntimeEventMask reads the runtime default's own event mask. The bits
// are retained rather than named, so a newer native library's default keeps
// selecting event types this build does not define. Those reach a host as
// unknown event and payload domains.
func defaultRuntimeEventMask() RuntimeEventMask {
	return RuntimeEventMask(C.mln_runtime_options_default().event_mask)
}

// Equal reports whether two descriptors hold the same field values.
func (options RuntimeOptions) Equal(other RuntimeOptions) bool {
	return options.AssetPath == other.AssetPath &&
		options.CachePath == other.CachePath &&
		options.EventMask == other.EventMask
}

func (options RuntimeOptions) validate() error {
	if _, err := memory.NewCString(options.AssetPath); err != nil {
		if errors.Is(err, memory.EmbeddedNulError()) {
			return newBindingError(ErrInvalidArgument, "RuntimeOptions.AssetPath contains embedded NUL")
		}
		return err
	}
	if _, err := memory.NewCString(options.CachePath); err != nil {
		if errors.Is(err, memory.EmbeddedNulError()) {
			return newBindingError(ErrInvalidArgument, "RuntimeOptions.CachePath contains embedded NUL")
		}
		return err
	}
	return nil
}

// RuntimeEventType identifies a runtime event kind.
type RuntimeEventType uint32

const (
	RuntimeEventMapCameraWillChange                 RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE)
	RuntimeEventMapCameraIsChanging                 RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING)
	RuntimeEventMapCameraDidChange                  RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE)
	RuntimeEventMapStyleLoaded                      RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
	RuntimeEventMapLoadingStarted                   RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_LOADING_STARTED)
	RuntimeEventMapLoadingFinished                  RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED)
	RuntimeEventMapLoadingFailed                    RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_LOADING_FAILED)
	RuntimeEventMapIdle                             RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_IDLE)
	RuntimeEventMapRenderUpdateAvailable            RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE)
	RuntimeEventMapRenderError                      RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_RENDER_ERROR)
	RuntimeEventMapStillImageFinished               RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED)
	RuntimeEventMapStillImageFailed                 RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED)
	RuntimeEventMapRenderFrameStarted               RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED)
	RuntimeEventMapRenderFrameFinished              RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED)
	RuntimeEventMapRenderMapStarted                 RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED)
	RuntimeEventMapRenderMapFinished                RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED)
	RuntimeEventMapStyleImageMissing                RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING)
	RuntimeEventMapTileAction                       RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_TILE_ACTION)
	RuntimeEventOfflineRegionStatusChanged          RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED)
	RuntimeEventOfflineRegionResponseError          RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR)
	RuntimeEventOfflineRegionTileCountLimitExceeded RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED)
	RuntimeEventOfflineOperationCompleted           RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED)
	RuntimeEventMapCameraTransitionFinished         RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED)
)

// RuntimeEventMask selects which event types a map or a runtime queues. An event
// whose type is unselected is never built and never queued, so it neither
// reaches a batch nor raises the runtime's wake flag.
//
// Every bit value comes from the C API, so a mask cannot drift from the
// RuntimeEventType constants. Masks combine with the bitwise operators: | adds
// types, &^ clears them, and RuntimeEventMaskNone is the empty mask.
type RuntimeEventMask uint64

const (
	// RuntimeEventMaskNone selects no event type.
	RuntimeEventMaskNone                                RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_NONE)
	RuntimeEventMaskMapCameraWillChange                 RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE)
	RuntimeEventMaskMapCameraIsChanging                 RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING)
	RuntimeEventMaskMapCameraDidChange                  RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE)
	RuntimeEventMaskMapStyleLoaded                      RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED)
	RuntimeEventMaskMapLoadingStarted                   RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED)
	RuntimeEventMaskMapLoadingFinished                  RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED)
	RuntimeEventMaskMapLoadingFailed                    RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED)
	RuntimeEventMaskMapIdle                             RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_IDLE)
	RuntimeEventMaskMapRenderUpdateAvailable            RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE)
	RuntimeEventMaskMapRenderError                      RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR)
	RuntimeEventMaskMapStillImageFinished               RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED)
	RuntimeEventMaskMapStillImageFailed                 RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED)
	RuntimeEventMaskMapRenderFrameStarted               RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED)
	RuntimeEventMaskMapRenderFrameFinished              RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED)
	RuntimeEventMaskMapRenderMapStarted                 RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED)
	RuntimeEventMaskMapRenderMapFinished                RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED)
	RuntimeEventMaskMapStyleImageMissing                RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING)
	RuntimeEventMaskMapTileAction                       RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION)
	RuntimeEventMaskMapCameraTransitionFinished         RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED)
	RuntimeEventMaskOfflineRegionStatusChanged          RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED)
	RuntimeEventMaskOfflineRegionResponseError          RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR)
	RuntimeEventMaskOfflineRegionTileCountLimitExceeded RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED)
	RuntimeEventMaskOfflineOperationCompleted           RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_OFFLINE_OPERATION_COMPLETED)
	// RuntimeEventMaskAllMapEvents selects every map-originated event type this
	// binding version defines.
	RuntimeEventMaskAllMapEvents RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS)
	// RuntimeEventMaskAllRuntimeEvents selects every runtime-originated event
	// type this binding version defines.
	RuntimeEventMaskAllRuntimeEvents RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS)
	// RuntimeEventMaskAll selects every event type this binding version defines,
	// and both mask setters accept it.
	RuntimeEventMaskAll RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_ALL)
)

// Has reports whether all requested event type bits are set.
func (mask RuntimeEventMask) Has(requested RuntimeEventMask) bool {
	return mask&requested == requested
}

// RuntimeEventSourceType identifies the native handle kind that emitted an event.
type RuntimeEventSourceType uint32

const (
	RuntimeEventSourceRuntime RuntimeEventSourceType = RuntimeEventSourceType(C.MLN_RUNTIME_EVENT_SOURCE_RUNTIME)
	RuntimeEventSourceMap     RuntimeEventSourceType = RuntimeEventSourceType(C.MLN_RUNTIME_EVENT_SOURCE_MAP)
)

// RuntimeEventPayloadType identifies the copied event payload shape.
type RuntimeEventPayloadType uint32

const (
	RuntimeEventPayloadNone                        RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_NONE)
	RuntimeEventPayloadRenderFrame                 RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME)
	RuntimeEventPayloadRenderMap                   RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP)
	RuntimeEventPayloadTileAction                  RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION)
	RuntimeEventPayloadOfflineRegionStatus         RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS)
	RuntimeEventPayloadOfflineRegionResponseError  RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR)
	RuntimeEventPayloadOfflineRegionTileCountLimit RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT)
	RuntimeEventPayloadOfflineOperationCompleted   RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED)
	RuntimeEventPayloadCameraTransitionFinished    RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED)
)

// MapID identifies a map within one RuntimeHandle.
type MapID uint64

// RuntimeEventSource identifies the runtime object that emitted an event.
type RuntimeEventSource struct {
	Type RuntimeEventSourceType
	// RawID is the native source id the C API reported, whatever Type is. It
	// names one object for the life of the process, so a host may compare it
	// against an id it already holds, even for a source type this binding
	// version does not name or a map this runtime no longer tracks. It is an
	// identity value only: no public handle comes from it.
	RawID uint64
	// MapID is the identity of the live map this runtime resolved RawID to, and 0
	// when Type is not RuntimeEventSourceMap or the map is gone. Compare RawID
	// instead to attribute an event whose map has been closed.
	MapID MapID
}

// RuntimeEvent is a copied runtime event. Unknown payloads preserve raw
// metadata and bytes.
type RuntimeEvent struct {
	Type       RuntimeEventType
	SourceType RuntimeEventSourceType
	Source     RuntimeEventSource
	// Code is a secondary event detail whose meaning Type selects: a
	// CameraChangeMode for the camera-will-change and camera-did-change events,
	// the ordinal of MapLibre Native's internal map load error kind for
	// map-loading-failed, and the result status for
	// offline-operation-completed. Every other event type reports 0.
	Code        int32
	PayloadType RuntimeEventPayloadType
	// Message is the event's text: a failure description, a missing style image
	// ID, or a tile action's source ID. It is empty for an event that carries no
	// message.
	Message string
	// Payload is the typed payload PayloadType selects, nil for an event without
	// one, and a RuntimeEventUnknownPayload for a payload type this binding
	// version does not define.
	Payload any
}

// RuntimeEventBatch is one drained batch of runtime events in queue order.
// Every field of every event is copied out of runtime-owned storage before the
// drain returns, so a batch and the values taken out of it stay readable after
// the next drain.
type RuntimeEventBatch struct {
	Events []RuntimeEvent
	// RemainingCount is the number of events still queued after this batch. A
	// nonzero value means another drain reports more.
	RemainingCount uint64
}

// CameraChangeMode reports whether a camera change belongs to an animated
// transition. It is the meaning of RuntimeEvent.Code for
// RuntimeEventMapCameraWillChange and RuntimeEventMapCameraDidChange.
type CameraChangeMode uint32

const (
	// CameraChangeModeImmediate marks a camera that reached its new value
	// without an animated transition.
	CameraChangeModeImmediate CameraChangeMode = CameraChangeMode(C.MLN_CAMERA_CHANGE_MODE_IMMEDIATE)
	// CameraChangeModeAnimated marks a camera that moved as part of an animated
	// transition.
	CameraChangeModeAnimated CameraChangeMode = CameraChangeMode(C.MLN_CAMERA_CHANGE_MODE_ANIMATED)
)

// RenderMode identifies a render observer mode.
type RenderMode uint32

const (
	RenderModePartial RenderMode = RenderMode(C.MLN_RENDER_MODE_PARTIAL)
	RenderModeFull    RenderMode = RenderMode(C.MLN_RENDER_MODE_FULL)
)

// TileOperation identifies a tile observer operation.
type TileOperation uint32

const (
	TileOperationRequestedFromCache   TileOperation = TileOperation(C.MLN_TILE_OPERATION_REQUESTED_FROM_CACHE)
	TileOperationRequestedFromNetwork TileOperation = TileOperation(C.MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK)
	TileOperationLoadFromNetwork      TileOperation = TileOperation(C.MLN_TILE_OPERATION_LOAD_FROM_NETWORK)
	TileOperationLoadFromCache        TileOperation = TileOperation(C.MLN_TILE_OPERATION_LOAD_FROM_CACHE)
	TileOperationStartParse           TileOperation = TileOperation(C.MLN_TILE_OPERATION_START_PARSE)
	TileOperationEndParse             TileOperation = TileOperation(C.MLN_TILE_OPERATION_END_PARSE)
	TileOperationError                TileOperation = TileOperation(C.MLN_TILE_OPERATION_ERROR)
	TileOperationCancelled            TileOperation = TileOperation(C.MLN_TILE_OPERATION_CANCELLED)
	TileOperationNull                 TileOperation = TileOperation(C.MLN_TILE_OPERATION_NULL)
)

// RenderingStats is copied render-frame statistics.
type RenderingStats struct {
	EncodingTime       float64
	RenderingTime      float64
	FrameCount         int64
	DrawCallCount      int64
	TotalDrawCallCount int64
}

// TileID is a copied overscaled/canonical tile identifier.
type TileID struct {
	OverscaledZ uint32
	Wrap        int32
	CanonicalZ  uint32
	CanonicalX  uint32
	CanonicalY  uint32
}

// RuntimeEventRenderFramePayload is a copied render-frame event payload.
type RuntimeEventRenderFramePayload struct {
	Mode             RenderMode
	RawMode          uint32
	NeedsRepaint     bool
	PlacementChanged bool
	Stats            RenderingStats
}

// RuntimeEventRenderMapPayload is a copied render-map event payload.
type RuntimeEventRenderMapPayload struct {
	Mode    RenderMode
	RawMode uint32
}

// RuntimeEventTileActionPayload is a copied tile-action event payload. The
// event message carries the source ID.
type RuntimeEventTileActionPayload struct {
	Operation    TileOperation
	RawOperation uint32
	TileID       TileID
}

// RuntimeEventCameraTransitionFinishedPayload is a copied camera
// transition-finished event payload. It carries the identity the caller stamped
// on the transition through AnimationOptions.TransitionID.
type RuntimeEventCameraTransitionFinishedPayload struct {
	TransitionID uint64
}

// RuntimeEventOfflineRegionStatusPayload is a copied offline status event payload.
type RuntimeEventOfflineRegionStatusPayload struct {
	RegionID OfflineRegionID
	Status   OfflineRegionStatus
}

// RuntimeEventOfflineRegionResponseErrorPayload is a copied offline response
// error event payload.
type RuntimeEventOfflineRegionResponseErrorPayload struct {
	RegionID  OfflineRegionID
	Reason    ResourceErrorReason
	RawReason uint32
}

// RuntimeEventOfflineRegionTileCountLimitPayload is a copied offline tile-count
// limit event payload.
type RuntimeEventOfflineRegionTileCountLimitPayload struct {
	RegionID OfflineRegionID
	Limit    uint64
}

// RuntimeEventOfflineOperationCompletedPayload is a copied offline operation
// completion event payload.
type RuntimeEventOfflineOperationCompletedPayload struct {
	OperationID   uint64
	OperationKind OfflineOperationKind
	ResultKind    OfflineOperationResultKind
	ResultStatus  int32
	Found         bool
}

// RuntimeEventUnknownPayload contains copied bytes for a payload type unknown to
// this Go binding version. Bytes is the event's whole payload window, which is
// the batch's event stride minus this binding's payload offset.
type RuntimeEventUnknownPayload struct {
	Bytes []byte
}

// RuntimeHandle owns scheduler state and event storage for one owner thread.
type RuntimeHandle struct {
	state *handle.State[nativeRuntime]

	resourceTransformMu   sync.Mutex
	resourceTransform     *callback.ResourceTransformState
	httpHeaderTransformMu sync.Mutex
	httpHeaderTransform   *callback.HttpHeaderTransformState
	resourceProviderMu    sync.Mutex
	resourceProvider      *callback.ResourceProviderState
	mapsMu                sync.Mutex
	// Resolves an event's source id to the public wrapper.
	maps map[MapID]*MapHandle
}

var destroyRuntimeHandle = func(native nativeRuntime) int32 {
	return int32(C.mln_runtime_destroy(C.mln_runtime(native)))
}

var offlineOperationDiscard = func(ptr nativeRuntime, id uint64) int32 {
	return int32(C.mln_runtime_offline_operation_discard(C.mln_runtime(ptr), C.mln_offline_operation_id(id)))
}

// String returns a diagnostic name for the status.
func (status NetworkStatus) String() string {
	switch status {
	case NetworkStatusOnline:
		return "online"
	case NetworkStatusOffline:
		return "offline"
	default:
		return "unknown"
	}
}

// CurrentNetworkStatus reads MapLibre Native's process-global network status.
func CurrentNetworkStatus() (NetworkStatus, error) {
	var raw C.uint32_t
	if err := checkNative(func() int32 { return int32(C.mln_network_status_get(&raw)) }); err != nil {
		return 0, err
	}
	return NetworkStatus(raw), nil
}

// SetNetworkStatus sets MapLibre Native's process-global network status.
func SetNetworkStatus(status NetworkStatus) error {
	raw, err := rawNetworkStatusForSet(status)
	if err != nil {
		return err
	}
	return networkStatusSetRaw(raw)
}

func networkStatusSetRaw(raw uint32) error {
	return checkNative(func() int32 { return int32(C.mln_network_status_set(C.uint32_t(raw))) })
}

func rawNetworkStatusForSet(status NetworkStatus) (uint32, error) {
	switch status {
	case NetworkStatusOnline, NetworkStatusOffline:
		return uint32(status), nil
	default:
		return 0, newBindingError(ErrInvalidArgument, "unknown network status cannot be set")
	}
}

// NewRuntime creates a runtime on the current OS thread using native defaults.
func NewRuntime() (*RuntimeHandle, error) {
	return createRuntime(CVersion(), func(out *nativeRuntime) int32 {
		var raw C.mln_runtime
		status := int32(C.mln_runtime_create(nil, &raw))
		if status == int32(C.MLN_STATUS_OK) {
			*out = nativeRuntime(raw)
		}
		return status
	})
}

// NewRuntimeWithOptions creates a runtime on the current OS thread using
// explicit options. Start from NewRuntimeOptions to keep every event type
// selected; a zero-value RuntimeOptions queues no event.
func NewRuntimeWithOptions(options RuntimeOptions) (*RuntimeHandle, error) {
	if err := options.validate(); err != nil {
		return nil, err
	}
	return createRuntime(CVersion(), func(out *nativeRuntime) int32 {
		rawOptions := C.mln_runtime_options_default()
		assetPath := C.CString(options.AssetPath)
		defer C.free(unsafe.Pointer(assetPath))
		cachePath := C.CString(options.CachePath)
		defer C.free(unsafe.Pointer(cachePath))
		rawOptions.asset_path = assetPath
		rawOptions.cache_path = cachePath
		rawOptions.event_mask = C.uint64_t(options.EventMask)

		var raw C.mln_runtime
		status := int32(C.mln_runtime_create(&rawOptions, &raw))
		if status == int32(C.MLN_STATUS_OK) {
			*out = nativeRuntime(raw)
		}
		return status
	})
}

type runtimeStateFactory func(nativeRuntime) (*handle.State[nativeRuntime], error)

func createRuntime(actualCABI uint32, create func(*nativeRuntime) int32) (*RuntimeHandle, error) {
	return createRuntimeWithStateFactory(actualCABI, create, newRuntimeState)
}

func createRuntimeWithStateFactory(actualCABI uint32, create func(*nativeRuntime) int32, newState runtimeStateFactory) (*RuntimeHandle, error) {
	if err := checkCompatibleCABI(actualCABI); err != nil {
		return nil, err
	}

	var runtime nativeRuntime
	if err := checkNative(func() int32 { return create(&runtime) }); err != nil {
		return nil, err
	}
	state, err := newState(runtime)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &RuntimeHandle{state: state}, nil
}

func newRuntimeState(runtime nativeRuntime) (*handle.State[nativeRuntime], error) {
	return handle.New(runtime, "RuntimeHandle")
}

func (runtime *RuntimeHandle) ptr() (nativeRuntime, func(), error) {
	if runtime == nil || runtime.state == nil {
		return 0, nil, newBindingError(ErrInvalidArgument, "RuntimeHandle is nil")
	}
	borrow, live := runtime.state.Borrow()
	if !live {
		return 0, nil, newBindingError(ErrInvalidArgument, "RuntimeHandle is closed")
	}
	return borrow.Handle(), borrow.Release, nil
}

// Pump advances this runtime. It parks the owner thread when timeout allows,
// then drains the owner-thread task queues, including tasks the drained ones
// enqueue. Take the queued runtime events with DrainEvents afterwards.
//
// timeout sets the park bound: zero drains and returns, a positive value parks
// for up to that long, and a negative value parks until a wake arrives. A
// parking call returns as soon as the runtime's wake flag is set and clears it,
// and returns without parking while unread runtime events are queued. Timers
// and ready file descriptors set the flag only when they queue owner-thread
// work, so pass a bounded timeout to cap how long a call waits.
//
// budget bounds the drain: a negative value drains without a bound, and zero
// or a positive value stops the drain at the first task boundary after that
// long, measured from the start of the drain. The first queued task always
// runs, so a bounded pump always makes progress, and tasks left behind set the
// wake flag so the next Pump returns without parking and continues them. The
// budget bounds the task queues alone: expired timers and ready file
// descriptors are serviced regardless, and a task runs to completion once
// started, so one long task can overrun the budget.
//
// A non-zero timeout blocks the calling goroutine and its OS thread. Call it
// outside any lock that a goroutine signalling a WakeSource takes.
func (runtime *RuntimeHandle) Pump(timeout time.Duration, budget time.Duration) error {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()

	timeoutMS := int64(-1)
	if timeout >= 0 {
		timeoutMS = int64(timeout / time.Millisecond)
	}
	budgetMS := int64(-1)
	if budget >= 0 {
		budgetMS = int64(budget / time.Millisecond)
	}
	return checkNative(func() int32 {
		return int32(C.mln_runtime_pump(
			C.mln_runtime(ptr), C.int64_t(timeoutMS), C.int64_t(budgetMS),
		))
	})
}

// WakeSource acquires a wake source that releases this runtime's parked owner
// thread. The returned source is usable from any goroutine, and the caller
// closes it.
func (runtime *RuntimeHandle) WakeSource() (*WakeSource, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var raw C.mln_wake_source
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_wake_source_acquire(C.mln_runtime(ptr), &raw))
	}); err != nil {
		return nil, err
	}
	state, err := handle.New(nativeWakeSource(raw), "WakeSource")
	if err != nil {
		C.mln_wake_source_destroy(raw)
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	source := &WakeSource{state: state}
	stdruntime.SetFinalizer(source, func(source *WakeSource) { source.Close() })
	return source, nil
}

// WakeSource releases a runtime owner thread parked in RuntimeHandle.Pump. It
// is usable from any goroutine. Signalling it after its runtime closes does
// nothing.
type WakeSource struct {
	state *handle.State[nativeWakeSource]
}

var destroyWakeSource = func(native nativeWakeSource) int32 {
	C.mln_wake_source_destroy(C.mln_wake_source(native))
	return int32(C.MLN_STATUS_OK)
}

// Signal sets the runtime's wake flag and releases the parked owner thread. A
// signal raised while the owner thread runs leaves the flag set, so the next
// Pump returns without parking.
func (source *WakeSource) Signal() error {
	if source == nil || source.state == nil {
		return newBindingError(ErrInvalidArgument, "WakeSource is nil")
	}
	borrow, live := source.state.Borrow()
	if !live {
		return newBindingError(ErrInvalidArgument, "WakeSource is closed")
	}
	defer borrow.Release()
	defer source.state.KeepAlive()
	return checkNative(func() int32 {
		return int32(C.mln_wake_source_signal(C.mln_wake_source(borrow.Handle())))
	})
}

// Close releases the wake source. Later signals report a closed handle.
func (source *WakeSource) Close() {
	if source == nil || source.state == nil {
		return
	}
	source.state.Close(destroyWakeSource)
}

// DrainEvents takes this runtime's queued events as one batch of copied values.
// Events arrive in queue order, and the batch reports how many events stayed
// queued.
//
// maxEvents bounds the drain: zero takes every queued event, and a positive
// value takes at most that many and leaves the rest queued. A negative value
// returns ErrInvalidArgument.
//
// Call Pump first to advance the runtime, then drain the events that pump
// produced.
func (runtime *RuntimeHandle) DrainEvents(maxEvents int) (RuntimeEventBatch, error) {
	if maxEvents < 0 {
		return RuntimeEventBatch{}, newBindingError(ErrInvalidArgument, "maxEvents is negative")
	}
	ptr, release, err := runtime.ptr()
	if err != nil {
		return RuntimeEventBatch{}, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	rawBatch, err := drainRawEvents(ptr, maxEvents)
	if err != nil {
		return RuntimeEventBatch{}, err
	}
	return runtime.copyEventBatch(rawBatch), nil
}

// SetEventMask selects which runtime-originated event types this runtime queues.
// It accepts RuntimeEventMaskAll, reads the bits in
// RuntimeEventMaskAllRuntimeEvents, and returns ErrInvalidArgument for a bit
// outside RuntimeEventMaskAll.
//
// Narrowing gates later events and keeps queued ones, so a caller drains what it
// already caused. An offline operation records its result before this mask is
// consulted, so the matching take-result call reports the result of an operation
// whose completion event this mask cleared.
func (runtime *RuntimeHandle) SetEventMask(mask RuntimeEventMask) error {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()

	return checkNative(func() int32 {
		return int32(C.mln_runtime_set_event_mask(C.mln_runtime(ptr), C.uint64_t(mask)))
	})
}

// EventMask reports which runtime-originated event types this runtime queues. A
// runtime that has not been narrowed reports RuntimeEventMaskAll.
func (runtime *RuntimeHandle) EventMask() (RuntimeEventMask, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var raw C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_get_event_mask(C.mln_runtime(ptr), &raw))
	}); err != nil {
		return 0, err
	}
	return RuntimeEventMask(raw), nil
}

func drainRawEvents(ptr nativeRuntime, maxEvents int) (C.mln_runtime_event_batch, error) {
	batch := C.mln_runtime_event_batch_default()
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_drain_events(C.mln_runtime(ptr), C.size_t(maxEvents), &batch))
	}); err != nil {
		return C.mln_runtime_event_batch{}, err
	}
	return batch, nil
}

// runtimeEventPayloadOffset is where the payload union starts inside an event
// record. Every field before it keeps its offset across C API versions, and the
// batch's event stride minus this offset is the payload window.
var runtimeEventPayloadOffset = unsafe.Offsetof(C.mln_runtime_event{}.payload)

// copyEventBatch copies a borrowed native batch into owned Go values. It takes
// the map registry lock once for the whole batch.
func (runtime *RuntimeHandle) copyEventBatch(raw C.mln_runtime_event_batch) RuntimeEventBatch {
	batch := RuntimeEventBatch{RemainingCount: uint64(raw.remaining_count)}
	count := int(raw.event_count)
	if count <= 0 || raw.events == nil {
		return batch
	}
	// The stride comes from the batch, never from this binding's own event size,
	// so a C API version that widens the payload union stays readable.
	stride := uintptr(raw.event_size)
	base := unsafe.Pointer(raw.events)
	payloadWindow := uintptr(0)
	if stride > runtimeEventPayloadOffset {
		payloadWindow = stride - runtimeEventPayloadOffset
	}

	events := make([]RuntimeEvent, 0, count)
	runtime.mapsMu.Lock()
	for index := 0; index < count; index++ {
		eventPtr := unsafe.Add(base, uintptr(index)*stride)
		rawEvent := (*C.mln_runtime_event)(eventPtr)
		source := RuntimeEventSource{
			Type:  RuntimeEventSourceType(rawEvent.source_type),
			RawID: uint64(rawEvent.source),
		}
		if source.Type == RuntimeEventSourceMap {
			if sourceMap := runtime.maps[MapID(rawEvent.source)]; sourceMap != nil {
				source.MapID = sourceMap.id
			}
		}
		events = append(events, RuntimeEvent{
			Type:        RuntimeEventType(rawEvent._type),
			SourceType:  source.Type,
			Source:      source,
			Code:        int32(rawEvent.code),
			PayloadType: RuntimeEventPayloadType(rawEvent.payload_type),
			Message:     runtimeEventMessage(raw, rawEvent),
			Payload:     runtimeEventPayloadFromC(rawEvent, unsafe.Add(eventPtr, runtimeEventPayloadOffset), payloadWindow),
		})
	}
	runtime.mapsMu.Unlock()

	batch.Events = events
	return batch
}

func runtimeEventMessage(batch C.mln_runtime_event_batch, event *C.mln_runtime_event) string {
	if event.message_size == 0 || batch.messages == nil {
		return ""
	}
	messages := unsafe.Add(unsafe.Pointer(batch.messages), uintptr(event.message_offset))
	return goCharBytes(messages, C.size_t(event.message_size))
}

func (runtime *RuntimeHandle) registerMap(m *MapHandle) {
	if runtime == nil || m == nil || m.state == nil {
		return
	}
	runtime.mapsMu.Lock()
	if runtime.maps == nil {
		runtime.maps = make(map[MapID]*MapHandle)
	}
	runtime.maps[m.id] = m
	runtime.mapsMu.Unlock()
}

func (runtime *RuntimeHandle) unregisterMap(m *MapHandle) {
	if runtime == nil || m == nil || m.state == nil {
		return
	}
	runtime.mapsMu.Lock()
	delete(runtime.maps, m.id)
	runtime.mapsMu.Unlock()
}

// runtimeEventPayloadFromC copies the payload member payload_type names. The
// union member reads go through the cgo shim, because cgo lowers the payload
// union to opaque bytes and names no member. window and windowSize describe the
// event's payload bytes, which an unknown payload type preserves as they are.
func runtimeEventPayloadFromC(event *C.mln_runtime_event, window unsafe.Pointer, windowSize uintptr) any {
	switch uint32(event.payload_type) {
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_NONE):
		return nil
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME):
		payload := C.mln_go_runtime_event_render_frame(event)
		mode := uint32(payload.mode)
		return RuntimeEventRenderFramePayload{
			Mode:             RenderMode(mode),
			RawMode:          mode,
			NeedsRepaint:     bool(payload.needs_repaint),
			PlacementChanged: bool(payload.placement_changed),
			Stats:            renderingStatsFromC(payload.stats),
		}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP):
		payload := C.mln_go_runtime_event_render_map(event)
		mode := uint32(payload.mode)
		return RuntimeEventRenderMapPayload{Mode: RenderMode(mode), RawMode: mode}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION):
		payload := C.mln_go_runtime_event_tile_action(event)
		operation := uint32(payload.operation)
		return RuntimeEventTileActionPayload{
			Operation:    TileOperation(operation),
			RawOperation: operation,
			TileID:       tileIDFromC(payload.tile_id),
		}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED):
		payload := C.mln_go_runtime_event_camera_transition_finished(event)
		return RuntimeEventCameraTransitionFinishedPayload{TransitionID: uint64(payload.transition_id)}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS):
		payload := C.mln_go_runtime_event_offline_region_status(event)
		return RuntimeEventOfflineRegionStatusPayload{RegionID: OfflineRegionID(payload.region_id), Status: offlineRegionStatusFromC(payload.status)}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR):
		payload := C.mln_go_runtime_event_offline_region_response_error(event)
		reason := uint32(payload.reason)
		return RuntimeEventOfflineRegionResponseErrorPayload{RegionID: OfflineRegionID(payload.region_id), Reason: ResourceErrorReason(reason), RawReason: reason}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT):
		payload := C.mln_go_runtime_event_offline_region_tile_count_limit(event)
		return RuntimeEventOfflineRegionTileCountLimitPayload{RegionID: OfflineRegionID(payload.region_id), Limit: uint64(payload.limit)}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED):
		payload := C.mln_go_runtime_event_offline_operation_completed(event)
		return RuntimeEventOfflineOperationCompletedPayload{
			OperationID:   uint64(payload.operation_id),
			OperationKind: OfflineOperationKind(payload.operation_kind),
			ResultKind:    OfflineOperationResultKind(payload.result_kind),
			ResultStatus:  int32(payload.result_status),
			Found:         bool(payload.found),
		}
	default:
		bytes, ok := goByteSlice(window, C.size_t(windowSize))
		if !ok {
			return RuntimeEventUnknownPayload{}
		}
		return RuntimeEventUnknownPayload{Bytes: bytes}
	}
}

func renderingStatsFromC(stats C.mln_rendering_stats) RenderingStats {
	return RenderingStats{
		EncodingTime:       float64(stats.encoding_time),
		RenderingTime:      float64(stats.rendering_time),
		FrameCount:         int64(stats.frame_count),
		DrawCallCount:      int64(stats.draw_call_count),
		TotalDrawCallCount: int64(stats.total_draw_call_count),
	}
}

func tileIDFromC(tileID C.mln_tile_id) TileID {
	return TileID{
		OverscaledZ: uint32(tileID.overscaled_z),
		Wrap:        int32(tileID.wrap),
		CanonicalZ:  uint32(tileID.canonical_z),
		CanonicalX:  uint32(tileID.canonical_x),
		CanonicalY:  uint32(tileID.canonical_y),
	}
}

func offlineRegionStatusFromC(status C.mln_offline_region_status) OfflineRegionStatus {
	return OfflineRegionStatus{
		DownloadState:                  OfflineRegionDownloadState(status.download_state),
		RawDownloadState:               uint32(status.download_state),
		CompletedResourceCount:         uint64(status.completed_resource_count),
		CompletedResourceSize:          uint64(status.completed_resource_size),
		CompletedTileCount:             uint64(status.completed_tile_count),
		RequiredTileCount:              uint64(status.required_tile_count),
		CompletedTileSize:              uint64(status.completed_tile_size),
		RequiredResourceCount:          uint64(status.required_resource_count),
		RequiredResourceCountIsPrecise: bool(status.required_resource_count_is_precise),
		Complete:                       bool(status.complete),
	}
}

// StartAmbientCacheOperation starts a native ambient cache maintenance
// operation.
func (runtime *RuntimeHandle) StartAmbientCacheOperation(operation AmbientCacheOperation) (*OfflineOperationHandle[struct{}], error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var id C.mln_offline_operation_id
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_run_ambient_cache_operation_start(C.mln_runtime(ptr), C.uint32_t(operation), &id))
	}); err != nil {
		return nil, err
	}
	if id == 0 {
		return nil, newBindingError(ErrInvalidState, "ambient cache operation did not return an ID")
	}
	return newOfflineOperationHandle[struct{}](runtime, uint64(id), OfflineOperationAmbientCache, OfflineOperationResultNone), nil
}

// StartSetMaximumAmbientCacheSize starts a change to this runtime's maximum
// ambient cache size. Lowering it evicts ambient resources to fit the new
// budget; offline regions are unaffected.
func (runtime *RuntimeHandle) StartSetMaximumAmbientCacheSize(size uint64) (*OfflineOperationHandle[struct{}], error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var id C.mln_offline_operation_id
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_set_maximum_ambient_cache_size_start(C.mln_runtime(ptr), C.uint64_t(size), &id))
	}); err != nil {
		return nil, err
	}
	if id == 0 {
		return nil, newBindingError(ErrInvalidState, "maximum ambient cache size operation did not return an ID")
	}
	return newOfflineOperationHandle[struct{}](runtime, uint64(id), OfflineOperationSetMaximumAmbientCacheSize, OfflineOperationResultNone), nil
}

// SetResourceProvider installs or replaces the runtime-scoped network resource
// provider, and may be called while maps are live. Native code may invoke the
// provider on worker or network threads, so callbacks must be thread-safe and
// must not call MapLibre map or runtime APIs. Once this call returns, a
// replaced provider is no longer invoked; requests it already took a handle for
// keep that handle, so complete or close each one as usual.
func (runtime *RuntimeHandle) SetResourceProvider(provider ResourceProviderCallback) error {
	if provider == nil {
		return newBindingError(ErrInvalidArgument, "ResourceProviderCallback is nil")
	}
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var replacement *callback.ResourceProviderState
	if err := checkNative(func() int32 {
		state, status := callback.SetResourceProvider(uint64(ptr), func(request callback.ResourceRequest, handle *callback.ResourceRequestHandle) uint32 {
			decision := provider(ResourceRequest{
				RequestedURL:        request.RequestedURL,
				ResolvedURL:         request.ResolvedURL,
				Kind:                ResourceKind(request.Kind),
				RawKind:             request.Kind,
				LoadingMethod:       ResourceLoadingMethod(request.LoadingMethod),
				Priority:            ResourcePriority(request.Priority),
				Usage:               ResourceUsage(request.Usage),
				StoragePolicy:       ResourceStoragePolicy(request.StoragePolicy),
				HasRange:            request.HasRange,
				RangeStart:          request.RangeStart,
				RangeEnd:            request.RangeEnd,
				HasPriorModified:    request.HasPriorModified,
				PriorModifiedUnixMS: request.PriorModifiedUnixMS,
				HasPriorExpires:     request.HasPriorExpires,
				PriorExpiresUnixMS:  request.PriorExpiresUnixMS,
				PriorETag:           request.PriorETag,
				PriorData:           request.PriorData,
			}, newResourceRequestHandle(handle))
			return rawResourceProviderDecision(decision)
		})
		replacement = state
		return status
	}); err != nil {
		return err
	}

	runtime.resourceProviderMu.Lock()
	previous := runtime.resourceProvider
	runtime.resourceProvider = replacement
	runtime.resourceProviderMu.Unlock()
	previous.Release()
	return nil
}

// ClearResourceProvider clears the runtime-scoped network resource provider, so
// later requests go to MapLibre's online file source. Once this call returns,
// the cleared provider is no longer invoked; requests it already took a handle
// for keep that handle, so complete or close each one as usual.
func (runtime *RuntimeHandle) ClearResourceProvider() error {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()

	if err := checkNative(func() int32 { return callback.ClearResourceProvider(uint64(ptr)) }); err != nil {
		return err
	}
	runtime.releaseResourceProvider()
	return nil
}

func (runtime *RuntimeHandle) releaseResourceProvider() {
	runtime.resourceProviderMu.Lock()
	previous := runtime.resourceProvider
	runtime.resourceProvider = nil
	runtime.resourceProviderMu.Unlock()
	previous.Release()
}

// SetResourceTransform installs or replaces the runtime-scoped network URL
// transform. Native code may invoke the transform on worker or network threads,
// so callbacks must be thread-safe and must not call MapLibre map/runtime APIs.
func (runtime *RuntimeHandle) SetResourceTransform(transform ResourceTransformCallback) error {
	if transform == nil {
		return newBindingError(ErrInvalidArgument, "ResourceTransformCallback is nil")
	}
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var replacement *callback.ResourceTransformState
	if err := checkNative(func() int32 {
		state, status := callback.SetResourceTransform(uint64(ptr), func(kind uint32, url string) (string, bool) {
			return transform(ResourceTransformRequest{Kind: ResourceKind(kind), RawKind: kind, URL: url})
		})
		replacement = state
		return status
	}); err != nil {
		return err
	}

	runtime.resourceTransformMu.Lock()
	previous := runtime.resourceTransform
	runtime.resourceTransform = replacement
	runtime.resourceTransformMu.Unlock()
	previous.Release()
	return nil
}

// ClearResourceTransform clears the runtime-scoped network URL transform.
func (runtime *RuntimeHandle) ClearResourceTransform() error {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()

	if err := checkNative(func() int32 { return callback.ClearResourceTransform(uint64(ptr)) }); err != nil {
		return err
	}
	runtime.releaseResourceTransform()
	return nil
}

func (runtime *RuntimeHandle) releaseResourceTransform() {
	runtime.resourceTransformMu.Lock()
	previous := runtime.resourceTransform
	runtime.resourceTransform = nil
	runtime.resourceTransformMu.Unlock()
	previous.Release()
}

// SetHttpHeaderTransform installs or replaces the runtime-scoped outgoing HTTP
// header transform.
func (runtime *RuntimeHandle) SetHttpHeaderTransform(transform HttpHeaderTransformCallback) error {
	if transform == nil {
		return newBindingError(ErrInvalidArgument, "HttpHeaderTransformCallback is nil")
	}
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var replacement *callback.HttpHeaderTransformState
	if err := checkNative(func() int32 {
		state, status := callback.SetHttpHeaderTransform(uint64(ptr), func(kind uint32, url string) []callback.HttpHeader {
			provided := transform(HttpHeaderTransformRequest{Kind: ResourceKind(kind), RawKind: kind, URL: url})
			headers := make([]callback.HttpHeader, len(provided))
			for index, header := range provided {
				headers[index] = callback.HttpHeader{Name: header.Name, Value: header.Value}
			}
			return headers
		})
		replacement = state
		return status
	}); err != nil {
		return err
	}
	runtime.httpHeaderTransformMu.Lock()
	previous := runtime.httpHeaderTransform
	runtime.httpHeaderTransform = replacement
	runtime.httpHeaderTransformMu.Unlock()
	previous.Release()
	return nil
}

// ClearHttpHeaderTransform clears the runtime-scoped outgoing HTTP header transform.
func (runtime *RuntimeHandle) ClearHttpHeaderTransform() error {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer release()
	defer runtime.state.KeepAlive()
	if err := checkNative(func() int32 { return callback.ClearHttpHeaderTransform(uint64(ptr)) }); err != nil {
		return err
	}
	runtime.releaseHttpHeaderTransform()
	return nil
}

func (runtime *RuntimeHandle) releaseHttpHeaderTransform() {
	runtime.httpHeaderTransformMu.Lock()
	previous := runtime.httpHeaderTransform
	runtime.httpHeaderTransform = nil
	runtime.httpHeaderTransformMu.Unlock()
	previous.Release()
}

// NewMap creates a map owned by this runtime with native default options.
func (runtime *RuntimeHandle) NewMap() (*MapHandle, error) {
	return runtime.createMap(func(ptr nativeRuntime, out *nativeMap) int32 {
		var raw C.mln_map
		status := int32(C.mln_map_create(C.mln_runtime(ptr), nil, &raw))
		if status == int32(C.MLN_STATUS_OK) {
			*out = nativeMap(raw)
		}
		return status
	})
}

// NewMapWithOptions creates a map owned by this runtime with explicit options.
// Start from NewMapOptions to keep every map-originated event type selected; a
// zero-value MapOptions queues no event.
func (runtime *RuntimeHandle) NewMapWithOptions(options MapOptions) (*MapHandle, error) {
	return runtime.createMap(func(ptr nativeRuntime, out *nativeMap) int32 {
		rawOptions := C.mln_map_options_default()
		rawOptions.width = C.uint32_t(options.Width)
		rawOptions.height = C.uint32_t(options.Height)
		rawOptions.scale_factor = C.double(options.ScaleFactor)
		rawOptions.map_mode = C.uint32_t(options.Mode)
		rawOptions.fast_pfor_enabled = C.bool(options.FastPFOREnabled)
		rawOptions.event_mask = C.uint64_t(options.EventMask)

		var raw C.mln_map
		status := int32(C.mln_map_create(C.mln_runtime(ptr), &rawOptions, &raw))
		if status == int32(C.MLN_STATUS_OK) {
			*out = nativeMap(raw)
		}
		return status
	})
}

func (runtime *RuntimeHandle) createMap(create func(nativeRuntime, *nativeMap) int32) (*MapHandle, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var rawMap nativeMap
	if err := checkNative(func() int32 { return create(ptr, &rawMap) }); err != nil {
		return nil, err
	}
	state, err := handle.New(rawMap, "MapHandle", runtime)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	m := &MapHandle{
		state:        state,
		runtime:      runtime,
		runtimeChild: runtime.state.AddChild(),
		id:           MapID(rawMap),
	}
	runtime.registerMap(m)
	return m, nil
}

// Close destroys this runtime. A successful close makes later calls no-ops. A
// failed close leaves the native handle live so callers can retry on the owner
// thread.
func (runtime *RuntimeHandle) Close() error {
	if runtime == nil || runtime.state == nil {
		return newBindingError(ErrInvalidArgument, "RuntimeHandle is nil")
	}
	var bindingErr error
	if err := checkNative(func() int32 {
		status, err := runtime.state.CloseChecked(func(native nativeRuntime) int32 {
			return destroyRuntimeHandle(native)
		})
		if err != nil {
			if errors.Is(err, handle.ErrLiveChildren) {
				bindingErr = newBindingError(ErrInvalidState, "RuntimeHandle has live child handles")
				return int32(C.MLN_STATUS_OK)
			}
			bindingErr = newBindingError(ErrInvalidState, err.Error())
			return int32(C.MLN_STATUS_OK)
		}
		return status
	}); err != nil {
		return err
	}
	if bindingErr != nil {
		return bindingErr
	}
	runtime.releaseResourceTransform()
	runtime.releaseHttpHeaderTransform()
	runtime.releaseResourceProvider()
	runtime.mapsMu.Lock()
	runtime.maps = nil
	runtime.mapsMu.Unlock()
	return nil
}
