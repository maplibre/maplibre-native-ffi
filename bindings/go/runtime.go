package maplibre

/*
#include <stdlib.h>

#include "internal/cgo_runtime_shim.h"
*/
import "C"

import (
	"sync"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
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

// RuntimeOptions configures runtime creation.
type RuntimeOptions struct {
	// AssetPath is the directory MapLibre resolves asset:// URLs against.
	AssetPath string
	// CachePath is the file path of the runtime's ambient cache and offline
	// database.
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
	if err := validateCStringArgument("RuntimeOptions.AssetPath", options.AssetPath); err != nil {
		return err
	}
	return validateCStringArgument("RuntimeOptions.CachePath", options.CachePath)
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
	RuntimeEventMapCameraTransitionFinished         RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED)
)

// RuntimeEventMask selects which event types a map or a runtime queues. An event
// whose type is unselected is never built, queued, or reported ready.
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
	// or the ordinal of MapLibre Native's internal map load error kind for
	// map-loading-failed. Every other event type reports 0.
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

// CommandDisposition identifies a command's terminal disposition.
type CommandDisposition uint32

const (
	CommandDispositionCommitted  CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_COMMITTED)
	CommandDispositionSuperseded CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_SUPERSEDED)
	CommandDispositionFailed     CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_FAILED)
	CommandDispositionCancelled  CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_CANCELLED)
)

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

// RuntimeEventUnknownPayload contains copied bytes for a payload type unknown to
// this Go binding version. Bytes is the event's whole payload window, which is
// the batch's event stride minus this binding's payload offset.
type RuntimeEventUnknownPayload struct {
	Bytes []byte
}

// RuntimeHandle owns autonomous scheduler state and event storage.
type RuntimeHandle struct {
	state *handle.State[nativeRuntime]

	mapsMu sync.Mutex
	// Resolves an event's source id to the public wrapper.
	maps map[MapID]*MapHandle
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

// NewRuntime creates a runtime using native defaults. Native worker threads
// make progress independently of the calling goroutine.
func NewRuntime() (*RuntimeHandle, error) {
	return NewRuntimeWithOptions(NewRuntimeOptions("", ""))
}

// NewRuntimeWithOptions creates a runtime using explicit options. Start from
// NewRuntimeOptions to keep every event type selected; a zero-value
// RuntimeOptions queues no event.
//
// The calling goroutine blocks until the runtime worker finishes initializing.
func NewRuntimeWithOptions(options RuntimeOptions) (*RuntimeHandle, error) {
	if err := options.validate(); err != nil {
		return nil, err
	}
	if err := checkCompatibleCABI(CVersion()); err != nil {
		return nil, err
	}
	rawOptions := C.mln_runtime_options_default()
	assetPath := C.CString(options.AssetPath)
	defer C.free(unsafe.Pointer(assetPath))
	cachePath := C.CString(options.CachePath)
	defer C.free(unsafe.Pointer(cachePath))
	rawOptions.asset_path = assetPath
	rawOptions.cache_path = cachePath
	rawOptions.event_mask = C.uint64_t(options.EventMask)

	var raw C.mln_runtime
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_create(&rawOptions, &raw))
	}); err != nil {
		return nil, err
	}
	state, err := handle.New(nativeRuntime(raw), "RuntimeHandle")
	if err != nil {
		// The wrapper never became visible, so no caller can await this future.
		_, _ = startNativeRuntimeRelease(nativeRuntime(raw))
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &RuntimeHandle{state: state}, nil
}

// startNativeRuntimeRelease releases a native runtime and returns the future
// that completes after its teardown finishes.
func startNativeRuntimeRelease(runtime nativeRuntime) (*Future[struct{}], error) {
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_runtime_release(C.mln_runtime(runtime), completion))
	}, completionUnit)
}

func (runtime *RuntimeHandle) ptr() (nativeRuntime, error) {
	if runtime == nil || runtime.state == nil {
		return 0, newBindingError(ErrInvalidArgument, "RuntimeHandle is nil")
	}
	value, live := runtime.state.Handle()
	if !live {
		return 0, newBindingError(ErrInvalidArgument, "RuntimeHandle is closed")
	}
	return value, nil
}

