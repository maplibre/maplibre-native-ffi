// Package status converts C status values into public Go errors while keeping
// diagnostic capture on the same OS thread as the native call.
package status

import "runtime"

// NativeError is the concrete status failure payload produced by CheckCall.
type NativeError struct {
	Status     int32
	Diagnostic string
}

// CheckCall runs call on a locked OS thread and captures the thread-local
// diagnostic immediately when the C status is non-OK.
func CheckCall[S ~int32](call func() S, diagnostic func() string) *NativeError {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	status := int32(call())
	if status == 0 {
		return nil
	}
	return &NativeError{Status: status, Diagnostic: diagnostic()}
}
