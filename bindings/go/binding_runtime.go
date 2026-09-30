package maplibre

/*
#include <stdlib.h>
#include <stdint.h>
#include "binding_callback.h"
#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"

*/
import "C"

import (
	"runtime"
	"runtime/cgo"
	"strings"
	"sync"
	"sync/atomic"
	"unsafe"
	"weak"
)

// UnknownVariant preserves a discriminator whose payload this binding cannot interpret.
type UnknownVariant struct{ Tag uint32 }

func (value UnknownVariant) bindingTag() uint32 { return value.Tag }

type bindingFailure struct{ error }

func bindingCall[T any](call func() T) (value T, err error) {
	defer func() {
		if failure := recover(); failure != nil {
			if failure, ok := failure.(bindingFailure); ok {
				err = failure.error
			} else {
				panic(failure)
			}
		}
	}()
	value = call()
	return
}

func bindingCheck(call func(*C.mln_diagnostic) int32) {
	if err := checkNative(call); err != nil {
		panic(bindingFailure{err})
	}
}

type bindingArena struct {
	pins        runtime.Pinner
	allocations []unsafe.Pointer
	callbacks   []*bindingCallbackTicket
}

func (arena *bindingArena) fail(message string) {
	panic(bindingFailure{newBindingError(ErrInvalidArgument, message)})
}

func (arena *bindingArena) allocate(size uintptr) unsafe.Pointer {
	if size == 0 {
		return nil
	}
	result := C.calloc(1, C.size_t(size))
	if result == nil {
		panic(bindingFailure{newBindingError(ErrNative, "native input allocation failed")})
	}
	arena.allocations = append(arena.allocations, result)
	return result
}

func (arena *bindingArena) bytes(value []byte) unsafe.Pointer {
	if len(value) == 0 {
		return nil
	}
	pointer := unsafe.Pointer(unsafe.SliceData(value))
	arena.pins.Pin(pointer)
	return pointer
}

func (arena *bindingArena) cstring(value string) *C.char {
	if strings.IndexByte(value, 0) >= 0 {
		arena.fail("text contains an embedded NUL")
	}
	pointer := arena.allocate(uintptr(len(value) + 1))
	copy(unsafe.Slice((*byte)(pointer), len(value)), value)
	return (*C.char)(pointer)
}

func (arena *bindingArena) close() {
	defer arena.pins.Unpin()
	for _, ticket := range arena.callbacks {
		ticket.release()
	}
	for _, allocation := range arena.allocations {
		C.free(allocation)
	}
}

func (arena *bindingArena) accept(owner *bindingOwner) {
	for _, ticket := range arena.callbacks {
		ticket.mu.Lock()
		if !ticket.retired {
			if owner != nil {
				owner.rootsMu.Lock()
				owner.roots[ticket.id] = ticket
				owner.rootsMu.Unlock()
				ticket.owner = weak.Make(owner)
			} else {
				bindingGlobalRoots.Lock()
				bindingGlobalRoots.values[ticket.id] = ticket
				bindingGlobalRoots.Unlock()
			}
		}
		ticket.mu.Unlock()
	}
	arena.callbacks = nil
}

func (arena *bindingArena) array(count int, size uintptr) unsafe.Pointer {
	if size != 0 && uintptr(count) > ^uintptr(0)/size {
		arena.fail("input array exceeds address space")
	}
	return arena.allocate(uintptr(count) * size)
}

// bindingCount checks narrowing to the declared C count type before allocation.
func bindingCount[T ~int8 | ~int16 | ~int32 | ~int64 | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~int | ~uint](count int) T {
	result := T(count)
	if count < 0 || result < 0 || uint64(result) != uint64(count) {
		panic(bindingFailure{newBindingError(ErrInvalidArgument, "input count exceeds C capacity")})
	}
	return result
}

func bindingCountLike[T ~int8 | ~int16 | ~int32 | ~int64 | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~int | ~uint](_ T, count int) T {
	return bindingCount[T](count)
}