// runtimeEventMaskByIDForTest calls the C runtime accessor with a raw handle
// id, so a test can pass an id of another handle kind. The safe API cannot
// express a raw id.
func runtimeEventMaskByIDForTest(id nativeRuntime) error {
	var mask C.uint64_t
	return checkNative(func() int32 {
		return int32(C.mln_runtime_get_event_mask(C.mln_runtime(id), &mask))
	})
}

// Barrier starts an ordered runtime operation that completes after every
// command accepted before it reaches a terminal disposition.
func (runtime *RuntimeHandle) Barrier() (*Future[struct{}], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_barrier(handle, out))
	}, completionUnit)
}

// DrainEvents takes this runtime's whole event queue in queue order. Every
// field of every event is copied out of runtime-owned storage and the native
// batch is released before the call returns, so the events stay readable after
// the next drain. An empty queue yields no events.
func (runtime *RuntimeHandle) DrainEvents() ([]RuntimeEvent, error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	rawBatch, ownedBatch, err := drainRawEvents(ptr)
	if err != nil {
		return nil, err
	}
	defer C.mln_event_batch_release(ownedBatch)
	return runtime.copyEvents(rawBatch), nil
}

// SetEventMask selects which runtime-originated event types this runtime queues.
// It accepts RuntimeEventMaskAll, reads the bits in
// RuntimeEventMaskAllRuntimeEvents, and returns ErrInvalidArgument for a bit
// outside RuntimeEventMaskAll.
//
// Narrowing gates later events and keeps queued ones, so a caller drains what it
// already caused.
func (runtime *RuntimeHandle) SetEventMask(mask RuntimeEventMask) error {
	ptr, err := runtime.ptr()
	if err != nil {
		return err
	}

	defer runtime.state.KeepAlive()

	return checkNative(func() int32 {
		return int32(C.mln_runtime_set_event_mask(C.mln_runtime(ptr), C.uint64_t(mask)))
	})
}

// EventMask reports which runtime-originated event types this runtime queues. A
// runtime that has not been narrowed reports RuntimeEventMaskAll.
func (runtime *RuntimeHandle) EventMask() (RuntimeEventMask, error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return 0, err
	}

	defer runtime.state.KeepAlive()

	var raw C.uint64_t
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_get_event_mask(C.mln_runtime(ptr), &raw))
	}); err != nil {
		return 0, err
	}
	return RuntimeEventMask(raw), nil
}

func drainRawEvents(ptr nativeRuntime) (C.mln_runtime_event_batch_view, C.mln_event_batch, error) {
	var batch C.mln_event_batch
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_drain_events(C.mln_runtime(ptr), &batch))
	}); err != nil {
		return C.mln_runtime_event_batch_view{}, 0, err
	}
	view := C.mln_runtime_event_batch_view{size: C.uint32_t(C.sizeof_mln_runtime_event_batch_view)}
	if err := checkNative(func() int32 { return int32(C.mln_event_batch_get(batch, &view)) }); err != nil {
		C.mln_event_batch_release(batch)
		return C.mln_runtime_event_batch_view{}, 0, err
	}
	return view, batch, nil
}

// runtimeEventPayloadOffset is where the payload union starts inside an event
// record. Every field before it keeps its offset across C API versions, and the
// batch's event stride minus this offset is the payload window.
var runtimeEventPayloadOffset = unsafe.Offsetof(C.mln_runtime_event{}.payload)

