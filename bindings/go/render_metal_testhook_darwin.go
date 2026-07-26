//go:build darwin && cgo

package maplibre

/*
#cgo darwin LDFLAGS: -framework Metal
void* MTLCreateSystemDefaultDevice(void);
*/
import "C"

func defaultMetalDeviceForTest() uintptr {
	return uintptr(C.MTLCreateSystemDefaultDevice())
}
