package handle

import (
	"errors"
	"fmt"
	"log"
	"runtime"
	"sync"
)

// DestroyFunc releases one owned native handle. A non-OK status leaves the
// handle live so callers can retry on the correct owner thread.
type DestroyFunc[T any] func(*T) int32

// ErrLiveChildren reports that an owner cannot close while dependent handles are
// still live.
var ErrLiveChildren = errors.New("handle has live children")

// State stores close-once state for one owned native handle.
type State[T any] struct {
	mu        sync.Mutex
	cond      *sync.Cond
	ptr       *T
	typeName  string
	parents   []any
	borrows   int
	children  int
	releasing bool
}

// Borrow keeps one native pointer live until Release is called.
type Borrow[T any] struct {
	state *State[T]
	ptr   *T
}

// Child tracks one live dependent handle against its parent.
type Child struct {
	mu     sync.Mutex
	parent childCounter
	live   bool
}

type childCounter interface {
	removeChild()
}

// New creates close-once state for a non-nil owned native handle pointer.
func New[T any](ptr *T, typeName string, parents ...any) (*State[T], error) {
	if ptr == nil {
		return nil, fmt.Errorf("%s pointer is nil", typeName)
	}
	state := &State[T]{ptr: ptr, typeName: typeName, parents: parents}
	state.cond = sync.NewCond(&state.mu)
	runtime.SetFinalizer(state, func(state *State[T]) {
		state.reportLeakIfLive()
	})
	return state, nil
}

// Ptr returns the native pointer and whether the handle is still live. The
// returned pointer is only a snapshot; use Borrow for cgo calls.
func (state *State[T]) Ptr() (*T, bool) {
	state.mu.Lock()
	defer state.mu.Unlock()
	return state.ptr, state.ptr != nil
}

// Borrow returns the native pointer and keeps it live until the borrow is
// released. Borrow fails while release is in progress.
func (state *State[T]) Borrow() (*Borrow[T], bool) {
	state.mu.Lock()
	defer state.mu.Unlock()
	if state.ptr == nil || state.releasing {
		return nil, false
	}
	state.borrows++
	return &Borrow[T]{state: state, ptr: state.ptr}, true
}

// Ptr returns the borrowed native pointer.
func (borrow *Borrow[T]) Ptr() *T {
	if borrow == nil {
		return nil
	}
	return borrow.ptr
}

// Release ends this native pointer borrow. It is safe to call more than once.
func (borrow *Borrow[T]) Release() {
	if borrow == nil || borrow.state == nil {
		return
	}
	state := borrow.state
	borrow.state = nil
	borrow.ptr = nil

	state.mu.Lock()
	state.borrows--
	if state.borrows == 0 {
		state.cond.Broadcast()
	}
	state.mu.Unlock()
	state.KeepAlive()
}

// IsClosed reports whether this handle has been successfully closed.
func (state *State[T]) IsClosed() bool {
	state.mu.Lock()
	defer state.mu.Unlock()
	return state.ptr == nil
}

// Close calls destroy at most once after a successful native release.
func (state *State[T]) Close(destroy DestroyFunc[T], liveChildStatus ...int32) int32 {
	status, err := state.CloseChecked(destroy)
	if err != nil {
		if errors.Is(err, ErrLiveChildren) && len(liveChildStatus) > 0 {
			return liveChildStatus[0]
		}
		return -1
	}
	return status
}

// CloseChecked calls destroy at most once after a successful native release and
// reports binding-owned preconditions separately from native statuses.
func (state *State[T]) CloseChecked(destroy DestroyFunc[T]) (int32, error) {
	state.mu.Lock()
	for state.releasing {
		state.cond.Wait()
	}
	if state.ptr == nil {
		state.mu.Unlock()
		return 0, nil
	}
	state.releasing = true
	for state.borrows > 0 {
		state.cond.Wait()
	}
	if state.children > 0 {
		state.releasing = false
		state.cond.Broadcast()
		state.mu.Unlock()
		state.KeepAlive()
		return 0, ErrLiveChildren
	}
	ptr := state.ptr
	state.mu.Unlock()

	status := destroy(ptr)
	state.mu.Lock()
	if status == 0 {
		state.ptr = nil
	}
	state.releasing = false
	state.cond.Broadcast()
	state.mu.Unlock()

	if status != 0 {
		state.KeepAlive()
		return status, nil
	}
	runtime.SetFinalizer(state, nil)
	state.KeepAlive()
	return 0, nil
}

// AddChild records one live dependent handle. The returned child should be
// released after the dependent native handle is successfully destroyed.
func (state *State[T]) AddChild() *Child {
	state.mu.Lock()
	state.children++
	state.mu.Unlock()
	return &Child{parent: state, live: true}
}

// Release removes this dependent handle from its parent. It is safe to call
// more than once.
func (child *Child) Release() {
	if child == nil {
		return
	}
	child.mu.Lock()
	parent := child.parent
	live := child.live
	child.live = false
	child.parent = nil
	child.mu.Unlock()
	if live && parent != nil {
		parent.removeChild()
	}
}

func (state *State[T]) removeChild() {
	state.mu.Lock()
	state.children--
	state.cond.Broadcast()
	state.mu.Unlock()
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
