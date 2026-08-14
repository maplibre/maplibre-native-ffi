package maplibre

/*
#include <stdlib.h>

#include "internal/cgo_runtime_shim.h"
*/
import "C"

import (
	"errors"
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

type operationKind uint8

const (
	operationAmbientCache operationKind = iota + 1
	operationRegionCreate
	operationRegionGet
	operationRegionsList
	operationRegionsMergeDatabase
	operationRegionUpdateMetadata
	operationRegionGetStatus
	operationRegionSetObserved
	operationRegionSetDownloadState
	operationRegionInvalidate
	operationRegionDelete
	operationSetMaximumAmbientCacheSize
	operationCameraQuery
	operationStillImage
	operationBarrier
)

type operationResultKind uint8

const (
	operationResultNone operationResultKind = iota
	operationResultRegion
	operationResultOptionalRegion
	operationResultRegionList
	operationResultRegionStatus
	operationResultCamera
)

// OperationHandle owns a common asynchronous native operation.
type OperationHandle[T any] struct {
	child          *handle.Child
	ownerChild     *handle.Child
	id             uint64
	kind           operationKind
	resultKind     operationResultKind
	takeResult     func(uint64) (T, bool, error)
	mu             sync.Mutex
	cond           *sync.Cond
	activeUses     int
	live           bool
	releasing      bool
	consuming      bool
	resultConsumed bool
}

func newOperationHandle[T any](runtime *RuntimeHandle, id uint64, kind operationKind, resultKind operationResultKind) *OperationHandle[T] {
	var child *handle.Child
	if runtime != nil && runtime.state != nil {
		child = runtime.state.AddChild()
	}
	operation := &OperationHandle[T]{child: child, id: id, kind: kind, resultKind: resultKind, live: true}
	operation.cond = sync.NewCond(&operation.mu)
	return operation
}

func newOwnedOperationHandle[T any](
	runtime *RuntimeHandle,
	ownerChild *handle.Child,
	id uint64,
	kind operationKind,
	resultKind operationResultKind,
) *OperationHandle[T] {
	operation := newOperationHandle[T](runtime, id, kind, resultKind)
	operation.ownerChild = ownerChild
	return operation
}

func (operation *OperationHandle[T]) beginUse() (uint64, error) {
	if operation == nil {
		return 0, newBindingError(ErrInvalidArgument, "OperationHandle is nil")
	}
	operation.mu.Lock()
	defer operation.mu.Unlock()
	if !operation.live || operation.releasing {
		return 0, newBindingError(ErrInvalidArgument, "OperationHandle is closed")
	}
	operation.activeUses++
	return operation.id, nil
}

func (operation *OperationHandle[T]) endUse() {
	operation.mu.Lock()
	operation.activeUses--
	operation.cond.Broadcast()
	operation.mu.Unlock()
}

func (operation *OperationHandle[T]) beginResultUse() (uint64, operationKind, operationResultKind, bool, error) {
	if operation == nil {
		return 0, 0, 0, false, newBindingError(ErrInvalidArgument, "OperationHandle is nil")
	}
	operation.mu.Lock()
	defer operation.mu.Unlock()
	for operation.consuming && operation.live && !operation.releasing {
		operation.cond.Wait()
	}
	if !operation.live || operation.releasing {
		return 0, 0, 0, false, newBindingError(ErrInvalidArgument, "OperationHandle is closed")
	}
	if operation.resultConsumed {
		return operation.id, operation.kind, operation.resultKind, true, nil
	}
	operation.consuming = true
	operation.activeUses++
	return operation.id, operation.kind, operation.resultKind, false, nil
}

func (operation *OperationHandle[T]) endResultUse(consumed bool) {
	operation.mu.Lock()
	if consumed {
		operation.resultConsumed = true
	}
	operation.consuming = false
	operation.activeUses--
	operation.cond.Broadcast()
	operation.mu.Unlock()
}

// Poll reports whether this operation reached a terminal disposition.
func (operation *OperationHandle[T]) Poll() (bool, error) {
	id, err := operation.beginUse()
	if err != nil {
		return false, err
	}
	defer operation.endUse()
	var completed C.bool
	err = checkNative(func() int32 {
		return int32(C.mln_operation_poll(C.mln_operation(id), &completed))
	})
	return bool(completed), err
}

// Wait waits up to timeout for this operation to complete. A negative timeout
// waits without a deadline. Cancel may run concurrently with Wait.
func (operation *OperationHandle[T]) Wait(timeout time.Duration) (bool, error) {
	id, err := operation.beginUse()
	if err != nil {
		return false, err
	}
	defer operation.endUse()
	timeoutMillis := int64(timeout / time.Millisecond)
	if timeout < 0 {
		timeoutMillis = -1
	}
	var completed C.bool
	err = checkNative(func() int32 {
		return int32(C.mln_operation_wait(C.mln_operation(id), C.int64_t(timeoutMillis), &completed))
	})
	return bool(completed), err
}

// Cancel requests cancellation of this operation.
func (operation *OperationHandle[T]) Cancel() error {
	id, err := operation.beginUse()
	if err != nil {
		return err
	}
	defer operation.endUse()
	return checkNative(func() int32 {
		return int32(C.mln_operation_cancel(C.mln_operation(id)))
	})
}

// Status returns the terminal native status of this completed operation.
func (operation *OperationHandle[T]) Status() (int32, error) {
	id, err := operation.beginUse()
	if err != nil {
		return 0, err
	}
	defer operation.endUse()
	var result C.mln_status
	err = checkNative(func() int32 {
		return int32(C.mln_operation_get_status(C.mln_operation(id), &result))
	})
	return int32(result), err
}

// Diagnostic returns a copy of this completed operation's diagnostic text.
func (operation *OperationHandle[T]) Diagnostic() (string, error) {
	rawID, err := operation.beginUse()
	if err != nil {
		return "", err
	}
	defer operation.endUse()
	id := C.mln_operation(rawID)
	var size C.size_t
	if err := checkNative(func() int32 {
		return int32(C.mln_operation_copy_diagnostic(id, nil, 0, &size))
	}); err != nil {
		return "", err
	}
	if size == 0 {
		return "", nil
	}
	if uint64(size) > uint64(^uint(0)>>1) {
		return "", newBindingError(ErrInvalidState, "operation diagnostic is too large")
	}
	buffer := make([]byte, int(size))
	if err := checkNative(func() int32 {
		return int32(C.mln_operation_copy_diagnostic(
			id,
			(*C.char)(unsafe.Pointer(&buffer[0])),
			C.size_t(len(buffer)),
			&size,
		))
	}); err != nil {
		return "", err
	}
	if uint64(size) > uint64(len(buffer)) {
		return "", newBindingError(ErrInvalidState, "operation diagnostic size changed while copying")
	}
	return string(buffer[:int(size)]), nil
}

// Discard destroys an untaken result from a completed operation. The operation
// remains live for status and diagnostic inspection until Release is called.
func (operation *OperationHandle[T]) Discard() error {
	id, _, _, consumed, err := operation.beginResultUse()
	if err != nil {
		return err
	}
	if consumed {
		return newBindingError(ErrInvalidState, "operation result was already consumed")
	}
	success := false
	defer func() { operation.endResultUse(success) }()
	if err := checkNative(func() int32 { return operationDiscard(id) }); err != nil {
		return err
	}
	success = true
	return nil
}

// Release detaches this observer and releases any untaken result. Releasing a
// pending operation requests cancellation when supported. Release waits for
// every native call that already started.
func (operation *OperationHandle[T]) Release() {
	if operation == nil {
		return
	}
	operation.mu.Lock()
	for operation.releasing {
		operation.cond.Wait()
	}
	if !operation.live {
		operation.mu.Unlock()
		return
	}
	operation.releasing = true
	for operation.activeUses > 0 {
		operation.cond.Wait()
	}
	id := operation.id
	child := operation.child
	ownerChild := operation.ownerChild
	operation.ownerChild = nil
	operation.child = nil
	operation.live = false
	operation.mu.Unlock()

	C.mln_operation_release(C.mln_operation(id))
	if child != nil {
		child.Release()
	}
	if ownerChild != nil {
		ownerChild.Release()
	}

	operation.mu.Lock()
	operation.releasing = false
	operation.cond.Broadcast()
	operation.mu.Unlock()
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
	RuntimeEventMapCameraTransitionFinished         RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED)
	RuntimeEventCommandFinished                     RuntimeEventType = RuntimeEventType(C.MLN_RUNTIME_EVENT_COMMAND_FINISHED)
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
	RuntimeEventMaskCommandFinished                     RuntimeEventMask = RuntimeEventMask(C.MLN_RUNTIME_EVENT_MASK_COMMAND_FINISHED)
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
	RuntimeEventPayloadCommandFinished             RuntimeEventPayloadType = RuntimeEventPayloadType(C.MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED)
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

// CommandDisposition identifies a command's terminal disposition.
type CommandDisposition uint32

const (
	CommandDispositionCommitted  CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_COMMITTED)
	CommandDispositionSuperseded CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_SUPERSEDED)
	CommandDispositionFailed     CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_FAILED)
	CommandDispositionCancelled  CommandDisposition = CommandDisposition(C.MLN_COMMAND_DISPOSITION_CANCELLED)
)