func bindingLength(count uint64) int {
	if count > uint64(^uint(0)>>1) {
		panic(bindingFailure{newBindingError(ErrNative, "native count exceeds Go capacity")})
	}
	return int(count)
}

func bindingElement(pointer unsafe.Pointer, index int, stride uint64, minimum, alignment uintptr) unsafe.Pointer {
	if pointer == nil || uintptr(pointer)%alignment != 0 || stride%uint64(alignment) != 0 || stride < uint64(minimum) || uint64(index) > uint64(^uint(0)>>1)/stride {
		panic(bindingFailure{newBindingError(ErrNative, "invalid native array layout")})
	}
	return unsafe.Add(pointer, uintptr(index)*uintptr(stride))
}

func bindingBytes(pointer unsafe.Pointer, count uint64) []byte {
	length := bindingLength(count)
	if length != 0 && pointer == nil {
		panic(bindingFailure{newBindingError(ErrNative, "null native buffer")})
	}
	return append([]byte{}, unsafe.Slice((*byte)(pointer), length)...)
}

func bindingString(pointer unsafe.Pointer, count uint64) string {
	length := bindingLength(count)
	if length != 0 && pointer == nil {
		panic(bindingFailure{newBindingError(ErrNative, "null native text")})
	}
	return string(unsafe.Slice((*byte)(pointer), length))
}

func bindingArenaString(pointer unsafe.Pointer, size, offset, count uint64) string {
	if offset > size || count > size-offset {
		panic(bindingFailure{newBindingError(ErrNative, "invalid native message range")})
	}
	if count == 0 {
		return ""
	}
	if pointer == nil {
		panic(bindingFailure{newBindingError(ErrNative, "null native message arena")})
	}
	return bindingString(unsafe.Add(pointer, bindingLength(offset)), count)
}

type bindingState struct {
	mu             sync.Mutex
	raw            uint64
	issued         uint64
	readers        int
	closing        bool
	dispose        func(uint64)
	deciding       bool
	completed      bool
	completing     bool
	pendingRelease uint64
}

type bindingOwner struct {
	state   *bindingState
	parent  any
	rootsMu sync.Mutex
	roots   map[cgo.Handle]*bindingCallbackTicket
}

func (owner *bindingOwner) bindingAcquire(read bool) (uint64, func()) {
	if owner == nil || owner.state == nil {
		panic(bindingFailure{newBindingError(ErrInvalidState, "nil handle")})
	}
	state := owner.state
	state.mu.Lock()
	if state.raw == 0 || state.closing {
		state.mu.Unlock()
		panic(bindingFailure{newBindingError(ErrInvalidState, "handle is closed")})
	}
	raw := state.raw
	if read {
		state.readers++
	}
	state.mu.Unlock()
	return raw, func() {
		if read {
			state.mu.Lock()
			state.readers--
			state.mu.Unlock()
		}
		runtime.KeepAlive(owner)
	}
}

func (owner *bindingOwner) IsClosed() bool {
	if owner == nil || owner.state == nil {
		return true
	}
	owner.state.mu.Lock()
	defer owner.state.mu.Unlock()
	return owner.state.raw == 0
}

func bindingAdopt(raw uint64, parent any, dispose func(uint64)) *bindingOwner {
	if raw == 0 {
		panic(bindingFailure{newBindingError(ErrNative, "native returned a null owner")})
	}
	state := &bindingState{raw: raw, issued: raw, dispose: dispose}
	owner := &bindingOwner{state: state, parent: parent, roots: make(map[cgo.Handle]*bindingCallbackTicket)}
	runtime.AddCleanup(owner, func(state *bindingState) {
		state.mu.Lock()
		raw := state.raw
		state.raw = 0
		state.mu.Unlock()
		if raw != 0 {
			state.dispose(raw)
		}
	}, state)
	return owner
}

type bindingClose struct {
	state     *bindingState
	raw       uint64
	committed bool
}

