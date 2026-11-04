package maplibre

/*
#include "binding_callback.h"
#include "maplibre_native_c.h"
*/
import "C"

import (
	"runtime"
	"unsafe"
)

// Input storage and callback roots remain live until the native call settles.

// bindingTarget describes how a native call treats its receiver.
type bindingTarget struct {
	// handle reports that the call passes owner's native id as its receiver.
	// owner is nil for a nil receiver, which bindingRun reports.
	handle bool
	owner  *bindingOwner
	// scope, when set, is the callback that the receiver is valid inside, on
	// the callback's thread.
	scope *bindingScope
	// consumes reports that native consumes the receiver when the call
	// succeeds.
	consumes bool
	// answers reports a call that ends the receiver whatever it returns.
	// Unlike a release, a repeated answer still calls native, which reports
	// the handle stale.
	answers bool
}

func bindingGlobal() bindingTarget { return bindingTarget{} }
func bindingOwned(owner *bindingOwner) bindingTarget {
	return bindingTarget{handle: true, owner: owner}
}

func bindingClosing(owner *bindingOwner) bindingTarget {
	return bindingTarget{handle: true, owner: owner, consumes: true}
}

func bindingAnswering(owner *bindingOwner) bindingTarget {
	return bindingTarget{handle: true, owner: owner, consumes: true, answers: true}
}

// bindingScoped gives a nil response a scope that has already expired.
func bindingScoped(scope *bindingScope) bindingTarget {
	if scope == nil {
		scope = &bindingScope{}
	}
	return bindingTarget{scope: scope}
}

// bindingRun keeps owners reachable until outputs are copied and registrations
// are rooted. Native leases each id for the duration of its call. A repeated
// synchronous release or dispose returns the zero T; an async release has no
// zero future to return, so a repeated one reaches native.
func bindingRun[T any](target bindingTarget, call func(arena *bindingArena, raw uint64) T, synchronous bool) T {
	arena := &bindingArena{}
	defer arena.close()
	if target.scope != nil {
		target.scope.check()
	}
	if !target.handle {
		value := call(arena, 0)
		arena.accept(nil)
		return value
	}
	owner := target.owner
	state := bindingHandleState(owner)
	defer runtime.KeepAlive(owner)
	if target.consumes && state.borrowed {
		panic(bindingFailure{newBindingError(StatusInvalidState, state.typeName+" is borrowed by a callback, which can't release it")})
	}
	if target.consumes && synchronous && !target.answers && state.released.Load() {
		var zero T
		return zero
	}
	if target.answers {
		// Native ends a live handle on every outcome. A failure releases the
		// handle in case it came before native saw the answer.
		defer func() {
			if !state.released.Swap(true) {
				state.dispose(state.raw)
			}
		}()
	}
	value := call(arena, state.raw)
	if target.consumes {
		state.released.Store(true)
	}
	arena.accept(owner)
	return value
}

func bindingDo(target bindingTarget, call func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32) error {
	_, err := bindingGet(target, call, func(*bindingArena) struct{} { return struct{}{} })
	return err
}

