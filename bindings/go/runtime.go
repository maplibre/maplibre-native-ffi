package maplibre

import (
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

// NetworkStatus is MapLibre Native's process-global network reachability mode.
type NetworkStatus uint32

const (
	NetworkStatusOnline  NetworkStatus = NetworkStatus(capi.NetworkStatusOnline)
	NetworkStatusOffline NetworkStatus = NetworkStatus(capi.NetworkStatusOffline)
)

// RuntimeEventType identifies a runtime event kind.
type RuntimeEventType uint32

const (
	RuntimeEventMapCameraWillChange                 RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapCameraWillChange)
	RuntimeEventMapCameraIsChanging                 RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapCameraIsChanging)
	RuntimeEventMapCameraDidChange                  RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapCameraDidChange)
	RuntimeEventMapStyleLoaded                      RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapStyleLoaded)
	RuntimeEventMapLoadingStarted                   RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapLoadingStarted)
	RuntimeEventMapLoadingFinished                  RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapLoadingFinished)
	RuntimeEventMapLoadingFailed                    RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapLoadingFailed)
	RuntimeEventMapIdle                             RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapIdle)
	RuntimeEventMapRenderUpdateAvailable            RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapRenderUpdateAvailable)
	RuntimeEventMapRenderError                      RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapRenderError)
	RuntimeEventMapStillImageFinished               RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapStillImageFinished)
	RuntimeEventMapStillImageFailed                 RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapStillImageFailed)
	RuntimeEventMapRenderFrameStarted               RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapRenderFrameStarted)
	RuntimeEventMapRenderFrameFinished              RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapRenderFrameFinished)
	RuntimeEventMapRenderMapStarted                 RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapRenderMapStarted)
	RuntimeEventMapRenderMapFinished                RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapRenderMapFinished)
	RuntimeEventMapStyleImageMissing                RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapStyleImageMissing)
	RuntimeEventMapTileAction                       RuntimeEventType = RuntimeEventType(capi.RuntimeEventMapTileAction)
	RuntimeEventOfflineRegionStatusChanged          RuntimeEventType = RuntimeEventType(capi.RuntimeEventOfflineRegionStatusChanged)
	RuntimeEventOfflineRegionResponseError          RuntimeEventType = RuntimeEventType(capi.RuntimeEventOfflineRegionResponseError)
	RuntimeEventOfflineRegionTileCountLimitExceeded RuntimeEventType = RuntimeEventType(capi.RuntimeEventOfflineRegionTileCountLimitExceeded)
	RuntimeEventOfflineOperationCompleted           RuntimeEventType = RuntimeEventType(capi.RuntimeEventOfflineOperationCompleted)
)

// RuntimeEventSourceType identifies the native handle kind that emitted an event.
type RuntimeEventSourceType uint32

const (
	RuntimeEventSourceRuntime RuntimeEventSourceType = RuntimeEventSourceType(capi.RuntimeEventSourceRuntime)
	RuntimeEventSourceMap     RuntimeEventSourceType = RuntimeEventSourceType(capi.RuntimeEventSourceMap)
)

// RuntimeEventPayloadType identifies the copied event payload shape.
type RuntimeEventPayloadType uint32

const (
	RuntimeEventPayloadNone                        RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadNone)
	RuntimeEventPayloadRenderFrame                 RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadRenderFrame)
	RuntimeEventPayloadRenderMap                   RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadRenderMap)
	RuntimeEventPayloadStyleImageMissing           RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadStyleImageMissing)
	RuntimeEventPayloadTileAction                  RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadTileAction)
	RuntimeEventPayloadOfflineRegionStatus         RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadOfflineRegionStatus)
	RuntimeEventPayloadOfflineRegionResponseError  RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadOfflineRegionResponseError)
	RuntimeEventPayloadOfflineRegionTileCountLimit RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadOfflineRegionTileCountLimit)
	RuntimeEventPayloadOfflineOperationCompleted   RuntimeEventPayloadType = RuntimeEventPayloadType(capi.RuntimeEventPayloadOfflineOperationCompleted)
)

