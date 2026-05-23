package handle

import (
	"fmt"
	"runtime"
	"sync"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

// DestroyFunc releases one owned native handle. A non-OK status leaves the
// handle live so callers can retry on the correct owner thread.
type DestroyFunc[T any] func(*T) capi.Status

// State stores close-once state for one owned native handle.
type State[T any] struct {
	mu       sync.Mutex
	ptr      *T
	typeName string
	parents  []any
}

// New creates close-once state for a non-nil owned native handle pointer.
func New[T any](ptr *T, typeName string, parents ...any) (*State[T], error) {
	if ptr == nil {
		return nil, fmt.Errorf("%s pointer is nil", typeName)
	}
	return &State[T]{ptr: ptr, typeName: typeName, parents: parents}, nil
}

// Ptr returns the native pointer and whether the handle is still live.
func (state *State[T]) Ptr() (*T, bool) {
	state.mu.Lock()
	defer state.mu.Unlock()
	return state.ptr, state.ptr != nil
}

// IsClosed reports whether this handle has been successfully closed.
func (state *State[T]) IsClosed() bool {
	_, live := state.Ptr()
	return !live
}

// Close calls destroy at most once after a successful native release.
func (state *State[T]) Close(destroy DestroyFunc[T]) capi.Status {
	state.mu.Lock()
	ptr := state.ptr
	state.mu.Unlock()
	if ptr == nil {
		return capi.StatusOK
	}

	status := destroy(ptr)
	if status != capi.StatusOK {
		state.KeepAlive()
		return status
	}

	state.mu.Lock()
	if state.ptr == ptr {
		state.ptr = nil
	}
	state.mu.Unlock()
	state.KeepAlive()
	return capi.StatusOK
}

// KeepAlive keeps the handle state and retained parents reachable until this
// point after a cgo call.
func (state *State[T]) KeepAlive() {
	for _, parent := range state.parents {
		runtime.KeepAlive(parent)
	}
	runtime.KeepAlive(state)
}

// TypeName returns the diagnostic native handle type name.
func (state *State[T]) TypeName() string {
	return state.typeName
}
