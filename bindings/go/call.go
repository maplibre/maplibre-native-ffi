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

// Borrowed outputs must be copied while the receiver is held. Input storage
// and callback roots remain live until the native call settles.

type bindingAccess uint8

const (
	bindingGlobalAccess bindingAccess = iota
	bindingOwnerAccess
	// The call leases the receiver and holds off its close, because the
	// results borrow from it until they are copied.
	bindingReadAccess
	// Native consumes the receiver when the call succeeds.
	bindingCloseAccess
	// Native always consumes the receiver.
	bindingConsumeAccess
	// The receiver is a response that is valid only inside its callback, on
	// the callback's thread.
	bindingScopedAccess
)

type bindingTarget struct {
	owner  *bindingOwner
	access bindingAccess
	scope  *bindingScope
}

func bindingGlobal() bindingTarget { return bindingTarget{} }
func bindingOwned(owner *bindingOwner) bindingTarget {
	return bindingTarget{owner: owner, access: bindingOwnerAccess}
}

func bindingRead(owner *bindingOwner) bindingTarget {
	return bindingTarget{owner: owner, access: bindingReadAccess}
}

func bindingClosing(owner *bindingOwner) bindingTarget {
	return bindingTarget{owner: owner, access: bindingCloseAccess}
}

func bindingConsuming(owner *bindingOwner) bindingTarget {
	return bindingTarget{owner: owner, access: bindingConsumeAccess}
}

func bindingScoped(scope *bindingScope) bindingTarget {
	return bindingTarget{access: bindingScopedAccess, scope: scope}
}

// bindingRun keeps owners reachable until outputs are copied and registrations
// are rooted. Native leases each id for the duration of its call.
func bindingRun[T any](target bindingTarget, call func(arena *bindingArena, raw uint64) T, synchronous bool) T {
	arena := &bindingArena{}
	defer arena.close()
	if target.access == bindingScopedAccess {
		target.scope.check()
	}
	if target.access == bindingGlobalAccess || target.access == bindingScopedAccess {
		value := call(arena, 0)
		arena.accept(nil)
		return value
	}
	owner := target.owner
	state := bindingHandleState(owner)
	defer runtime.KeepAlive(owner)
	if target.access == bindingCloseAccess || target.access == bindingConsumeAccess {
		if synchronous && state.released.Load() {
			var zero T
			return zero
		}
		if state.reads != nil {
			if !state.reads.TryLock() {
				panic(bindingFailure{newBindingError(StatusInvalidState, state.typeName+" is in use")})
			}
			defer state.reads.Unlock()
		}
		if target.access == bindingConsumeAccess {
			state.released.Store(true)
		}
		value := call(arena, state.raw)
		state.released.Store(true)
		arena.accept(owner)
		return value
	}
	if target.access == bindingReadAccess {
		state.reads.RLock()
		defer state.reads.RUnlock()
	}
	value := call(arena, state.raw)
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

// bindingGetUnless runs a status-returning native call like bindingGet. When
// native returns absent, which reports that the call published no output, it
// returns the zero T and no error instead.
func bindingGetUnless[T any](target bindingTarget, absent int32, call func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32, result func(arena *bindingArena) T) (T, error) {
	return bindingCall(func() T {
		return bindingRun(target, func(arena *bindingArena, raw uint64) T {
			present := true
			bindingCheck(func(diagnostic *C.mln_diagnostic) int32 {
				status := call(arena, raw, diagnostic)
				if status != absent {
					return status
				}
				present = false
				return int32(C.MLN_STATUS_OK)
			})
			if !present {
				var none T
				return none
			}
			return result(arena)
		}, true)
	})
}

func bindingDirect[T any](target bindingTarget, call func(arena *bindingArena, raw uint64) T) (T, error) {
	return bindingCall(func() T { return bindingRun(target, call, true) })
}

func bindingStart[T any](target bindingTarget, start func(arena *bindingArena, raw uint64, completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32, convert func(*C.mln_completion_result) (T, error), deliversStatus bool) (*Future[T], error) {
	return bindingStartWith(target, start, convert, func(_ *bindingArena, future *Future[T]) *Future[T] { return future }, deliversStatus)
}

func bindingStartWith[T, R any](target bindingTarget, start func(arena *bindingArena, raw uint64, completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32, convert func(*C.mln_completion_result) (T, error), result func(arena *bindingArena, future *Future[T]) R, deliversStatus bool) (R, error) {
	return bindingCall(func() R {
		return bindingRun(target, func(arena *bindingArena, raw uint64) R {
			future, err := startCompletion(func(completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32 {
				return start(arena, raw, completion, diagnostic)
			}, convert, deliversStatus)
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

func bindingOptionalView[T ~string | ~[]byte](value *T, arena *bindingArena) C.mln_buffer_view {
	if value == nil {
		return C.mln_buffer_view{}
	}
	return bindingView(*value, arena)
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
