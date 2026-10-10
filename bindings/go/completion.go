package maplibre

/*
#include "binding_callback.h"
#include "internal/cgo_completion_shim.h"
*/
import "C"

import (
	"context"
	"runtime/cgo"
	"sync"
	"unsafe"
)

// CommandCompletion describes the terminal outcome of an ordered map command.
type CommandCompletion struct {
	Disposition CommandDisposition
	Generation  uint64
	RawStatus   int32
	Diagnostic  string
}

type futureResult[T any] struct {
	value T
	err   error
}

type futureState[T any] struct {
	ready     chan struct{}
	mu        sync.Mutex
	result    futureResult[T]
	completed bool
}

// Future is a one-shot native result. Await may be called from any goroutine.
type Future[T any] struct {
	state *futureState[T]
}

// completedFuture returns a future that already carries value, for work the
// binding satisfied without a native submission.
func completedFuture[T any](value T) *Future[T] {
	state := &futureState[T]{ready: make(chan struct{}), completed: true}
	state.result = futureResult[T]{value: value}
	close(state.ready)
	return &Future[T]{state: state}
}

// Done closes when native has delivered the terminal result. It lets a host
// service another loop without blocking in Await. A nil Future reports done
// immediately, and Await then reports ErrInvalidArgument rather than blocking.
func (future *Future[T]) Done() <-chan struct{} {
	if future == nil || future.state == nil {
		closed := make(chan struct{})
		close(closed)
		return closed
	}
	return future.state.ready
}

// Await blocks until native completes the work or the context is cancelled.
// A cancelled context ends only this wait: native work continues, and a later
// Await still returns its result. A handle that a creation delivers belongs to
// the caller once Await returns it. The owner's cleanup retires the handle of a
// Future that is dropped before any Await returns it.
func (future *Future[T]) Await(ctx context.Context) (T, error) {
	var zero T
	if future == nil || future.state == nil {
		return zero, newBindingError(ErrInvalidArgument, "Future is nil")
	}
	// A context that has already ended wins over a result that has arrived,
	// so its outcome does not depend on timing.
	if err := ctx.Err(); err != nil {
		return zero, err
	}
	select {
	case <-future.state.ready:
		future.state.mu.Lock()
		defer future.state.mu.Unlock()
		return future.state.result.value, future.state.result.err
	case <-ctx.Done():
		return zero, ctx.Err()
	}
}

type completionReceiver interface {
	complete(*C.mln_completion_result)
}

type completionBridge[T any] struct {
	state *futureState[T]
	// deliversStatus reports whether a non-OK terminal status belongs in the
	// converted value rather than in the future's error.
	deliversStatus bool
	convert        func(*C.mln_completion_result) (T, error)
}

func (bridge *completionBridge[T]) complete(raw *C.mln_completion_result) {
	var result futureResult[T]
	defer func() {
		if failure := recover(); failure != nil {
			if failure, ok := failure.(bindingFailure); ok {
				result.err = failure.error
			} else {
				result.err = newBindingError(ErrNative, "completion conversion panicked")
			}
		}
		bridge.state.mu.Lock()
		if bridge.state.completed {
			bridge.state.mu.Unlock()
			return
		}
		bridge.state.result = result
		bridge.state.completed = true
		bridge.state.mu.Unlock()
		close(bridge.state.ready)
	}()
	if raw == nil {
		result.err = newBindingError(ErrInvalidState, "native completion returned nil")
	} else if raw.status != C.MLN_STATUS_OK && !bridge.deliversStatus {
		result.err = newStatusError(int32(raw.status), bindingString(raw.diagnostic.data, uint64(raw.diagnostic.size)))
	} else {
		result.value, result.err = bridge.convert(raw)
	}
}

func startCompletion[T any](start func(*C.mln_completion, *C.mln_diagnostic) int32, convert func(*C.mln_completion_result) (T, error)) (*Future[T], error) {
	state := &futureState[T]{ready: make(chan struct{})}
	_, deliversStatus := any(*new(T)).(CommandCompletion)
	bridge := &completionBridge[T]{state: state, deliversStatus: deliversStatus, convert: convert}
	handle := cgo.NewHandle(completionReceiver(bridge))
	cell := bindingHandleCell(handle)
	completion := C.mln_go_make_completion(cell)
	// Native never calls a refused completion or its release, and start may
	// panic while it converts the call's arguments, before native sees it.
	submitted := false
	defer func() {
		if !submitted {
			handle.Delete()
			C.binding_handle_free(cell)
		}
	}()
	if err := checkNative(func(diagnostic *C.mln_diagnostic) int32 { return start(&completion, diagnostic) }); err != nil {
		return nil, err
	}
	submitted = true
	return &Future[T]{state: state}, nil
}

func completionUnit(result *C.mln_completion_result) (struct{}, error) {
	if result.value != nil || result.value_count != 0 {
		return struct{}{}, newBindingError(ErrInvalidState, "unit completion returned a value")
	}
	return struct{}{}, nil
}

func completionCommand(result *C.mln_completion_result) (CommandCompletion, error) {
	if _, err := completionUnit(result); err != nil {
		return CommandCompletion{}, err
	}
	return CommandCompletion{
		Disposition: CommandDisposition(result.disposition),
		Generation:  uint64(result.generation),
		RawStatus:   int32(result.status),
		Diagnostic:  completionDiagnostic(result),
	}, nil
}

func completionDiagnostic(result *C.mln_completion_result) string {
	if result.diagnostic.data == nil || result.diagnostic.size == 0 {
		return ""
	}
	return bindingString(result.diagnostic.data, uint64(result.diagnostic.size))
}

func completionValue[T any](result *C.mln_completion_result) (T, error) {
	var zero T
	if result.value == nil || result.value_count != 1 {
		return zero, newBindingError(ErrInvalidState, "native completion returned no value")
	}
	return *(*T)(result.value), nil
}

// completionItem returns element index of the completion's native array, which
// native lays out with a stride of value_size bytes.
func completionItem[T any](result *C.mln_completion_result, index int) *T {
	var item T
	return (*T)(bindingElement(result.value, index, uint64(result.value_size), unsafe.Sizeof(item), unsafe.Alignof(item)))
}

//export mln_go_completion_callback
func mln_go_completion_callback(userData unsafe.Pointer, result *C.mln_completion_result) {
	if userData == nil {
		return
	}
	// startCompletion is the only writer of this handle, so a value of another
	// type is a binding defect and the assertion panics rather than dropping a
	// terminal result.
	bindingHandleOf(userData).Value().(completionReceiver).complete(result)
}

//export mln_go_completion_release
func mln_go_completion_release(userData unsafe.Pointer) {
	if userData != nil {
		bindingHandleOf(userData).Delete()
		C.binding_handle_free(userData)
	}
}
