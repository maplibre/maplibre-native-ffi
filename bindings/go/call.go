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

// The steps that every generated operation shares. A generated operation
// names its target, which says how it reaches its receiver, and passes a
// closure that makes the native call with each argument converted in place.
// The functions here admit the call, hold the receiver, provide the input
// arena, check the native status, settle a close or a completion, move the
// callback roots that inputs registered to their owner, and convert the
// results while the receiver is still held.

// bindingAccess says how an operation reaches its receiver.
type bindingAccess uint8

const (
	// The operation has no receiver.
	bindingGlobalAccess bindingAccess = iota
	// The call leases the receiver.
	bindingLiveAccess
	// The call leases the receiver and holds off its close, because the
	// results borrow from it until they are copied.
	bindingReadAccess
	// The call passes the receiver's issued id without a lease.
	bindingIssuedAccess
	// Native consumes the receiver when the call succeeds.
	bindingCloseAccess
	// Native always consumes the receiver.
	bindingConsumeAccess
	// The call completes the receiver's pending decision.
	bindingCompleteAccess
	// The receiver is a response that is valid only inside its callback, on
	// the callback's thread.
	bindingScopedAccess
)

// bindingTarget is an operation's receiver, how the call reaches it, and the
// operation's identity for callback admission.
type bindingTarget struct {
	owner     *bindingOwner
	access    bindingAccess
	operation uint32
	// A scoped receiver's scope and the native address admission checks.
	scope    *bindingScope
	identity uint64
}

func bindingGlobal(operation uint32) bindingTarget {
	return bindingTarget{operation: operation}
}

func bindingLive(owner *bindingOwner, operation uint32) bindingTarget {
	return bindingTarget{owner: owner, access: bindingLiveAccess, operation: operation}
}

func bindingRead(owner *bindingOwner, operation uint32) bindingTarget {
	return bindingTarget{owner: owner, access: bindingReadAccess, operation: operation}
}

func bindingIssued(owner *bindingOwner, operation uint32) bindingTarget {
	return bindingTarget{owner: owner, access: bindingIssuedAccess, operation: operation}
}

func bindingClosing(owner *bindingOwner, operation uint32) bindingTarget {
	return bindingTarget{owner: owner, access: bindingCloseAccess, operation: operation}
}

func bindingConsuming(owner *bindingOwner, operation uint32) bindingTarget {
	return bindingTarget{owner: owner, access: bindingConsumeAccess, operation: operation}
}

func bindingCompleting(owner *bindingOwner, operation uint32) bindingTarget {
	return bindingTarget{owner: owner, access: bindingCompleteAccess, operation: operation}
}

// bindingScoped targets a callback-scoped response whose native address is
// identity. A nil scope reports a nil handle.
func bindingScoped(scope *bindingScope, identity uint64, operation uint32) bindingTarget {
	return bindingTarget{access: bindingScopedAccess, operation: operation, scope: scope, identity: identity}
}

// bindingCloseDecision closes a decision handle. A close while its decision or
// completion is in flight takes effect when that finishes.
func bindingCloseDecision(owner *bindingOwner, operation uint32) error {
	_, err := bindingCall(func() struct{} {
		if owner == nil || owner.state == nil {
			panic(bindingFailure{newBindingError(ErrInvalidState, "nil handle")})
		}
		bindingAdmission(operation, owner.state.issued)
		owner.state.closeDecision()
		runtime.KeepAlive(owner)
		return struct{}{}
	})
	return err
}

// bindingRun admits the target's operation, holds its receiver, and runs call
// with the receiver's raw id and an input arena. A call that returns without
// panicking succeeded: a close commits, a decision completes, and the callback
// roots that the inputs registered move to the receiver. A receiver that is
// already closed makes a closing call return closed's value without running.
func bindingRun[T any](target bindingTarget, call func(arena *bindingArena, raw uint64) T, closed func() T) T {
	arena := &bindingArena{}
	defer arena.close()
	owner := target.owner
	switch target.access {
	case bindingGlobalAccess:
		bindingAdmission(target.operation, 0)
		value := call(arena, 0)
		arena.accept(nil)
		return value
	case bindingScopedAccess:
		if target.scope == nil {
			panic(bindingFailure{newBindingError(ErrInvalidState, "nil handle")})
		}
		bindingAdmission(target.operation, target.identity)
		target.scope.check()
		value := call(arena, 0)
		arena.accept(nil)
		return value
	}
	if owner == nil || owner.state == nil {
		panic(bindingFailure{newBindingError(ErrInvalidState, "nil handle")})
	}
	bindingAdmission(target.operation, owner.state.issued)
	defer runtime.KeepAlive(owner)
	arena.identity = owner.state.issued
	var value T
	switch target.access {
	case bindingIssuedAccess:
		value = call(arena, owner.state.issued)
	case bindingCloseAccess, bindingConsumeAccess:
		raw, transaction := owner.state.reserveClose()
		defer transaction.finish()
		if raw == 0 {
			if closed == nil {
				return value
			}
			return closed()
		}
		if target.access == bindingConsumeAccess {
			transaction.commit()
		}
		value = call(arena, raw)
		transaction.commit()
	case bindingCompleteAccess:
		raw, finish := owner.state.reserveCompletion()
		accepted := false
		defer func() { finish(accepted) }()
		value = call(arena, raw)
		accepted = true
	default:
		raw, done := owner.bindingAcquire(target.access == bindingReadAccess)
		defer done()
		value = call(arena, raw)
	}
	arena.accept(owner)
	return value
}