func (state *bindingState) reserveClose() (uint64, *bindingClose) {
	if state == nil {
		panic(bindingFailure{newBindingError(ErrInvalidState, "nil handle")})
	}
	state.mu.Lock()
	defer state.mu.Unlock()
	if state.closing || state.readers != 0 {
		panic(bindingFailure{newBindingError(ErrBusy, "handle has an active read or close")})
	}
	transaction := &bindingClose{state: state, raw: state.raw}
	state.closing = true
	return state.raw, transaction
}
func (transaction *bindingClose) commit() { transaction.committed = true }
func (transaction *bindingClose) finish() {
	state := transaction.state
	state.mu.Lock()
	defer state.mu.Unlock()
	if transaction.committed {
		state.raw = 0
	}
	state.closing = false
	runtime.KeepAlive(state)
}

// Callback roots belong to their Go owner. Native stores a weak ticket so that
// a callback closure that captures its owner remains collectible as a cycle.
var bindingGlobalRoots = struct {
	sync.Mutex
	values map[cgo.Handle]*bindingCallbackTicket
}{values: make(map[cgo.Handle]*bindingCallbackTicket)}

var bindingCallbackCount atomic.Int64

type bindingCallbackTicket struct {
	mu sync.Mutex
	id cgo.Handle
	// cell is the C memory holding id, whose address native stores as the
	// registration's user data.
	cell     unsafe.Pointer
	value    any
	owner    weak.Pointer[bindingOwner]
	identity uint64
	retired  bool
}

func (arena *bindingArena) register(value any, identity uint64) unsafe.Pointer {
	ticket := &bindingCallbackTicket{value: value, identity: identity}
	ticket.id = cgo.NewHandle(weak.Make(ticket))
	ticket.cell = bindingHandleCell(ticket.id)
	bindingCallbackCount.Add(1)
	arena.callbacks = append(arena.callbacks, ticket)
	return ticket.cell
}

// bindingHandleCell copies a handle into C memory, whose address is what a
// native user_data field holds. See binding_handle_cell in binding_callback.h.
func bindingHandleCell(handle cgo.Handle) unsafe.Pointer {
	cell := C.binding_handle_cell(C.uintptr_t(handle))
	if cell == nil {
		handle.Delete()
		panic(bindingFailure{newBindingError(ErrNative, "native handle cell allocation failed")})
	}
	return cell
}

// bindingHandleOf reads the handle that a cell from bindingHandleCell holds.
func bindingHandleOf(cell unsafe.Pointer) cgo.Handle {
	return cgo.Handle(uintptr(C.binding_handle_value(cell)))
}

func bindingTicket(pointer unsafe.Pointer) *bindingCallbackTicket {
	if pointer == nil {
		return nil
	}
	return bindingHandleOf(pointer).Value().(weak.Pointer[bindingCallbackTicket]).Value()
}

func (ticket *bindingCallbackTicket) release() {
	ticket.mu.Lock()
	defer ticket.mu.Unlock()
	if ticket.retired {
		return
	}
	ticket.retired = true
	ticket.id.Delete()
	C.binding_handle_free(ticket.cell)
	bindingCallbackCount.Add(-1)
	if owner := ticket.owner.Value(); owner != nil {
		owner.rootsMu.Lock()
		delete(owner.roots, ticket.id)
		owner.rootsMu.Unlock()
	} else {
		bindingGlobalRoots.Lock()
		delete(bindingGlobalRoots.values, ticket.id)
		bindingGlobalRoots.Unlock()
	}
	ticket.value = nil
}

func bindingCallbackValue[T any](pointer unsafe.Pointer) (T, bool) {
	var zero T
	ticket := bindingTicket(pointer)
	if ticket == nil {
		return zero, false
	}
	ticket.mu.Lock()
	defer ticket.mu.Unlock()
	value, ok := ticket.value.(T)
	if ticket.identity != 0 {
		ticket.value = nil
	}
	return value, ok
}