// copyEvents copies a borrowed native batch view into owned Go values. It
// resolves map sources in one short critical section after the copy, so a drain
// never holds the map registry lock while native map creation wants it.
func (runtime *RuntimeHandle) copyEvents(raw C.mln_runtime_event_batch_view) []RuntimeEvent {
	count := int(raw.event_count)
	if count <= 0 || raw.events == nil {
		return nil
	}
	// The stride comes from the batch, never from this binding's own event size,
	// so a C API version that widens the payload union stays readable.
	stride := uintptr(raw.event_size)
	base := unsafe.Pointer(raw.events)
	payloadWindow := uintptr(0)
	if stride > runtimeEventPayloadOffset {
		payloadWindow = stride - runtimeEventPayloadOffset
	}

	events := make([]RuntimeEvent, count)
	var mapSourced []int
	for index := range events {
		eventPtr := unsafe.Add(base, uintptr(index)*stride)
		rawEvent := (*C.mln_runtime_event)(eventPtr)
		source := RuntimeEventSource{
			Type:  RuntimeEventSourceType(rawEvent.source_type),
			RawID: uint64(rawEvent.source),
		}
		if source.Type == RuntimeEventSourceMap {
			mapSourced = append(mapSourced, index)
		}
		events[index] = RuntimeEvent{
			Type:        RuntimeEventType(rawEvent._type),
			SourceType:  source.Type,
			Source:      source,
			Code:        int32(rawEvent.code),
			PayloadType: RuntimeEventPayloadType(rawEvent.payload_type),
			Message:     runtimeEventMessage(raw, rawEvent),
			Payload:     runtimeEventPayloadFromC(rawEvent, unsafe.Add(eventPtr, runtimeEventPayloadOffset), payloadWindow),
		}
	}
	if len(mapSourced) == 0 {
		return events
	}

	runtime.mapsMu.Lock()
	for _, index := range mapSourced {
		if sourceMap := runtime.maps[MapID(events[index].Source.RawID)]; sourceMap != nil {
			events[index].Source.MapID = sourceMap.id
		}
	}
	runtime.mapsMu.Unlock()
	return events
}

func runtimeEventMessage(batch C.mln_runtime_event_batch_view, event *C.mln_runtime_event) string {
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

// AmbientCacheOperation starts one ambient cache maintenance operation. The
// call validates its arguments and returns without waiting for the runtime
// worker; a database failure reports ErrNative through the returned future.
func (runtime *RuntimeHandle) AmbientCacheOperation(operation AmbientCacheOperation) (*Future[struct{}], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_run_ambient_cache_operation(handle, C.uint32_t(operation), out))
	}, completionUnit)
}

// SetMaximumAmbientCacheSize starts a change to this runtime's maximum ambient
// cache size. Lowering it evicts ambient resources to fit the new budget, and
// leaves offline regions in place. The call returns without waiting for the
// runtime worker; a database failure reports ErrNative through the returned
// future.
func (runtime *RuntimeHandle) SetMaximumAmbientCacheSize(size uint64) (*Future[struct{}], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_set_maximum_ambient_cache_size(handle, C.uint64_t(size), out))
	}, completionUnit)
}

// SetResourceProvider submits a runtime-scoped network resource provider.
// Native code may invoke the provider on worker or network threads. The
// callback must be thread-safe and must not call map or runtime APIs. The
// The returned future resolves after the provider change becomes terminal.
func (runtime *RuntimeHandle) SetResourceProvider(provider ResourceProviderCallback) (*Future[CommandCompletion], error) {
	if provider == nil {
		return nil, newBindingError(ErrInvalidArgument, "ResourceProviderCallback is nil")
	}
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.SetResourceProvider(uint64(ptr), func(request callback.ResourceRequest, handle *callback.ResourceRequestHandle) uint32 {
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
		}, unsafe.Pointer(completion))
	}, completionCommand)
}

// ClearResourceProvider submits removal of the runtime-scoped network resource
// provider.
func (runtime *RuntimeHandle) ClearResourceProvider() (*Future[CommandCompletion], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.ClearResourceProvider(uint64(ptr), unsafe.Pointer(completion))
	}, completionCommand)
}

// SetResourceTransform submits a runtime-scoped network URL transform. Native
// code may invoke the transform on worker or network threads. The callback must
// be thread-safe and must not call map or runtime APIs.
func (runtime *RuntimeHandle) SetResourceTransform(transform ResourceTransformCallback) (*Future[CommandCompletion], error) {
	if transform == nil {
		return nil, newBindingError(ErrInvalidArgument, "ResourceTransformCallback is nil")
	}
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.SetResourceTransform(uint64(ptr), func(kind uint32, url string) (string, bool) {
			return transform(ResourceTransformRequest{Kind: ResourceKind(kind), RawKind: kind, URL: url})
		}, unsafe.Pointer(completion))
	}, completionCommand)
}

// ClearResourceTransform submits removal of the runtime-scoped network URL
// transform.
func (runtime *RuntimeHandle) ClearResourceTransform() (*Future[CommandCompletion], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.ClearResourceTransform(uint64(ptr), unsafe.Pointer(completion))
	}, completionCommand)
}