// RuntimeEventCommandFinishedPayload identifies the command that reached a
// terminal disposition and its committed generation, when it has one.
type RuntimeEventCommandFinishedPayload struct {
	CommandID      uint64
	Disposition    CommandDisposition
	RawDisposition uint32
	Generation     uint64
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

// RuntimeEventUnknownPayload contains copied bytes for a payload type unknown to
// this Go binding version. Bytes is the event's whole payload window, which is
// the batch's event stride minus this binding's payload offset.
type RuntimeEventUnknownPayload struct {
	Bytes []byte
}

type pendingResourceTransform struct {
	replacement *callback.ResourceTransformState
}

type pendingHttpHeaderTransform struct {
	replacement *callback.HttpHeaderTransformState
}

type pendingResourceProvider struct {
	replacement *callback.ResourceProviderState
}

// RuntimeHandle owns autonomous scheduler state and event storage.
type RuntimeHandle struct {
	state                *handle.State[nativeRuntime]
	notificationSource   nativeNotificationSource
	notificationMu       sync.Mutex
	notificationCallback uintptr

	resourceTransformMu        sync.Mutex
	resourceTransform          *callback.ResourceTransformState
	pendingResourceTransform   map[uint64]pendingResourceTransform
	httpHeaderTransformMu      sync.Mutex
	httpHeaderTransform        *callback.HttpHeaderTransformState
	pendingHttpHeaderTransform map[uint64]pendingHttpHeaderTransform
	resourceProviderMu         sync.Mutex
	resourceProvider           *callback.ResourceProviderState
	pendingResourceProvider    map[uint64]pendingResourceProvider
	mapsMu                     sync.Mutex
	// Resolves an event's source id to the public wrapper.
	maps map[MapID]*MapHandle
}

// Test seam for synthetic handles. Production close uses closeNativeRuntime.
var destroyRuntimeHandle func(nativeRuntime) int32

var operationDiscard = func(id uint64) int32 {
	return int32(C.mln_operation_discard_result(C.mln_operation(id)))
}

func waitNativeOperation(operation C.mln_operation) error {
	var completed C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_operation_wait(operation, -1, &completed))
	}); err != nil {
		return err
	}
	if !bool(completed) {
		return newBindingError(ErrInvalidState, "native operation wait returned before completion")
	}
	var terminal C.mln_status
	if err := checkNative(func() int32 {
		return int32(C.mln_operation_get_status(operation, &terminal))
	}); err != nil {
		return err
	}
	if int32(terminal) == int32(C.MLN_STATUS_OK) {
		return nil
	}
	diagnostic, _ := copyNativeOperationDiagnostic(operation)
	return &Error{
		kind:       kindForStatus(int32(terminal)),
		rawStatus:  int32(terminal),
		hasStatus:  true,
		diagnostic: diagnostic,
	}
}

