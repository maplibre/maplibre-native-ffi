package maplibre

import (
	"errors"
	"fmt"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
	internalstatus "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/status"
)

var (
	// ErrInvalidArgument reports invalid arguments, released handles, invalid
	// enum values, invalid string shapes, or other binding-owned argument errors.
	ErrInvalidArgument = errors.New("maplibre: invalid argument")
	// ErrInvalidState reports valid objects used in an invalid lifecycle state.
	ErrInvalidState = errors.New("maplibre: invalid state")
	// ErrWrongThread reports use of an owner-thread-affine handle from the wrong
	// OS thread.
	ErrWrongThread = errors.New("maplibre: wrong thread")
	// ErrUnsupported reports a backend, platform, or operation unavailable in
	// the linked native build.
	ErrUnsupported = errors.New("maplibre: unsupported")
	// ErrNative reports a MapLibre Native error converted to a C status.
	ErrNative = errors.New("maplibre: native error")
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

func checkNative(call func() capi.Status) error {
	failure := internalstatus.CheckCall(call)
	if failure == nil {
		return nil
	}
	return newStatusError(failure)
}

func kindForStatus(status capi.Status) error {
	switch status {
	case capi.StatusInvalidArgument:
		return ErrInvalidArgument
	case capi.StatusInvalidState:
		return ErrInvalidState
	case capi.StatusWrongThread:
		return ErrWrongThread
	case capi.StatusUnsupported:
		return ErrUnsupported
	case capi.StatusNativeError:
		return ErrNative
	default:
		return ErrUnknownStatus
	}
}
