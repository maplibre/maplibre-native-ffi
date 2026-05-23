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

// NewMap creates a map owned by this runtime with native default options.
func (runtime *RuntimeHandle) NewMap() (*MapHandle, error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer runtime.state.KeepAlive()

	var m *capi.Map
	if err := checkNative(func() capi.Status { return capi.MapCreateDefault(ptr, &m) }); err != nil {
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