func copyNativeOperationDiagnostic(operation C.mln_operation) (string, error) {
	var size C.size_t
	if err := checkNative(func() int32 {
		return int32(C.mln_operation_copy_diagnostic(operation, nil, 0, &size))
	}); err != nil {
		return "", err
	}
	if size == 0 {
		return "", nil
	}
	buffer := make([]byte, int(size))
	if err := checkNative(func() int32 {
		return int32(C.mln_operation_copy_diagnostic(
			operation,
			(*C.char)(unsafe.Pointer(&buffer[0])),
			C.size_t(len(buffer)),
			&size,
		))
	}); err != nil {
		return "", err
	}
	return string(buffer[:int(size)]), nil
}

func statusFromError(err error) int32 {
	var native *Error
	if errors.As(err, &native) {
		if status, ok := native.RawStatus(); ok {
			return status
		}
	}
	return int32(C.MLN_STATUS_NATIVE_ERROR)
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
func NewRuntimeWithOptions(options RuntimeOptions) (*RuntimeHandle, error) {
	if err := options.validate(); err != nil {
		return nil, err
	}
	if err := checkCompatibleCABI(CVersion()); err != nil {
		return nil, err
	}
	var source C.mln_notification_source
	if err := checkNative(func() int32 {
		return int32(C.mln_notification_source_create(&source))
	}); err != nil {
		return nil, err
	}
	keepSource := false
	defer func() {
		if !keepSource {
			_ = C.mln_notification_source_close(source)
		}
	}()

	rawOptions := C.mln_runtime_options_default()
	assetPath := C.CString(options.AssetPath)
	defer C.free(unsafe.Pointer(assetPath))
	cachePath := C.CString(options.CachePath)
	defer C.free(unsafe.Pointer(cachePath))
	rawOptions.asset_path = assetPath
	rawOptions.cache_path = cachePath
	rawOptions.event_mask = C.uint64_t(options.EventMask)
	rawOptions.notification_source = source

	var operation C.mln_operation
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_create_start(&rawOptions, &operation))
	}); err != nil {
		return nil, err
	}
	defer C.mln_operation_release(operation)
	if err := waitNativeOperation(operation); err != nil {
		return nil, err
	}
	var raw C.mln_runtime
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_create_take_result(operation, &raw))
	}); err != nil {
		return nil, err
	}
	state, err := newRuntimeState(nativeRuntime(raw))
	if err != nil {
		_ = closeNativeRuntime(nativeRuntime(raw))
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	keepSource = true
	return &RuntimeHandle{
		state:              state,
		notificationSource: nativeNotificationSource(source),
	}, nil
}