// bindingDo runs a status-returning native call.
func bindingDo(target bindingTarget, call func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32) error {
	_, err := bindingGet(target, call, func(*bindingArena) struct{} { return struct{}{} })
	return err
}

// bindingGet runs a status-returning native call and converts its outputs
// with result while the receiver is held.
func bindingGet[T any](target bindingTarget, call func(arena *bindingArena, raw uint64, diagnostic *C.mln_diagnostic) int32, result func(arena *bindingArena) T) (T, error) {
	return bindingCall(func() T {
		return bindingRun(target, func(arena *bindingArena, raw uint64) T {
			bindingCheck(func(diagnostic *C.mln_diagnostic) int32 { return call(arena, raw, diagnostic) })
			return result(arena)
		}, nil)
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
		}, nil)
	})
}

// bindingDirect runs a native call that returns its result rather than a
// status, and returns what call converts it to.
func bindingDirect[T any](target bindingTarget, call func(arena *bindingArena, raw uint64) T) (T, error) {
	return bindingCall(func() T { return bindingRun(target, call, nil) })
}

// bindingStart starts native work that reports through a completion and
// returns its future, which convert fills from the completion result.
func bindingStart[T any](target bindingTarget, start func(arena *bindingArena, raw uint64, completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32, convert func(*C.mln_completion_result) (T, error)) (*Future[T], error) {
	return bindingStartWith(target, start, convert, func(_ *bindingArena, future *Future[T]) *Future[T] { return future })
}

// bindingStartWith starts native work like bindingStart, and converts the
// outputs native wrote when it accepted the work with result.
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
		}, func() R {
			// Only a close finds its receiver closed, and a close has no
			// outputs for result to convert.
			var zero T
			return result(&bindingArena{}, completedFuture(zero))
		})
	})
}

// bindingWithView passes use a view of a value that the receiver lends only
// while use runs. begin, when the receiver needs one, opens a native view
// scope that end closes; get fills the value, and view wraps it with the
// binding scope that expires when use returns.
func bindingWithView[V any](target bindingTarget, use func(V) error, begin func(raw uint64, token *unsafe.Pointer, diagnostic *C.mln_diagnostic) int32, end func(token unsafe.Pointer), get func(raw uint64, diagnostic *C.mln_diagnostic) int32, view func(scope *bindingScope) V) error {
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
		}, nil)
	})
	return err
}

// Input conversions. Each converts one binding value to the native value a
// parameter takes, in storage that lasts until the call returns.

// bindingView borrows text or bytes as a native buffer view.
func bindingView[T ~string | ~[]byte](value T, arena *bindingArena) C.mln_buffer_view {
	return C.mln_buffer_view{data: arena.bytes([]byte(value)), size: C.size_t(len(value))}
}

// bindingNullableView borrows optional text or bytes, keeping present empty
// text distinct from absent text with a non-null view.
func bindingNullableView[T ~string | ~[]byte](value *T, arena *bindingArena) C.mln_buffer_view {
	if value == nil {
		return C.mln_buffer_view{}
	}
	view := bindingView(*value, arena)
	if view.data == nil {
		view.data = arena.allocate(1)
	}
	return view
}

// bindingOptionalView borrows optional text or bytes, which native reads as
// absent when empty.
func bindingOptionalView[T ~string | ~[]byte](value *T, arena *bindingArena) C.mln_buffer_view {
	if value == nil {
		return C.mln_buffer_view{}
	}
	return bindingView(*value, arena)
}

// bindingOptionalCString copies optional text with a terminating NUL.
func bindingOptionalCString(value *string, arena *bindingArena) *C.char {
	if value == nil {
		return nil
	}
	return arena.cstring(*value)
}

// Completion result conversions. Each returns a converter that copies a
// completion's value before native reuses the result's storage.

// completionOf converts the completion's one native value with copy.
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

// completionNullable converts like convert, or returns nil for a null value.
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

// completionListOf converts each item of the completion's native array.
func completionListOf[N, T any](copy func(N) T) func(*C.mln_completion_result) ([]T, error) {
	return func(result *C.mln_completion_result) ([]T, error) {
		if result.value_count == 0 {
			return []T{}, nil
		}
		if result.value == nil {
			return nil, newBindingError(ErrInvalidState, "native completion returned a null slice")
		}
		copied := make([]T, bindingLength(uint64(result.value_count)))
		for i := range copied {
			copied[i] = copy(*completionItem[N](result, i))
		}
		return copied, nil
	}
}

// completionNullableListOf converts like completionListOf, or returns a nil
// slice for a null array.
func completionNullableListOf[N, T any](copy func(N) T) func(*C.mln_completion_result) ([]T, error) {
	convert := completionListOf(copy)
	return func(result *C.mln_completion_result) ([]T, error) {
		if result.value == nil {
			return nil, nil
		}
		return convert(result)
	}
}

// copyViewText and copyViewBytes copy the contents of a native buffer view.
func copyViewText(raw C.mln_buffer_view) string { return bindingString(raw.data, uint64(raw.size)) }

func copyViewBytes(raw C.mln_buffer_view) []byte { return bindingBytes(raw.data, uint64(raw.size)) }

// copyOptionalViewText and copyOptionalViewBytes copy a native buffer view
// that is absent when empty.
func copyOptionalViewText(raw C.mln_buffer_view) *string {
	if raw.size == 0 {
		return nil
	}
	value := copyViewText(raw)
	return &value
}

func copyOptionalViewBytes(raw C.mln_buffer_view) *[]byte {
	if raw.size == 0 {
		return nil
	}
	value := copyViewBytes(raw)
	return &value
}
