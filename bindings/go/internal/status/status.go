// Package status converts C status values into public Go errors while keeping
// diagnostic capture on the same OS thread as the native call.
package status

import (
	"runtime"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

// NativeError is the concrete status failure payload produced by CheckCall.
type NativeError struct {
	Status     capi.Status
	Diagnostic string
}

// CheckCall runs call on a locked OS thread and captures the thread-local
// diagnostic immediately when the C status is non-OK.
func CheckCall(call func() capi.Status) *NativeError {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	status := call()
	if status == capi.StatusOK {
		return nil
	}
	return &NativeError{Status: status, Diagnostic: capi.ThreadLastErrorMessage()}
}
