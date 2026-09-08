package callback

/*
#cgo CFLAGS: -std=c2x
#include <stdint.h>
#include <stdlib.h>
#include "../cgo_shim.h"

extern uint32_t goMaplibreLogCallback(void* user_data, uint32_t severity, uint32_t event, int64_t code, const char* message);
extern void goMaplibreReleaseCallbackState(void* user_data);
*/
import "C"

import (
	"runtime/cgo"
	"sync"
	"sync/atomic"
	"unsafe"
)

// liveLogCallbacks counts the log callback states this package holds a cgo
// handle for. LogCallbackLiveCountForTest reads it.
var liveLogCallbacks atomic.Int64

// LogCallbackLiveCountForTest reports how many log callback states are still
// alive, which is how a test observes that the C API's release callback freed
// one.
func LogCallbackLiveCountForTest() int64 {
	return liveLogCallbacks.Load()
}

// LogCallback is the internal shape for process-global log callbacks.
type LogCallback func(severity uint32, event uint32, code int64, message string) bool

type LogCallbackState struct {
	callback LogCallback
	handle   cgo.Handle
	once     sync.Once
}

func newLogCallbackState(callback LogCallback) *LogCallbackState {
	state := &LogCallbackState{callback: callback}
	state.handle = cgo.NewHandle(state)
	liveLogCallbacks.Add(1)
	return state
}

func (state *LogCallbackState) Release() {
	if state != nil {
		state.once.Do(func() {
			state.handle.Delete()
			liveLogCallbacks.Add(-1)
		})
	}
}

// SetLogCallback installs or replaces the process-global native log callback.
func SetLogCallback(callback LogCallback) int32 {
	if callback == nil {
		return ClearLogCallback()
	}

	state := newLogCallbackState(callback)
	status := int32(C.mln_log_set_callback(
		C.mln_log_callback(C.goMaplibreLogCallback),
		C.mln_go_handle_to_pointer(C.uintptr_t(state.handle)),
		C.mln_log_callback_release(C.goMaplibreReleaseCallbackState),
	))
	if status != int32(C.MLN_STATUS_OK) {
		state.Release()
		return status
	}
	return int32(C.MLN_STATUS_OK)
}

// ClearLogCallback clears the process-global native log callback.
func ClearLogCallback() int32 {
	return int32(C.mln_log_clear_callback())
}

// SetAsyncLogSeverityMask sets the native asynchronous logging severity mask.
func SetAsyncLogSeverityMask(mask uint32) int32 {
	return int32(C.mln_log_set_async_severity_mask(C.uint32_t(mask)))
}

func invokeLogCallbackForTest(callback LogCallback) uint32 {
	state := newLogCallbackState(callback)
	defer state.Release()

	message := C.CString("test message")
	defer C.free(unsafe.Pointer(message))
	return uint32(goMaplibreLogCallback(
		C.mln_go_handle_to_pointer(C.uintptr_t(state.handle)),
		C.uint32_t(C.MLN_LOG_SEVERITY_INFO),
		C.uint32_t(C.MLN_LOG_EVENT_GENERAL),
		0,
		message,
	))
}