func closeNativeRuntime(runtime nativeRuntime) error {
	var operation C.mln_operation
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_close_start(C.mln_runtime(runtime), &operation))
	}); err != nil {
		return err
	}
	defer C.mln_operation_release(operation)
	return waitNativeOperation(operation)
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
		if destroyRuntimeHandle != nil {
			_ = destroyRuntimeHandle(runtime)
		} else {
			_ = closeNativeRuntime(runtime)
		}
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

// Barrier starts an ordered runtime operation that completes after every
// command accepted before it reaches a terminal disposition.
func (runtime *RuntimeHandle) Barrier() (*OperationHandle[struct{}], error) {
	return startOperation[struct{}](runtime, operationBarrier, operationResultNone, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_barrier_start(C.mln_runtime(handle), out))
	})
}

// DrainEvents takes this runtime's queued events as one batch of copied values.
// Events arrive in queue order, and the batch reports how many events stayed
// queued.
//
// maxEvents bounds the drain: zero takes every queued event, and a positive
// value takes at most that many and leaves the rest queued. A negative value
// returns ErrInvalidArgument.
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

	rawBatch, ownedBatch, err := drainRawEvents(ptr, maxEvents)
	if err != nil {
		return RuntimeEventBatch{}, err
	}
	defer C.mln_event_batch_release(ownedBatch)
	return runtime.copyEventBatch(rawBatch), nil
}

// SetEventMask selects which runtime-originated event types this runtime queues.
// It accepts RuntimeEventMaskAll, reads the bits in
// RuntimeEventMaskAllRuntimeEvents, and returns ErrInvalidArgument for a bit
// outside RuntimeEventMaskAll.
//
// Narrowing gates later events and keeps queued ones, so a caller drains what it
// already caused.
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

