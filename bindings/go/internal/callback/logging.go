package callback

/*
#cgo CFLAGS: -std=c2x
#include <stdint.h>
#include <stdlib.h>
#include "../cgo_shim.h"

extern uint32_t goMaplibreLogCallback(void* user_data, uint32_t severity, uint32_t event, int64_t code, const char* message);
*/
import "C"
import (
	"sync"
	"unsafe"
)

// LogCallback is the internal shape for process-global log callbacks.
type LogCallback func(severity uint32, event uint32, code int64, message string) bool

var logState struct {
	sync.Mutex
	active   bool
	callback LogCallback
}

var logInstallMu sync.Mutex

var (
	setNativeLogCallback = func() int32 {
		return int32(C.mln_log_set_callback(
			(C.mln_log_callback)(C.goMaplibreLogCallback),
			nil,
		))
	}
	clearNativeLogCallback = func() int32 {
		return int32(C.mln_log_clear_callback())
	}
)

// SetLogCallback installs or replaces the process-global native log callback.
func SetLogCallback(callback LogCallback) int32 {
	if callback == nil {
		return ClearLogCallback()
	}

	logInstallMu.Lock()
	defer logInstallMu.Unlock()

	oldCallback, oldActive := swapCurrentLogCallback(callback, true)
	status := setNativeLogCallback()
	if status != int32(C.MLN_STATUS_OK) {
		swapCurrentLogCallback(oldCallback, oldActive)
		return status
	}
	return int32(C.MLN_STATUS_OK)
}

// ClearLogCallback clears the process-global native log callback.
func ClearLogCallback() int32 {
	logInstallMu.Lock()
	defer logInstallMu.Unlock()

	oldCallback, oldActive := swapCurrentLogCallback(nil, false)
	status := clearNativeLogCallback()
	if status != int32(C.MLN_STATUS_OK) {
		swapCurrentLogCallback(oldCallback, oldActive)
		return status
	}
	return int32(C.MLN_STATUS_OK)
}

// SetAsyncLogSeverityMask sets the native asynchronous logging severity mask.
func SetAsyncLogSeverityMask(mask uint32) int32 {
	return int32(C.mln_log_set_async_severity_mask(C.uint32_t(mask)))
}

func currentLogCallback() LogCallback {
	logState.Lock()
	defer logState.Unlock()
	if !logState.active {
		return nil
	}
	return logState.callback
}

func swapCurrentLogCallback(callback LogCallback, active bool) (LogCallback, bool) {
	logState.Lock()
	defer logState.Unlock()
	oldCallback := logState.callback
	oldActive := logState.active
	logState.callback = callback
	logState.active = active
	return oldCallback, oldActive
}

func invokeLogCallbackForTest(callback LogCallback) uint32 {
	oldCallback, oldActive := swapCurrentLogCallback(callback, callback != nil)
	defer func() {
		swapCurrentLogCallback(oldCallback, oldActive)
	}()

	message := C.CString("test message")
	defer C.free(unsafe.Pointer(message))
	return uint32(goMaplibreLogCallback(
		nil,
		C.uint32_t(C.MLN_LOG_SEVERITY_INFO),
		C.uint32_t(C.MLN_LOG_EVENT_GENERAL),
		0,
		message,
	))
}
