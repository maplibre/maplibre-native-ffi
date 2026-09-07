package maplibre

/*
#include "internal/cgo_completion_shim.h"
*/
import "C"

import (
	"context"
	"runtime/cgo"
	"sync"
	"unsafe"

	internalstatus "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/status"
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
	retained  any
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

func (future *Future[T]) retain(value any) {
	future.state.mu.Lock()
	defer future.state.mu.Unlock()
	if !future.state.completed {
		future.state.retained = value
	}
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
func (future *Future[T]) Await(ctx context.Context) (T, error) {
	var zero T
	if future == nil || future.state == nil {
		return zero, newBindingError(ErrInvalidArgument, "Future is nil")
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
	release()
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
	if raw == nil {
		result.err = newBindingError(ErrInvalidState, "native completion returned nil")
	} else if raw.status != C.MLN_STATUS_OK && !bridge.deliversStatus {
		diagnostic := ""
		if raw.diagnostic.data != nil && raw.diagnostic.size != 0 {
			diagnostic = string(unsafe.Slice((*byte)(raw.diagnostic.data), int(raw.diagnostic.size)))
		}
		result.err = newStatusError(&internalstatus.NativeError{
			Status: int32(raw.status), Diagnostic: diagnostic,
		})
	} else {
		result.value, result.err = bridge.convert(raw)
	}
	bridge.state.mu.Lock()
	bridge.state.result = result
	bridge.state.completed = true
	bridge.state.mu.Unlock()
	close(bridge.state.ready)
}

func (bridge *completionBridge[T]) release() {
	bridge.state.mu.Lock()
	bridge.state.retained = nil
	bridge.state.mu.Unlock()
}

func startCompletion[T any](
	start func(*C.mln_completion) int32,
	convert func(*C.mln_completion_result) (T, error),
) (*Future[T], error) {
	state := &futureState[T]{ready: make(chan struct{})}
	_, deliversStatus := any(*new(T)).(CommandCompletion)
	bridge := &completionBridge[T]{state: state, deliversStatus: deliversStatus, convert: convert}
	handle := cgo.NewHandle(completionReceiver(bridge))
	completion := C.mln_go_make_completion_from_handle(C.uintptr_t(handle))
	if err := checkNative(func() int32 { return start(&completion) }); err != nil {
		handle.Delete()
		return nil, err
	}
	return &Future[T]{state: state}, nil
}

// startMapCompletion submits one map command or ordered query and returns its
// future. The handle stays reachable across the native call.
func startMapCompletion[T any](
	m *MapHandle,
	start func(C.mln_map, *C.mln_completion) int32,
	convert func(*C.mln_completion_result) (T, error),
) (*Future[T], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return start(C.mln_map(ptr), completion)
	}, convert)
}

// startRuntimeCompletion submits one runtime operation and returns its future.
func startRuntimeCompletion[T any](
	runtime *RuntimeHandle,
	start func(C.mln_runtime, *C.mln_completion) int32,
	convert func(*C.mln_completion_result) (T, error),
) (*Future[T], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer runtime.state.KeepAlive()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return start(C.mln_runtime(ptr), completion)
	}, convert)
}

// startRenderCompletion submits one render-session operation and returns its
// future.
func startRenderCompletion[T any](
	session *RenderSessionHandle,
	start func(C.mln_render_session, *C.mln_completion) int32,
	convert func(*C.mln_completion_result) (T, error),
) (*Future[T], error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return start(C.mln_render_session(ptr), completion)
	}, convert)
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
	return string(unsafe.Slice((*byte)(result.diagnostic.data), int(result.diagnostic.size)))
}

func completionValue[T any](result *C.mln_completion_result) (T, error) {
	var zero T
	if result.value == nil || result.value_count != 1 {
		return zero, newBindingError(ErrInvalidState, "native completion returned no value")
	}
	return *(*T)(result.value), nil
}

func completionSlice[T any](result *C.mln_completion_result) ([]T, error) {
	if result.value_count == 0 {
		return []T{}, nil
	}
	if result.value == nil {
		return nil, newBindingError(ErrInvalidState, "native completion returned a null slice")
	}
	return append([]T(nil), unsafe.Slice((*T)(result.value), int(result.value_count))...), nil
}

func completionBuffer(result *C.mln_completion_result) ([]byte, error) {
	view, err := completionValue[C.mln_buffer_view](result)
	if err != nil {
		return nil, err
	}
	if view.size == 0 {
		return nil, nil
	}
	if view.data == nil {
		return nil, newBindingError(ErrInvalidState, "native completion returned a null buffer")
	}
	return append([]byte(nil), unsafe.Slice((*byte)(view.data), int(view.size))...), nil
}

func completionNullableBuffer(result *C.mln_completion_result) ([]byte, error) {
	if result.value_count == 0 {
		return nil, nil
	}
	return completionBuffer(result)
}

//export mln_go_completion_callback
func mln_go_completion_callback(userData unsafe.Pointer, result *C.mln_completion_result) {
	if userData == nil {
		return
	}
	// startCompletion is the only writer of this handle, so a value of another
	// type is a binding defect and the assertion panics rather than dropping a
	// terminal result.
	cgo.Handle(uintptr(userData)).Value().(completionReceiver).complete(result)
}

//export mln_go_completion_release
func mln_go_completion_release(userData unsafe.Pointer) {
	if userData != nil {
		handle := cgo.Handle(uintptr(userData))
		handle.Value().(completionReceiver).release()
		handle.Delete()
	}
}