func drainRawEvents(ptr nativeRuntime, maxEvents int) (C.mln_runtime_event_batch_view, C.mln_event_batch, error) {
	var batch C.mln_event_batch
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_drain_events(C.mln_runtime(ptr), C.size_t(maxEvents), &batch))
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

// copyEventBatch copies an owned native batch view into owned Go values. It
// takes the map registry lock once for the whole batch.
func (runtime *RuntimeHandle) copyEventBatch(raw C.mln_runtime_event_batch_view) RuntimeEventBatch {
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
		payload := runtimeEventPayloadFromC(rawEvent, unsafe.Add(eventPtr, runtimeEventPayloadOffset), payloadWindow)
		event := RuntimeEvent{
			Type:        RuntimeEventType(rawEvent._type),
			SourceType:  source.Type,
			Source:      source,
			Code:        int32(rawEvent.code),
			PayloadType: RuntimeEventPayloadType(rawEvent.payload_type),
			Message:     runtimeEventMessage(raw, rawEvent),
			Payload:     payload,
		}
		events = append(events, event)
		if finished, ok := payload.(RuntimeEventCommandFinishedPayload); ok {
			runtime.finishResourceCallbackCommand(finished.CommandID, finished.Disposition)
		}
	}
	runtime.mapsMu.Unlock()

	batch.Events = events
	return batch
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

func (runtime *RuntimeHandle) finishResourceCallbackCommand(commandID uint64, disposition CommandDisposition) {
	runtime.resourceTransformMu.Lock()
	if transition, ok := runtime.pendingResourceTransform[commandID]; ok {
		delete(runtime.pendingResourceTransform, commandID)
		if disposition == CommandDispositionCommitted {
			previous := runtime.resourceTransform
			runtime.resourceTransform = transition.replacement
			runtime.resourceTransformMu.Unlock()
			previous.Release()
		} else {
			runtime.resourceTransformMu.Unlock()
			transition.replacement.Release()
		}
	} else {
		runtime.resourceTransformMu.Unlock()
	}

	runtime.httpHeaderTransformMu.Lock()
	if transition, ok := runtime.pendingHttpHeaderTransform[commandID]; ok {
		delete(runtime.pendingHttpHeaderTransform, commandID)
		if disposition == CommandDispositionCommitted {
			previous := runtime.httpHeaderTransform
			runtime.httpHeaderTransform = transition.replacement
			runtime.httpHeaderTransformMu.Unlock()
			previous.Release()
		} else {
			runtime.httpHeaderTransformMu.Unlock()
			transition.replacement.Release()
		}
	} else {
		runtime.httpHeaderTransformMu.Unlock()
	}

	runtime.resourceProviderMu.Lock()
	if transition, ok := runtime.pendingResourceProvider[commandID]; ok {
		delete(runtime.pendingResourceProvider, commandID)
		if disposition == CommandDispositionCommitted {
			previous := runtime.resourceProvider
			runtime.resourceProvider = transition.replacement
			runtime.resourceProviderMu.Unlock()
			previous.Release()
		} else {
			runtime.resourceProviderMu.Unlock()
			transition.replacement.Release()
		}
	} else {
		runtime.resourceProviderMu.Unlock()
	}
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
	case uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED):
		payload := C.mln_go_runtime_event_command_finished(event)
		disposition := uint32(payload.disposition)
		return RuntimeEventCommandFinishedPayload{
			CommandID:      uint64(payload.command_id),
			Disposition:    CommandDisposition(disposition),
			RawDisposition: disposition,
			Generation:     uint64(payload.generation),
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
func (runtime *RuntimeHandle) StartAmbientCacheOperation(operation AmbientCacheOperation) (*OperationHandle[struct{}], error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var id C.mln_operation
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_run_ambient_cache_operation_start(C.mln_runtime(ptr), C.uint32_t(operation), &id))
	}); err != nil {
		return nil, err
	}
	if id == 0 {
		return nil, newBindingError(ErrInvalidState, "ambient cache operation did not return an ID")
	}
	return newOperationHandle[struct{}](runtime, uint64(id), operationAmbientCache, operationResultNone), nil
}

