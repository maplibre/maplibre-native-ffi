package handle

import (
	"fmt"
	"log"
	"runtime"
	"sync"
)

// DestroyFunc releases one owned native handle. A non-OK status leaves the
// handle live so callers can retry on the correct owner thread.
type DestroyFunc[T any] func(*T) int32

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
	state := &State[T]{ptr: ptr, typeName: typeName, parents: parents}
	runtime.SetFinalizer(state, func(state *State[T]) {
		state.reportLeakIfLive()
	})
	return state, nil
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
func (state *State[T]) Close(destroy DestroyFunc[T]) int32 {
	state.mu.Lock()
	ptr := state.ptr
	if ptr == nil {
		state.mu.Unlock()
		return 0
	}

	status := destroy(ptr)
	if status == 0 {
		state.ptr = nil
	}
	state.mu.Unlock()

	if status != 0 {
		state.KeepAlive()
		return status
	}
	runtime.SetFinalizer(state, nil)
	state.KeepAlive()
	return 0
}

func (state *State[T]) reportLeakIfLive() {
	state.mu.Lock()
	live := state.ptr != nil
	typeName := state.typeName
	state.mu.Unlock()
	if live {
		log.Printf("maplibre: leaked %s; call Close on its owner thread", typeName)
	}
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
