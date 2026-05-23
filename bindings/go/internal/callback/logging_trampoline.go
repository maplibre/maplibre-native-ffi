package callback

/*
#include <stdint.h>
*/
import "C"
import (
	"runtime/cgo"
	"unsafe"
)

//export goMaplibreLogCallback
func goMaplibreLogCallback(userData unsafe.Pointer, severity C.uint32_t, event C.uint32_t, code C.int64_t, message *C.char) C.uint32_t {
	defer func() {
		_ = recover()
	}()

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*logCallbackState)
	if !ok || state == nil || state.callback == nil {
		return 0
	}
	if state.callback(uint32(severity), uint32(event), int64(code), C.GoString(message)) {
		return 1
	}
	return 0
}
