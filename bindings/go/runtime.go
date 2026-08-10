package maplibre

/*
#include <stdlib.h>
#include "maplibre_native_c.h"
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
}

// Equal reports whether two descriptors hold the same field values.
func (options RuntimeOptions) Equal(other RuntimeOptions) bool {
	return options.AssetPath == other.AssetPath &&
		options.CachePath == other.CachePath
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
	RuntimeEventPayloadStyleImageMissing           RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING)
	RuntimeEventPayloadTileAction                  RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION)
	RuntimeEventPayloadOfflineRegionStatus         RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS)
	RuntimeEventPayloadOfflineRegionResponseError  RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR)
	RuntimeEventPayloadOfflineRegionTileCountLimit RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT)
	RuntimeEventPayloadOfflineOperationCompleted   RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED)
	RuntimeEventPayloadCameraTransitionFinished    RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED)
)

// MapID identifies a map within one RuntimeHandle.
type MapID uint64

// RuntimeEventSource identifies the runtime object that emitted an event without
// exposing native handle addresses.
type RuntimeEventSource struct {
	Type  RuntimeEventSourceType
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
	PayloadSize uintptr
	Message     string
	Payload     any

	rawSource MapID
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

// RuntimeEventStyleImageMissingPayload is a copied style-image-missing event payload.
type RuntimeEventStyleImageMissingPayload struct {
	ImageID string
}

// RuntimeEventTileActionPayload is a copied tile-action event payload.
type RuntimeEventTileActionPayload struct {
	Operation    TileOperation
	RawOperation uint32
	TileID       TileID
	SourceID     string
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
// this Go binding version.
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
// explicit options.
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
// enqueue. Drain the queued runtime events with PollEvent afterwards.
//
// timeout sets the park bound: zero drains and returns, a positive value parks
// for up to that long, and a negative value parks until a wake arrives. A
// parking call returns as soon as the runtime's wake flag is set and clears it,
// and returns without parking while unread runtime events are queued. Timers
// and ready file descriptors set the flag only when they queue owner-thread
// work, so pass a bounded timeout to cap how long a call waits.
//
// A non-zero timeout blocks the calling goroutine and its OS thread. Call it
// outside any lock that a goroutine signalling a WakeSource takes.
func (runtime *RuntimeHandle) Pump(timeout time.Duration) error {
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
	return checkNative(func() int32 {
		return int32(C.mln_runtime_pump(C.mln_runtime(ptr), C.int64_t(timeoutMS)))
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

// PollEvent polls one queued runtime event and copies it into a Go value. It
// reports a nil event when the queue is empty.
//
// Polling a map-style-loaded event releases that map's detached custom geometry
// sources, so drain the queue to keep dropped sources from lingering.
func (runtime *RuntimeHandle) PollEvent() (*RuntimeEvent, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	rawEvent := C.mln_runtime_event{size: C.uint32_t(unsafe.Sizeof(C.mln_runtime_event{}))}
	var hasEvent C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_poll_event(C.mln_runtime(ptr), &rawEvent, &hasEvent))
	}); err != nil {
		return nil, err
	}
	if !bool(hasEvent) {
		return nil, nil
	}
	event := runtime.runtimeEventFromC(rawEvent)
	runtime.handleEventSideEffects(event)
	return event, nil
}

func runtimeEventFromC(event C.mln_runtime_event) *RuntimeEvent {
	return runtimeEventFromCWithSource(event, RuntimeEventSource{Type: RuntimeEventSourceType(event.source_type)})
}

func (runtime *RuntimeHandle) runtimeEventFromC(event C.mln_runtime_event) *RuntimeEvent {
	source := RuntimeEventSource{Type: RuntimeEventSourceType(event.source_type)}
	if source.Type == RuntimeEventSourceMap {
		if m := runtime.mapForEventSource(MapID(event.source)); m != nil {
			source.MapID = m.id
		}
	}
	return runtimeEventFromCWithSource(event, source)
}

func runtimeEventFromCWithSource(event C.mln_runtime_event, source RuntimeEventSource) *RuntimeEvent {
	return &RuntimeEvent{
		Type:        RuntimeEventType(event._type),
		SourceType:  RuntimeEventSourceType(event.source_type),
		Source:      source,
		Code:        int32(event.code),
		PayloadType: RuntimeEventPayloadType(event.payload_type),
		PayloadSize: uintptr(event.payload_size),
		Message:     goCharBytes(unsafe.Pointer(event.message), event.message_size),
		Payload:     runtimeEventPayloadFromC(event),
		rawSource:   MapID(event.source),
	}
}

func (runtime *RuntimeHandle) handleEventSideEffects(event *RuntimeEvent) {
	if event == nil || event.Type != RuntimeEventMapStyleLoaded || event.SourceType != RuntimeEventSourceMap {
		return
	}
	m := runtime.mapForEventSource(event.rawSource)
	if m != nil {
		m.releaseDetachedCustomGeometrySources()
	}
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

func (runtime *RuntimeHandle) mapForEventSource(source MapID) *MapHandle {
	if runtime == nil || source == 0 {
		return nil
	}
	runtime.mapsMu.Lock()
	m := runtime.maps[source]
	runtime.mapsMu.Unlock()
	return m
}

func runtimeEventPayloadFromC(event C.mln_runtime_event) any {
	if event.payload == nil {
		return nil
	}
	switch uint32(event.payload_type) {
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_render_frame{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_render_frame)(event.payload)
		mode := uint32(payload.mode)
		return RuntimeEventRenderFramePayload{
			Mode:             RenderMode(mode),
			RawMode:          mode,
			NeedsRepaint:     bool(payload.needs_repaint),
			PlacementChanged: bool(payload.placement_changed),
			Stats:            renderingStatsFromC(payload.stats),
		}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_render_map{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_render_map)(event.payload)
		mode := uint32(payload.mode)
		return RuntimeEventRenderMapPayload{Mode: RenderMode(mode), RawMode: mode}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_style_image_missing{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_style_image_missing)(event.payload)
		return RuntimeEventStyleImageMissingPayload{ImageID: goCharBytes(unsafe.Pointer(payload.image_id), payload.image_id_size)}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_tile_action{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_tile_action)(event.payload)
		operation := uint32(payload.operation)
		return RuntimeEventTileActionPayload{
			Operation:    TileOperation(operation),
			RawOperation: operation,
			TileID:       tileIDFromC(payload.tile_id),
			SourceID:     goCharBytes(unsafe.Pointer(payload.source_id), payload.source_id_size),
		}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_camera_transition_finished{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_camera_transition_finished)(event.payload)
		return RuntimeEventCameraTransitionFinishedPayload{TransitionID: uint64(payload.transition_id)}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_offline_region_status{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_offline_region_status)(event.payload)
		return RuntimeEventOfflineRegionStatusPayload{RegionID: OfflineRegionID(payload.region_id), Status: offlineRegionStatusFromC(payload.status)}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_offline_region_response_error{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_offline_region_response_error)(event.payload)
		reason := uint32(payload.reason)
		return RuntimeEventOfflineRegionResponseErrorPayload{RegionID: OfflineRegionID(payload.region_id), Reason: ResourceErrorReason(reason), RawReason: reason}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_offline_region_tile_count_limit{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_offline_region_tile_count_limit)(event.payload)
		return RuntimeEventOfflineRegionTileCountLimitPayload{RegionID: OfflineRegionID(payload.region_id), Limit: uint64(payload.limit)}
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED):
		if !runtimeEventPayloadHasSize(event, unsafe.Sizeof(C.mln_runtime_event_offline_operation_completed{})) {
			return runtimeEventUnknownPayloadFromC(event)
		}
		payload := (*C.mln_runtime_event_offline_operation_completed)(event.payload)
		return RuntimeEventOfflineOperationCompletedPayload{
			OperationID:   uint64(payload.operation_id),
			OperationKind: OfflineOperationKind(payload.operation_kind),
			ResultKind:    OfflineOperationResultKind(payload.result_kind),
			ResultStatus:  int32(payload.result_status),
			Found:         bool(payload.found),
		}
	default:
		return runtimeEventUnknownPayloadFromC(event)
	}
}

func runtimeEventPayloadHasSize(event C.mln_runtime_event, required uintptr) bool {
	return uintptr(event.payload_size) >= required
}

func runtimeEventUnknownPayloadFromC(event C.mln_runtime_event) RuntimeEventUnknownPayload {
	bytes, ok := goByteSlice(event.payload, event.payload_size)
	if !ok {
		return RuntimeEventUnknownPayload{}
	}
	return RuntimeEventUnknownPayload{Bytes: bytes}
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
func (runtime *RuntimeHandle) NewMapWithOptions(options MapOptions) (*MapHandle, error) {
	return runtime.createMap(func(ptr nativeRuntime, out *nativeMap) int32 {
		rawOptions := C.mln_map_options_default()
		rawOptions.width = C.uint32_t(options.Width)
		rawOptions.height = C.uint32_t(options.Height)
		rawOptions.scale_factor = C.double(options.ScaleFactor)
		rawOptions.map_mode = C.uint32_t(options.Mode)
		rawOptions.fast_pfor_enabled = C.bool(options.FastPFOREnabled)

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
	m := &MapHandle{state: state, runtime: runtime, runtimeChild: runtime.state.AddChild(), id: MapID(rawMap)}
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
