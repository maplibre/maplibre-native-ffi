package maplibre

/*
#include "binding_callback.h"
#include "maplibre_native_c.h"
*/
import "C"

import (
	"log/slog"
	"runtime"
	"runtime/cgo"
	"runtime/debug"
	"strings"
	"sync"
	"sync/atomic"
	"unsafe"
	"weak"
)

const bindingCABIVersion = 0

// A native library with another C ABI version would misread every struct this
// binding passes, so loading one fails before any call can reach it.
func init() {
	if err := checkCABIVersion(uint32(C.mln_c_version())); err != nil {
		panic(err)
	}
}

func checkCABIVersion(version uint32) error {
	if version != bindingCABIVersion {
		return &ABIVersionMismatchError{Expected: bindingCABIVersion, Actual: version}
	}
	return nil
}

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
	owners      []*bindingOwner
}

func bindingHandleState(owner *bindingOwner) *bindingState {
	if owner == nil || owner.state == nil {
		panic(bindingFailure{newBindingError(StatusInvalidArgument, "nil handle")})
	}
	return owner.state
}

func (arena *bindingArena) lease(owner *bindingOwner) uint64 {
	state := bindingHandleState(owner)
	arena.owners = append(arena.owners, owner)
	return state.raw
}

func (arena *bindingArena) fail(message string) {
	panic(bindingFailure{newBindingError(StatusInvalidArgument, message)})
}