// StartSetMaximumAmbientCacheSize starts a change to this runtime's maximum
// ambient cache size. Lowering it evicts ambient resources to fit the new
// budget; offline regions are unaffected.
func (runtime *RuntimeHandle) StartSetMaximumAmbientCacheSize(size uint64) (*OperationHandle[struct{}], error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	var id C.mln_operation
	if err := checkNative(func() int32 {
		return int32(C.mln_runtime_set_maximum_ambient_cache_size_start(C.mln_runtime(ptr), C.uint64_t(size), &id))
	}); err != nil {
		return nil, err
	}
	if id == 0 {
		return nil, newBindingError(ErrInvalidState, "maximum ambient cache size operation did not return an ID")
	}
	return newOperationHandle[struct{}](runtime, uint64(id), operationSetMaximumAmbientCacheSize, operationResultNone), nil
}

// SetResourceProvider submits a runtime-scoped network resource provider.
// Native code may invoke the provider on worker or network threads. The
// callback must be thread-safe and must not call map or runtime APIs. The
// returned command ID identifies the terminal command-finished event.
func (runtime *RuntimeHandle) SetResourceProvider(provider ResourceProviderCallback) (uint64, error) {
	if provider == nil {
		return 0, newBindingError(ErrInvalidArgument, "ResourceProviderCallback is nil")
	}
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	runtime.resourceProviderMu.Lock()
	defer runtime.resourceProviderMu.Unlock()
	var replacement *callback.ResourceProviderState
	var commandID uint64
	if err := checkNative(func() int32 {
		state, id, status := callback.SetResourceProvider(uint64(ptr), func(request callback.ResourceRequest, handle *callback.ResourceRequestHandle) uint32 {
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
		commandID = id
		return status
	}); err != nil {
		return 0, err
	}
	if runtime.pendingResourceProvider == nil {
		runtime.pendingResourceProvider = make(map[uint64]pendingResourceProvider)
	}
	runtime.pendingResourceProvider[commandID] = pendingResourceProvider{replacement: replacement}
	return commandID, nil
}

// ClearResourceProvider submits removal of the runtime-scoped network resource
// provider and returns the accepted command ID.
func (runtime *RuntimeHandle) ClearResourceProvider() (uint64, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	runtime.resourceProviderMu.Lock()
	defer runtime.resourceProviderMu.Unlock()
	var commandID uint64
	if err := checkNative(func() int32 {
		id, status := callback.ClearResourceProvider(uint64(ptr))
		commandID = id
		return status
	}); err != nil {
		return 0, err
	}
	if runtime.pendingResourceProvider == nil {
		runtime.pendingResourceProvider = make(map[uint64]pendingResourceProvider)
	}
	runtime.pendingResourceProvider[commandID] = pendingResourceProvider{}
	return commandID, nil
}

func (runtime *RuntimeHandle) releaseResourceProvider() {
	runtime.resourceProviderMu.Lock()
	current := runtime.resourceProvider
	runtime.resourceProvider = nil
	pending := runtime.pendingResourceProvider
	runtime.pendingResourceProvider = nil
	runtime.resourceProviderMu.Unlock()
	current.Release()
	for _, transition := range pending {
		transition.replacement.Release()
	}
}

// SetResourceTransform submits a runtime-scoped network URL transform. Native
// code may invoke the transform on worker or network threads. The callback must
// be thread-safe and must not call map or runtime APIs.
func (runtime *RuntimeHandle) SetResourceTransform(transform ResourceTransformCallback) (uint64, error) {
	if transform == nil {
		return 0, newBindingError(ErrInvalidArgument, "ResourceTransformCallback is nil")
	}
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	runtime.resourceTransformMu.Lock()
	defer runtime.resourceTransformMu.Unlock()
	var replacement *callback.ResourceTransformState
	var commandID uint64
	if err := checkNative(func() int32 {
		state, id, status := callback.SetResourceTransform(uint64(ptr), func(kind uint32, url string) (string, bool) {
			return transform(ResourceTransformRequest{Kind: ResourceKind(kind), RawKind: kind, URL: url})
		})
		replacement = state
		commandID = id
		return status
	}); err != nil {
		return 0, err
	}
	if runtime.pendingResourceTransform == nil {
		runtime.pendingResourceTransform = make(map[uint64]pendingResourceTransform)
	}
	runtime.pendingResourceTransform[commandID] = pendingResourceTransform{replacement: replacement}
	return commandID, nil
}

// ClearResourceTransform submits removal of the runtime-scoped network URL
// transform and returns the accepted command ID.
func (runtime *RuntimeHandle) ClearResourceTransform() (uint64, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	runtime.resourceTransformMu.Lock()
	defer runtime.resourceTransformMu.Unlock()
	var commandID uint64
	if err := checkNative(func() int32 {
		id, status := callback.ClearResourceTransform(uint64(ptr))
		commandID = id
		return status
	}); err != nil {
		return 0, err
	}
	if runtime.pendingResourceTransform == nil {
		runtime.pendingResourceTransform = make(map[uint64]pendingResourceTransform)
	}
	runtime.pendingResourceTransform[commandID] = pendingResourceTransform{}
	return commandID, nil
}

func (runtime *RuntimeHandle) releaseResourceTransform() {
	runtime.resourceTransformMu.Lock()
	current := runtime.resourceTransform
	runtime.resourceTransform = nil
	pending := runtime.pendingResourceTransform
	runtime.pendingResourceTransform = nil
	runtime.resourceTransformMu.Unlock()
	current.Release()
	for _, transition := range pending {
		transition.replacement.Release()
	}
}

// SetHttpHeaderTransform submits a runtime-scoped outgoing HTTP header
// transform and returns the accepted command ID.
func (runtime *RuntimeHandle) SetHttpHeaderTransform(transform HttpHeaderTransformCallback) (uint64, error) {
	if transform == nil {
		return 0, newBindingError(ErrInvalidArgument, "HttpHeaderTransformCallback is nil")
	}
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	runtime.httpHeaderTransformMu.Lock()
	defer runtime.httpHeaderTransformMu.Unlock()
	var replacement *callback.HttpHeaderTransformState
	var commandID uint64
	if err := checkNative(func() int32 {
		state, id, status := callback.SetHttpHeaderTransform(uint64(ptr), func(kind uint32, url string) []callback.HttpHeader {
			provided := transform(HttpHeaderTransformRequest{Kind: ResourceKind(kind), RawKind: kind, URL: url})
			headers := make([]callback.HttpHeader, len(provided))
			for index, header := range provided {
				headers[index] = callback.HttpHeader{Name: header.Name, Value: header.Value}
			}
			return headers
		})
		replacement = state
		commandID = id
		return status
	}); err != nil {
		return 0, err
	}
	if runtime.pendingHttpHeaderTransform == nil {
		runtime.pendingHttpHeaderTransform = make(map[uint64]pendingHttpHeaderTransform)
	}
	runtime.pendingHttpHeaderTransform[commandID] = pendingHttpHeaderTransform{replacement: replacement}
	return commandID, nil
}

// ClearHttpHeaderTransform submits removal of the runtime-scoped outgoing HTTP
// header transform and returns the accepted command ID.
func (runtime *RuntimeHandle) ClearHttpHeaderTransform() (uint64, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	runtime.httpHeaderTransformMu.Lock()
	defer runtime.httpHeaderTransformMu.Unlock()
	var commandID uint64
	if err := checkNative(func() int32 {
		id, status := callback.ClearHttpHeaderTransform(uint64(ptr))
		commandID = id
		return status
	}); err != nil {
		return 0, err
	}
	if runtime.pendingHttpHeaderTransform == nil {
		runtime.pendingHttpHeaderTransform = make(map[uint64]pendingHttpHeaderTransform)
	}
	runtime.pendingHttpHeaderTransform[commandID] = pendingHttpHeaderTransform{}
	return commandID, nil
}

func (runtime *RuntimeHandle) releaseHttpHeaderTransform() {
	runtime.httpHeaderTransformMu.Lock()
	current := runtime.httpHeaderTransform
	runtime.httpHeaderTransform = nil
	pending := runtime.pendingHttpHeaderTransform
	runtime.pendingHttpHeaderTransform = nil
	runtime.httpHeaderTransformMu.Unlock()
	current.Release()
	for _, transition := range pending {
		transition.replacement.Release()
	}
}

// NewMap creates a 512 by 512 logical map with native default options.
func (runtime *RuntimeHandle) NewMap() (*MapHandle, error) {
	return runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
}

// NewMapWithOptions creates a map owned by this runtime with explicit options.
// Start from NewMapOptions to keep every map-originated event type selected; a
// zero-value MapOptions queues no event.
func (runtime *RuntimeHandle) NewMapWithOptions(options MapOptions) (*MapHandle, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer release()
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

	var operation C.mln_operation
	if err := checkNative(func() int32 {
		return int32(C.mln_map_create_start(C.mln_runtime(ptr), &rawOptions, &operation))
	}); err != nil {
		return nil, err
	}
	defer C.mln_operation_release(operation)
	if err := waitNativeOperation(operation); err != nil {
		return nil, err
	}
	var raw C.mln_map
	if err := checkNative(func() int32 {
		return int32(C.mln_map_create_take_result(operation, &raw))
	}); err != nil {
		return nil, err
	}
	state, err := handle.New(nativeMap(raw), "MapHandle", runtime)
	if err != nil {
		_ = closeNativeMap(nativeMap(raw))
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	m := &MapHandle{
		state:        state,
		runtime:      runtime,
		runtimeChild: runtime.state.AddChild(),
		id:           MapID(raw),
	}
	runtime.registerMap(m)
	return m, nil
}

func closeNativeMap(m nativeMap) error {
	var operation C.mln_operation
	if err := checkNative(func() int32 {
		return int32(C.mln_map_close_start(C.mln_map(m), &operation))
	}); err != nil {
		return err
	}
	defer C.mln_operation_release(operation)
	return waitNativeOperation(operation)
}

// Close waits for this runtime's native close operation. A successful close
// makes later calls no-ops. A failed close leaves the native handle live so
// callers can retry after closing its children.
func (runtime *RuntimeHandle) Close() error {
	if runtime == nil || runtime.state == nil {
		return newBindingError(ErrInvalidArgument, "RuntimeHandle is nil")
	}
	var closeErr error
	_, err := runtime.state.CloseChecked(func(native nativeRuntime) int32 {
		var operation C.mln_operation
		if destroyRuntimeHandle != nil {
			status := destroyRuntimeHandle(native)
			if status != int32(C.MLN_STATUS_OK) {
				closeErr = &Error{
					kind:       kindForStatus(status),
					rawStatus:  status,
					hasStatus:  true,
					diagnostic: "synthetic runtime close failure",
				}
			}
			return status
		}
		if err := checkNative(func() int32 {
			return int32(C.mln_runtime_close_start(C.mln_runtime(native), &operation))
		}); err != nil {
			closeErr = err
			return statusFromError(err)
		}
		defer C.mln_operation_release(operation)
		if err := waitNativeOperation(operation); err != nil {
			closeErr = err
			return statusFromError(err)
		}
		return int32(C.MLN_STATUS_OK)
	})
	if err != nil {
		if errors.Is(err, handle.ErrLiveChildren) {
			return newBindingError(ErrInvalidState, "RuntimeHandle has live child handles")
		}
		return newBindingError(ErrInvalidState, err.Error())
	}
	if closeErr != nil {
		return closeErr
	}
	runtime.releaseResourceTransform()
	runtime.releaseHttpHeaderTransform()
	runtime.releaseResourceProvider()
	if err := runtime.clearNotificationCallback(); err != nil {
		return err
	}
	source := runtime.notificationSource
	if source != 0 {
		if err := checkNative(func() int32 {
			return int32(C.mln_notification_source_close(C.mln_notification_source(source)))
		}); err != nil {
			return err
		}
		runtime.notificationSource = 0
	}
	runtime.mapsMu.Lock()
	runtime.maps = nil
	runtime.mapsMu.Unlock()
	return nil
}
