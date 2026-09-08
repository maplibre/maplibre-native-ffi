//go:build darwin && cgo

package testsupport

/*
#cgo darwin LDFLAGS: -framework Metal -lobjc
#include <stdint.h>

void* MTLCreateSystemDefaultDevice(void);
void objc_release(void* object);

static inline void mln_go_test_release_metal_device(uintptr_t device) {
	if (device != 0) {
		objc_release((void*)device);
	}
}
*/
import "C"

// DefaultMetalDevice returns a default Metal device with one caller-owned
// reference. The caller must balance it with ReleaseMetalDevice.
func DefaultMetalDevice() uintptr {
	return uintptr(C.MTLCreateSystemDefaultDevice())
}

// ReleaseMetalDevice balances DefaultMetalDevice's retained return value.
func ReleaseMetalDevice(device uintptr) {
	C.mln_go_test_release_metal_device(C.uintptr_t(device))
}