func (arena *bindingArena) allocate(size uintptr) unsafe.Pointer {
	if size == 0 {
		return nil
	}
	result := C.calloc(1, C.size_t(size))
	if result == nil {
		panic(bindingFailure{newBindingError(StatusNativeError, "native input allocation failed")})
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
	defer runtime.KeepAlive(arena.owners)
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

// decline releases the callback roots of a registration that native reported
// it kept nothing from, so a later accept moves none of them to the receiver.
func (arena *bindingArena) decline() {
	for _, ticket := range arena.callbacks {
		ticket.release()
	}
	arena.callbacks = nil
}

func (arena *bindingArena) array(count int, size uintptr) unsafe.Pointer {
	if size != 0 && uintptr(count) > ^uintptr(0)/size {
		arena.fail("input array exceeds address space")
	}
	return arena.allocate(uintptr(count) * size)
}

func bindingCount[T ~int8 | ~int16 | ~int32 | ~int64 | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~int | ~uint](count int) T {
	result := T(count)
	if count < 0 || result < 0 || uint64(result) != uint64(count) {
		panic(bindingFailure{newBindingError(StatusInvalidArgument, "input count exceeds C capacity")})
	}
	return result
}

func bindingCountLike[T ~int8 | ~int16 | ~int32 | ~int64 | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~int | ~uint](_ T, count int) T {
	return bindingCount[T](count)
}

func bindingLength(count uint64) int {
	if count > uint64(^uint(0)>>1) {
		panic(bindingFailure{newBindingError(StatusNativeError, "native count exceeds Go capacity")})
	}
	return int(count)
}

func bindingElement(pointer unsafe.Pointer, index int, stride uint64, minimum, alignment uintptr) unsafe.Pointer {
	if pointer == nil || uintptr(pointer)%alignment != 0 || stride%uint64(alignment) != 0 || stride < uint64(minimum) || uint64(index) > uint64(^uint(0)>>1)/stride {
		panic(bindingFailure{newBindingError(StatusNativeError, "invalid native array layout")})
	}
	return unsafe.Add(pointer, uintptr(index)*uintptr(stride))
}

func bindingBytes(pointer unsafe.Pointer, count uint64) []byte {
	length := bindingLength(count)
	if length != 0 && pointer == nil {
		panic(bindingFailure{newBindingError(StatusNativeError, "null native buffer")})
	}
	return append([]byte{}, unsafe.Slice((*byte)(pointer), length)...)
}

func bindingString(pointer unsafe.Pointer, count uint64) string {
	length := bindingLength(count)
	if length != 0 && pointer == nil {
		panic(bindingFailure{newBindingError(StatusNativeError, "null native text")})
	}
	return string(unsafe.Slice((*byte)(pointer), length))
}

type bindingState struct {
	typeName string
	raw      uint64
	released atomic.Bool
	reads    *sync.RWMutex
	dispose  func(uint64)
}

type bindingOwner struct {
	state   *bindingState
	parent  any
	rootsMu sync.Mutex
	roots   map[cgo.Handle]*bindingCallbackTicket
}

// bindingAdopt keeps cleanup state separate from the owner so that callback
// cycles and children remain collectible.
func bindingAdopt(raw uint64, parent any, typeName string, guarded bool, dispose func(uint64)) *bindingOwner {
	state := &bindingState{typeName: typeName, raw: raw, dispose: dispose}
	if guarded {
		state.reads = &sync.RWMutex{}
	}
	owner := &bindingOwner{state: state, parent: parent, roots: make(map[cgo.Handle]*bindingCallbackTicket)}
	runtime.AddCleanup(owner, func(state *bindingState) {
		if !state.released.Load() {
			state.dispose(state.raw)
			slog.Warn("maplibre: leaked handle; close it explicitly", "type", state.typeName, "handle", state.raw)
		}
	}, state)
	return owner
}

// Callback roots belong to their Go owner. Native stores a weak ticket so that
// a callback closure that captures its owner remains collectible as a cycle.
var bindingGlobalRoots = struct {
	sync.Mutex
	values map[cgo.Handle]*bindingCallbackTicket
}{values: make(map[cgo.Handle]*bindingCallbackTicket)}

type bindingCallbackTicket struct {
	mu sync.Mutex
	id cgo.Handle
	// cell is the C memory holding id, whose address native stores as the
	// registration's user data.
	cell    unsafe.Pointer
	value   any
	owner   weak.Pointer[bindingOwner]
	retired bool
}

func (arena *bindingArena) register(value any) unsafe.Pointer {
	ticket := &bindingCallbackTicket{value: value}
	ticket.id = cgo.NewHandle(weak.Make(ticket))
	ticket.cell = bindingHandleCell(ticket.id)
	arena.callbacks = append(arena.callbacks, ticket)
	return ticket.cell
}

// bindingHandleCell copies a handle into C memory, whose address is what a
// native user_data field holds. See binding_handle_cell in binding_callback.h.
func bindingHandleCell(handle cgo.Handle) unsafe.Pointer {
	cell := C.binding_handle_cell(C.uintptr_t(handle))
	if cell == nil {
		handle.Delete()
		panic(bindingFailure{newBindingError(StatusNativeError, "native handle cell allocation failed")})
	}
	return cell
}

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
	return value, ok
}

// bindingReportCallbackPanic reports a panic that the trampoline of the C
// callback type named callback recovered, through the default slog logger,
// which a host replaces with slog.SetDefault. Native receives the callback's
// failure value instead.
func bindingReportCallbackPanic(callback string, failure any) {
	// A panicking handler must not escape the trampoline either.
	defer func() { _ = recover() }()
	slog.Error("maplibre: callback panicked", "callback", callback, "panic", failure, "stack", string(debug.Stack()))
}

// Native threads enter the export below, so a panic must not cross cgo:
// it would abort the process. Like the generated trampolines, they recover.

//export mlnGoCallbackRelease
func mlnGoCallbackRelease(pointer unsafe.Pointer) {
	defer func() {
		if failure := recover(); failure != nil {
			bindingReportCallbackPanic("mlnGoCallbackRelease", failure)
		}
	}()
	if ticket := bindingTicket(pointer); ticket != nil {
		ticket.release()
	} else if pointer != nil {
		// The collector reclaimed the ticket, whose cell native still held.
		bindingHandleOf(pointer).Delete()
		C.binding_handle_free(pointer)
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
		panic(bindingFailure{newBindingError(StatusInvalidState, "callback response is expired or used from another thread")})
	}
}

// Id returns the native handle value, which runtime events report as their
// source. It stays readable after Close, so events drained later still match
// this handle.
func (owner *bindingOwner) Id() uint64 {
	if owner == nil || owner.state == nil {
		return 0
	}
	return owner.state.raw
}