// SetHttpHeaderTransform submits a runtime-scoped outgoing HTTP header
// transform.
func (runtime *RuntimeHandle) SetHttpHeaderTransform(transform HttpHeaderTransformCallback) (*Future[CommandCompletion], error) {
	if transform == nil {
		return nil, newBindingError(ErrInvalidArgument, "HttpHeaderTransformCallback is nil")
	}
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.SetHttpHeaderTransform(uint64(ptr), func(kind uint32, url string) []callback.HttpHeader {
			provided := transform(HttpHeaderTransformRequest{Kind: ResourceKind(kind), RawKind: kind, URL: url})
			headers := make([]callback.HttpHeader, len(provided))
			for index, header := range provided {
				headers[index] = callback.HttpHeader{Name: header.Name, Value: header.Value}
			}
			return headers
		}, unsafe.Pointer(completion))
	}, completionCommand)
}

// ClearHttpHeaderTransform submits removal of the runtime-scoped outgoing HTTP
// header transform.
func (runtime *RuntimeHandle) ClearHttpHeaderTransform() (*Future[CommandCompletion], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.ClearHttpHeaderTransform(uint64(ptr), unsafe.Pointer(completion))
	}, completionCommand)
}

// NewMap creates a 512 by 512 logical map with native default options.
func (runtime *RuntimeHandle) NewMap() (*Future[*MapHandle], error) {
	return runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
}

// NewMapWithOptions creates a map owned by this runtime with explicit options.
// Start from NewMapOptions to keep every map-originated event type selected; a
// zero-value MapOptions queues no event.
func (runtime *RuntimeHandle) NewMapWithOptions(options MapOptions) (*Future[*MapHandle], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	rawOptions := C.mln_map_options_default()
	rawOptions.initial_extent = C.mln_logical_extent{
		width:        C.uint32_t(options.Width),
		height:       C.uint32_t(options.Height),
		scale_factor: C.double(options.ScaleFactor),
	}
	rawOptions.map_mode = C.uint32_t(options.Mode)
	rawOptions.fast_pfor_enabled = C.bool(options.FastPFOREnabled)
	rawOptions.event_mask = C.uint64_t(options.EventMask)

	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_create(C.mln_runtime(ptr), &rawOptions, completion))
	}, func(result *C.mln_completion_result) (*MapHandle, error) {
		raw, err := completionValue[C.mln_map](result)
		if err != nil {
			return nil, err
		}
		state, err := handle.New(nativeMap(raw), "MapHandle")
		if err != nil {
			// The wrapper never became visible, so no caller can await this future.
			_, _ = startCompletion(func(completion *C.mln_completion) int32 {
				return int32(C.mln_map_release(C.mln_map(raw), completion))
			}, completionUnit)
			return nil, newBindingError(ErrInvalidArgument, err.Error())
		}
		m := &MapHandle{state: state, runtime: runtime, id: MapID(raw)}
		runtime.registerMap(m)
		return m, nil
	})
}

// Close releases this runtime's public native handle and returns the future for
// its native teardown. Native teardown continues in submission order after this
// method returns; the future completes once every accepted submission,
// including released maps' teardown, has finished and the runtime's threads and
// resources are gone. A host that awaits it may exit the process without racing
// native teardown, and a host that outlives its runtimes may drop it. Closing an
// already closed runtime returns a future that has already completed. A failed
// close returns no future and leaves the handle live so callers can correct the
// native precondition and retry.
func (runtime *RuntimeHandle) Close() (*Future[struct{}], error) {
	if runtime == nil || runtime.state == nil {
		return nil, newBindingError(ErrInvalidArgument, "RuntimeHandle is nil")
	}
	// A closed handle leaves teardown unset, because its native release already
	// ran for an earlier caller.
	teardown := completedFuture(struct{}{})
	if err := runtime.state.Close(func(native nativeRuntime) error {
		future, err := startNativeRuntimeRelease(native)
		if err != nil {
			return err
		}
		teardown = future
		return nil
	}); err != nil {
		return nil, err
	}
	runtime.mapsMu.Lock()
	runtime.maps = nil
	runtime.mapsMu.Unlock()
	return teardown, nil
}
