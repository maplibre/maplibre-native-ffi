package callback

/*
#cgo CFLAGS: -std=c2x
#include <stdint.h>
#include <stdlib.h>
#include "maplibre_native_c.h"

extern uint32_t goMaplibreLogCallback(void* user_data, uint32_t severity, uint32_t event, int64_t code, const char* message);

static inline void* mln_go_handle_to_pointer(uintptr_t handle) {
  return (void*)handle;
}
*/
import "C"
import (
	"runtime/cgo"
	"sync"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
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
func SetLogCallback(callback LogCallback) capi.Status {
	if callback == nil {
		return ClearLogCallback()
	}

	state := &logCallbackState{callback: callback}
	handle := cgo.NewHandle(state)
	status := capi.Status(C.mln_log_set_callback(
		(C.mln_log_callback)(C.goMaplibreLogCallback),
		C.mln_go_handle_to_pointer(C.uintptr_t(handle)),
	))
	if status != capi.StatusOK {
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
	return capi.StatusOK
}

// ClearLogCallback clears the process-global native log callback.
func ClearLogCallback() capi.Status {
	status := capi.Status(C.mln_log_clear_callback())
	if status != capi.StatusOK {
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
	return capi.StatusOK
}

// SetAsyncLogSeverityMask sets the native asynchronous logging severity mask.
func SetAsyncLogSeverityMask(mask uint32) capi.Status {
	return capi.Status(C.mln_log_set_async_severity_mask(C.uint32_t(mask)))
}

func invokeLogCallbackForTest(callback LogCallback) uint32 {
	state := &logCallbackState{callback: callback}
	handle := cgo.NewHandle(state)
	defer handle.Delete()
	message := C.CString("test message")
	defer C.free(unsafe.Pointer(message))
	return uint32(goMaplibreLogCallback(
		C.mln_go_handle_to_pointer(C.uintptr_t(handle)),
		C.uint32_t(capi.LogSeverityInfo),
		C.uint32_t(capi.LogEventGeneral),
		0,
		message,
	))
}