// RuntimeEvent is a copied runtime event. Payload-specific copied structs are
// added alongside their feature areas; unknown payloads preserve raw metadata.
type RuntimeEvent struct {
	Type        RuntimeEventType
	SourceType  RuntimeEventSourceType
	Source      uintptr
	Code        int32
	PayloadType RuntimeEventPayloadType
	PayloadSize uintptr
	Message     string
}

// RuntimeHandle owns scheduler state and event storage for one owner thread.
type RuntimeHandle struct {
	state *handle.State[capi.Runtime]
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
	var raw uint32
	if err := checkNative(func() capi.Status { return capi.NetworkStatusGet(&raw) }); err != nil {
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
	return checkNative(func() capi.Status { return capi.NetworkStatusSet(raw) })
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
	var runtime *capi.Runtime
	if err := checkNative(func() capi.Status { return capi.RuntimeCreateDefault(&runtime) }); err != nil {
		return nil, err
	}
	state, err := handle.New(runtime, "RuntimeHandle")
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &RuntimeHandle{state: state}, nil
}

func (runtime *RuntimeHandle) ptr() (*capi.Runtime, error) {
	if runtime == nil || runtime.state == nil {
		return nil, newBindingError(ErrInvalidArgument, "RuntimeHandle is nil")
	}
	ptr, live := runtime.state.Ptr()
	if !live {
		return nil, newBindingError(ErrInvalidArgument, "RuntimeHandle is closed")
	}
	return ptr, nil
}

// RunOnce runs one pending owner-thread task for this runtime.
func (runtime *RuntimeHandle) RunOnce() error {
	ptr, err := runtime.ptr()
	if err != nil {
		return err
	}
	defer runtime.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.RuntimeRunOnce(ptr) })
}

// PollEvent polls one queued runtime event and copies it into a Go value.
func (runtime *RuntimeHandle) PollEvent() (*RuntimeEvent, error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer runtime.state.KeepAlive()

	var rawEvent capi.RuntimeEvent
	var hasEvent bool
	if err := checkNative(func() capi.Status { return capi.RuntimePollEvent(ptr, &rawEvent, &hasEvent) }); err != nil {
		return nil, err
	}
	if !hasEvent {
		return nil, nil
	}
	return runtimeEventFromCAPI(rawEvent), nil
}

func runtimeEventFromCAPI(event capi.RuntimeEvent) *RuntimeEvent {
	return &RuntimeEvent{
		Type:        RuntimeEventType(event.Type),
		SourceType:  RuntimeEventSourceType(event.SourceType),
		Source:      event.Source,
		Code:        event.Code,
		PayloadType: RuntimeEventPayloadType(event.PayloadType),
		PayloadSize: event.PayloadSize,
		Message:     event.Message,
	}
}

// NewMap creates a map owned by this runtime with native default options.
func (runtime *RuntimeHandle) NewMap() (*MapHandle, error) {
	return runtime.createMap(func(ptr *capi.Runtime, out **capi.Map) capi.Status {
		return capi.MapCreateDefault(ptr, out)
	})
}

// NewMapWithOptions creates a map owned by this runtime with explicit options.
func (runtime *RuntimeHandle) NewMapWithOptions(options MapOptions) (*MapHandle, error) {
	return runtime.createMap(func(ptr *capi.Runtime, out **capi.Map) capi.Status {
		return capi.MapCreate(ptr, options.toCAPI(), out)
	})
}

func (runtime *RuntimeHandle) createMap(create func(*capi.Runtime, **capi.Map) capi.Status) (*MapHandle, error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer runtime.state.KeepAlive()

	var m *capi.Map
	if err := checkNative(func() capi.Status { return create(ptr, &m) }); err != nil {
		return nil, err
	}
	state, err := handle.New(m, "MapHandle", runtime)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &MapHandle{state: state, runtime: runtime}, nil
}

// Close destroys this runtime. A successful close makes later calls no-ops. A
// failed close leaves the native handle live so callers can retry on the owner
// thread.
func (runtime *RuntimeHandle) Close() error {
	if runtime == nil || runtime.state == nil {
		return newBindingError(ErrInvalidArgument, "RuntimeHandle is nil")
	}
	return checkNative(func() capi.Status {
		return runtime.state.Close(capi.RuntimeDestroy)
	})
}
