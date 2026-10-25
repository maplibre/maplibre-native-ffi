package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"bytes"
	"fmt"
	"sync"
	"unsafe"
)

// Error carries a native status and the diagnostic that native copied for it.
type Error struct {
	Status     Status
	Diagnostic string
	// Generation is the map snapshot generation that a failed map command
	// published, and zero for any other failure.
	Generation uint64
}

func newBindingError(status Status, diagnostic string) *Error {
	return &Error{Status: status, Diagnostic: diagnostic}
}

func newStatusError(status int32, diagnostic string) *Error {
	return newBindingError(Status(status), diagnostic)
}

func (e *Error) Error() string {
	if e.Diagnostic == "" {
		return e.Status.Error()
	}
	return e.Status.Error() + ": " + e.Diagnostic
}

func (e *Error) Unwrap() error { return e.Status }

// AbiVersionMismatchError identifies the incompatible native and binding versions.
type AbiVersionMismatchError struct {
	Expected uint32
	Actual   uint32
}

func (e *AbiVersionMismatchError) Error() string {
	return fmt.Sprintf("maplibre: unsupported C ABI version %d; this binding supports %d", e.Actual, e.Expected)
}
func (e *AbiVersionMismatchError) Unwrap() error { return StatusUnsupported }

// Native writes a call's diagnostic at call exit, and checkNative copies the
// message before returning the diagnostic to the pool, so calls reuse
// diagnostics instead of allocating a message buffer each. A diagnostic holds
// no Go pointers, so cgo permits passing it to native.
var diagnosticPool = sync.Pool{New: func() any { return new(C.mln_diagnostic) }}

// checkNative runs a status-returning call with a diagnostic and converts a
// failure status into an *Error that carries the call's diagnostic.
func checkNative(call func(*C.mln_diagnostic) int32) error {
	diagnostic := diagnosticPool.Get().(*C.mln_diagnostic)
	defer diagnosticPool.Put(diagnostic)
	diagnostic.size = C.uint32_t(unsafe.Sizeof(*diagnostic))
	// A call without a diagnostic parameter leaves the message untouched, so
	// clear any message a previous call left behind.
	diagnostic.message[0] = 0
	status := call(diagnostic)
	if status == int32(C.MLN_STATUS_OK) {
		return nil
	}
	message := unsafe.Slice((*byte)(unsafe.Pointer(&diagnostic.message[0])), len(diagnostic.message))
	if end := bytes.IndexByte(message, 0); end >= 0 {
		message = message[:end]
	}
	return newStatusError(status, string(message))
}
