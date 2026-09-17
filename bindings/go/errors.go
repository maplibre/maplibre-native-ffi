package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"strings"

	internalstatus "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/status"
)

var (
	// ErrInvalidArgument reports invalid arguments, released handles, invalid
	// enum values, invalid string shapes, or other binding-owned argument errors.
	ErrInvalidArgument = errors.New("maplibre: invalid argument")
	// ErrInvalidState reports valid objects used in an invalid lifecycle state.
	ErrInvalidState = errors.New("maplibre: invalid state")
	// ErrWrongThread reports use of a thread-affine render handle from the
	// wrong OS thread.
	ErrWrongThread = errors.New("maplibre: wrong thread")
	// ErrUnsupported reports a backend, platform, or operation unavailable in
	// the linked native build.
	ErrUnsupported = errors.New("maplibre: unsupported")
	// ErrNative reports a MapLibre Native error converted to a C status.
	ErrNative = errors.New("maplibre: native error")
	// ErrCancelled reports a terminal cancelled operation.
	ErrCancelled = errors.New("maplibre: cancelled")
	// ErrBusy reports a conflicting driver or lifecycle call.
	ErrBusy = errors.New("maplibre: busy")
	// ErrTargetLost reports irreversible render-target loss.
	ErrTargetLost = errors.New("maplibre: target lost")
	// ErrNotReady reports that a nonblocking call has no result yet.
	ErrNotReady = errors.New("maplibre: not ready")
	// ErrNotFound reports a command or operation that named an ID with no live
	// object behind it.
	ErrNotFound = errors.New("maplibre: not found")
	// ErrABIVersionMismatch reports that the loaded C ABI version is
	// incompatible with this binding.
	ErrABIVersionMismatch = errors.New("maplibre: ABI version mismatch")
	// ErrUnknownStatus reports a status value unknown to this binding version.
	ErrUnknownStatus = errors.New("maplibre: unknown native status")
)

// Error describes a MapLibre Native binding or C status failure.
type Error struct {
	kind       error
	rawStatus  int32
	hasStatus  bool
	diagnostic string
}

func newBindingError(kind error, diagnostic string) *Error {
	return &Error{kind: kind, diagnostic: diagnostic}
}

func newABIVersionMismatchError(expected, actual uint32) *Error {
	return newBindingError(
		ErrABIVersionMismatch,
		fmt.Sprintf("unsupported MapLibre Native C ABI version %d; expected %d", actual, expected),
	)
}

func newStatusError(failure *internalstatus.NativeError) *Error {
	return &Error{
		kind:       kindForStatus(failure.Status),
		rawStatus:  int32(failure.Status),
		hasStatus:  true,
		diagnostic: failure.Diagnostic,
	}
}

// Error returns a human-readable failure description.
func (e *Error) Error() string {
	if e == nil {
		return "<nil>"
	}
	if e.hasStatus {
		return fmt.Sprintf("MapLibre Native status %d: %s", e.rawStatus, e.diagnostic)
	}
	return e.diagnostic
}

// Unwrap returns the stable category sentinel so errors.Is works.
func (e *Error) Unwrap() error {
	if e == nil {
		return nil
	}
	return e.kind
}

// RawStatus returns the C status value and whether this error came from native
// status conversion.
func (e *Error) RawStatus() (int32, bool) {
	if e == nil {
		return 0, false
	}
	return e.rawStatus, e.hasStatus
}

// Diagnostic returns the copied native diagnostic or binding-owned message.
func (e *Error) Diagnostic() string {
	if e == nil {
		return ""
	}
	return e.diagnostic
}

// validateCStringArgument rejects a value that C would truncate at an embedded
// NUL byte. name is the argument name the diagnostic reports.
func validateCStringArgument(name string, value string) error {
	if strings.IndexByte(value, 0) >= 0 {
		return newBindingError(ErrInvalidArgument, name+" contains embedded NUL")
	}
	return nil
}

func checkNative[S ~int32](call func() S) error {
	failure := internalstatus.CheckCall(call, threadLastErrorMessage)
	if failure == nil {
		return nil
	}
	return newStatusError(failure)
}

func threadLastErrorMessage() string {
	return C.GoString(C.mln_thread_last_error_message())
}

func kindForStatus(status int32) error {
	switch status {
	case int32(C.MLN_STATUS_INVALID_ARGUMENT):
		return ErrInvalidArgument
	case int32(C.MLN_STATUS_INVALID_STATE):
		return ErrInvalidState
	case int32(C.MLN_STATUS_WRONG_THREAD):
		return ErrWrongThread
	case int32(C.MLN_STATUS_UNSUPPORTED):
		return ErrUnsupported
	case int32(C.MLN_STATUS_NATIVE_ERROR):
		return ErrNative
	case int32(C.MLN_STATUS_CANCELLED):
		return ErrCancelled
	case int32(C.MLN_STATUS_BUSY):
		return ErrBusy
	case int32(C.MLN_STATUS_TARGET_LOST):
		return ErrTargetLost
	case int32(C.MLN_STATUS_NOT_READY):
		return ErrNotReady
	case int32(C.MLN_STATUS_NOT_FOUND):
		return ErrNotFound
	default:
		return ErrUnknownStatus
	}
}