// Native threads enter the two exports below, so a panic must not cross cgo:
// it would abort the process. Like the generated trampolines, they recover.

//export mlnGoCallbackRelease
func mlnGoCallbackRelease(pointer unsafe.Pointer) {
	defer func() { _ = recover() }()
	if ticket := bindingTicket(pointer); ticket != nil {
		ticket.release()
	} else if pointer != nil {
		// The collector reclaimed the ticket, whose cell native still held.
		bindingHandleOf(pointer).Delete()
		C.binding_handle_free(pointer)
		bindingCallbackCount.Add(-1)
	}
}

//export mlnGoCallbackOwner
func mlnGoCallbackOwner(pointer unsafe.Pointer) (owner C.uint64_t) {
	defer func() {
		if recover() != nil {
			owner = 0
		}
	}()
	if ticket := bindingTicket(pointer); ticket != nil {
		return C.uint64_t(ticket.identity)
	}
	return 0
}

// bindingAdmission needs no thread pin: a goroutine inside a cgo callback is
// already locked to the callback's thread, and every other goroutine sees an
// empty policy stack on whichever thread it runs.
func bindingAdmission(operation uint32, owner uint64) {
	if !bool(C.binding_policy_check(C.uint32_t(operation), C.uint64_t(owner))) {
		panic(bindingFailure{newBindingError(ErrInvalidState, "operation is unavailable in this native callback")})
	}
}

type bindingScope struct {
	alive  atomic.Bool
	thread uintptr
}

func bindingNewScope() *bindingScope {
	scope := &bindingScope{thread: uintptr(C.binding_thread())}
	scope.alive.Store(true)
	return scope
}

func (scope *bindingScope) check() {
	if scope == nil || !scope.alive.Load() || scope.thread != uintptr(C.binding_thread()) {
		panic(bindingFailure{newBindingError(ErrInvalidState, "callback response is expired or used from another thread")})
	}
}

func (state *bindingState) beginDecision() { state.mu.Lock(); state.deciding = true; state.mu.Unlock() }

func (state *bindingState) finishDecision(decision, accept, pass uint32) uint32 {
	state.mu.Lock()
	state.deciding = false
	var release uint64
	if state.completed || state.completing || state.pendingRelease != 0 || decision == accept {
		decision = accept
		if !state.completing {
			release = state.pendingRelease
			state.pendingRelease = 0
		}
	} else {
		state.raw = 0
		state.pendingRelease = 0
		decision = pass
	}
	state.mu.Unlock()
	if release != 0 {
		state.dispose(release)
	}
	return decision
}

func (state *bindingState) reserveCompletion() (uint64, func(bool)) {
	state.mu.Lock()
	if state.raw == 0 || state.closing || state.completed || state.completing {
		state.mu.Unlock()
		panic(bindingFailure{newBindingError(ErrInvalidState, "request is closed or completion is already accepted")})
	}
	state.completing = true
	raw := state.raw
	state.mu.Unlock()
	return raw, func(accepted bool) {
		state.mu.Lock()
		state.completing = false
		state.completed = state.completed || accepted
		var release uint64
		if !state.deciding {
			release = state.pendingRelease
			state.pendingRelease = 0
		}
		state.mu.Unlock()
		if release != 0 {
			state.dispose(release)
		}
	}
}

func (state *bindingState) closeDecision() {
	state.mu.Lock()
	raw := state.raw
	if raw == 0 {
		state.mu.Unlock()
		return
	}
	state.raw = 0
	if state.deciding || state.completing {
		state.pendingRelease = raw
		raw = 0
	}
	state.mu.Unlock()
	if raw != 0 {
		state.dispose(raw)
	}
}

// ID returns the live generation ID used to correlate native events.
func (owner *bindingOwner) Id() (uint64, error) {
	return bindingCall(func() uint64 { raw, done := owner.bindingAcquire(false); defer done(); return raw })
}
