package callback

/*
#include <stdint.h>
*/
import "C"
import "unsafe"

//export goMaplibreLogCallback
func goMaplibreLogCallback(userData unsafe.Pointer, severity C.uint32_t, event C.uint32_t, code C.int64_t, message *C.char) C.uint32_t {
	defer func() {
		_ = recover()
	}()

	callback := currentLogCallback()
	if callback == nil {
		return 0
	}
	if callback(uint32(severity), uint32(event), int64(code), C.GoString(message)) {
		return 1
	}
	return 0
}