func callDiscard(target bindingTarget, call func(arena *bindingArena, raw uint64, completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32) error {
	return bindingDo(target, func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32 {
		return call(arena, raw, nil, diagnostic)
	})
}

func bindingGet[T any](target bindingTarget, call func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32, result func(arena *bindingArena) T) (T, error) {
	return bindingCall(func() T {
		return bindingRun(target, func(arena *bindingArena, raw uint64) T {
			bindingCheck(func(diagnostic *C.mln_diagnostic) int32 { return call(arena, raw, diagnostic) })
			return result(arena)
		}, true)
	})
}

func bindingDirect[T any](target bindingTarget, call func(arena *bindingArena, raw uint64) T) (T, error) {
	return bindingCall(func() T { return bindingRun(target, call, true) })
}

func bindingStart[T any](target bindingTarget, start func(arena *bindingArena, raw uint64, completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32, convert func(*C.mln_completion_result) (T, error)) (*Future[T], error) {
	return bindingStartWith(target, start, convert, func(_ *bindingArena, future *Future[T]) *Future[T] { return future })
}

func bindingStartWith[T, R any](target bindingTarget, start func(arena *bindingArena, raw uint64, completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32, convert func(*C.mln_completion_result) (T, error), result func(arena *bindingArena, future *Future[T]) R) (R, error) {
	return bindingCall(func() R {
		return bindingRun(target, func(arena *bindingArena, raw uint64) R {
			future, err := startCompletion(func(completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32 {
				return start(arena, raw, completion, diagnostic)
			}, convert)
			if err != nil {
				panic(bindingFailure{err})
			}
			return result(arena, future)
		}, false)
	})
}

// bindingWithView passes use a view of a value that the receiver lends only
// while use runs. begin, when the receiver needs one, opens a native view
// scope that end closes; get fills the value, and view wraps it with the
// binding scope that expires when use returns.
func bindingWithView[V any](target bindingTarget, use func(V) error, begin func(raw uint64, token *unsafe.Pointer, diagnostic *C.mln_diagnostic) int32, end func(token unsafe.Pointer), get func(raw uint64, diagnostic *C.mln_diagnostic) int32, view func(scope *bindingScope) V) error {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()
	_, err := bindingCall(func() struct{} {
		return bindingRun(target, func(arena *bindingArena, raw uint64) struct{} {
			if use == nil {
				arena.fail("view callback is nil")
			}
			if begin != nil {
				var token unsafe.Pointer
				bindingCheck(func(diagnostic *C.mln_diagnostic) int32 { return begin(raw, &token, diagnostic) })
				defer end(token)
			}
			scope := bindingNewScope()
			defer scope.alive.Store(false)
			bindingCheck(func(diagnostic *C.mln_diagnostic) int32 { return get(raw, diagnostic) })
			if err := use(view(scope)); err != nil {
				panic(bindingFailure{err})
			}
			return struct{}{}
		}, true)
	})
	return err
}

func bindingView[T ~string | ~[]byte](value T, arena *bindingArena) C.mln_buffer_view {
	return C.mln_buffer_view{data: arena.bytes([]byte(value)), size: C.size_t(len(value))}
}

// Copy completion values before native reuses their storage.

func completionOf[N, T any](copy func(N) T) func(*C.mln_completion_result) (T, error) {
	return func(result *C.mln_completion_result) (T, error) {
		raw, err := completionValue[N](result)
		if err != nil {
			var zero T
			return zero, err
		}
		return copy(raw), nil
	}
}

func completionNullable[T any](convert func(*C.mln_completion_result) (T, error)) func(*C.mln_completion_result) (*T, error) {
	return func(result *C.mln_completion_result) (*T, error) {
		if result.value == nil {
			return nil, nil
		}
		value, err := convert(result)
		if err != nil {
			return nil, err
		}
		return &value, nil
	}
}

// completionOrZero reads an absent value as T's zero value, which no present
// value takes.
func completionOrZero[T any](convert func(*C.mln_completion_result) (T, error)) func(*C.mln_completion_result) (T, error) {
	return func(result *C.mln_completion_result) (T, error) {
		if result.value == nil {
			var zero T
			return zero, nil
		}
		return convert(result)
	}
}

func completionListOf[N, T any](copy func(N) T) func(*C.mln_completion_result) ([]T, error) {
	return func(result *C.mln_completion_result) ([]T, error) {
		if result.value_count == 0 {
			return []T{}, nil
		}
		if result.value == nil {
			return nil, newBindingError(StatusInvalidState, "native completion returned a null slice")
		}
		copied := make([]T, bindingLength(uint64(result.value_count)))
		for i := range copied {
			copied[i] = copy(*completionItem[N](result, i))
		}
		return copied, nil
	}
}

func copyViewText(raw C.mln_buffer_view) string { return bindingString(raw.data, uint64(raw.size)) }

func copyViewBytes(raw C.mln_buffer_view) []byte { return bindingBytes(raw.data, uint64(raw.size)) }
