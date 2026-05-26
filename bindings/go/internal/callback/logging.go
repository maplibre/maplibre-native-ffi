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
	"runtime/cgo"
	"sync"
	"unsafe"
)

// LogCallback is the internal shape for process-global log callbacks.
type LogCallback func(severity uint32, event uint32, code int64, message string) bool

type logCallbackState struct {
	callback LogCallback
}

var logState struct {
	sync.Mutex
	handle cgo.Handle
	active bool
}

// SetLogCallback installs or replaces the process-global native log callback.
func SetLogCallback(callback LogCallback) int32 {
	if callback == nil {
		return ClearLogCallback()
	}

	state := &logCallbackState{callback: callback}
	handle := cgo.NewHandle(state)
	status := int32(C.mln_log_set_callback(
		(C.mln_log_callback)(C.goMaplibreLogCallback),
		C.mln_go_handle_to_pointer(C.uintptr_t(handle)),
	))
	if status != int32(C.MLN_STATUS_OK) {
		handle.Delete()
		return status
	}

	logState.Lock()
	oldHandle := logState.handle
	oldActive := logState.active
	logState.handle = handle
	logState.active = true
	logState.Unlock()

	if oldActive {
		oldHandle.Delete()
	}
	return int32(C.MLN_STATUS_OK)
}

// ClearLogCallback clears the process-global native log callback.
func ClearLogCallback() int32 {
	status := int32(C.mln_log_clear_callback())
	if status != int32(C.MLN_STATUS_OK) {
		return status
	}

	logState.Lock()
	oldHandle := logState.handle
	oldActive := logState.active
	logState.handle = cgo.Handle(0)
	logState.active = false
	logState.Unlock()

	if oldActive {
		oldHandle.Delete()
	}
	return int32(C.MLN_STATUS_OK)
}

// SetAsyncLogSeverityMask sets the native asynchronous logging severity mask.
func SetAsyncLogSeverityMask(mask uint32) int32 {
	return int32(C.mln_log_set_async_severity_mask(C.uint32_t(mask)))
}

func invokeLogCallbackForTest(callback LogCallback) uint32 {
	state := &logCallbackState{callback: callback}
	handle := cgo.NewHandle(state)
	defer handle.Delete()
	message := C.CString("test message")
	defer C.free(unsafe.Pointer(message))
	return uint32(goMaplibreLogCallback(
		C.mln_go_handle_to_pointer(C.uintptr_t(handle)),
		C.uint32_t(C.MLN_LOG_SEVERITY_INFO),
		C.uint32_t(C.MLN_LOG_EVENT_GENERAL),
		0,
		message,
	))
}
