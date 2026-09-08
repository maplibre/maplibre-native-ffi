package handle

import (
	"fmt"
	"log"
	"runtime"
	"sync"
)

// DestroyFunc releases one owned native handle. A non-nil error leaves the
// handle live so callers can retry after correcting the failure.
type DestroyFunc[T ~uint64] func(T) error

// State stores close-once state for one owned native handle. The zero handle
// means closed. The C API leases the native object for each entry point, so
// calls do not need a second binding-side borrow.
type State[T ~uint64] struct {
	mu       sync.Mutex
	handle   T
	typeName string
}

// New creates close-once state for an owned native handle.
func New[T ~uint64](handle T, typeName string) (*State[T], error) {
	if handle == 0 {
		return nil, fmt.Errorf("%s handle is the null handle", typeName)
	}
	state := &State[T]{handle: handle, typeName: typeName}
	runtime.SetFinalizer(state, func(state *State[T]) {
		state.reportLeakIfLive()
	})
	return state, nil
}

// Handle returns the native handle and whether this wrapper still owns it.
func (state *State[T]) Handle() (T, bool) {
	state.mu.Lock()
	defer state.mu.Unlock()
	return state.handle, state.handle != 0
}

// IsClosed reports whether this handle has been successfully closed.
func (state *State[T]) IsClosed() bool {
	state.mu.Lock()
	defer state.mu.Unlock()
	return state.handle == 0
}

// Close calls destroy at most once after a successful native release. Closing
// an already closed handle is a no-op that reports no error.
func (state *State[T]) Close(destroy DestroyFunc[T]) error {
	state.mu.Lock()
	defer state.mu.Unlock()
	if state.handle == 0 {
		return nil
	}
	if err := destroy(state.handle); err != nil {
		return err
	}
	state.handle = 0
	runtime.SetFinalizer(state, nil)
	return nil
}

func (state *State[T]) reportLeakIfLive() {
	state.mu.Lock()
	live := state.handle != 0
	typeName := state.typeName
	state.mu.Unlock()
	if live {
		log.Printf("maplibre: leaked %s; call Close explicitly", typeName)
	}
}

// KeepAlive prevents the leak-reporting finalizer from running before a C call
// that used this state has returned.
func (state *State[T]) KeepAlive() {
	runtime.KeepAlive(state)
}

// TypeName returns the diagnostic native handle type name.
func (state *State[T]) TypeName() string {
	return state.typeName
}
