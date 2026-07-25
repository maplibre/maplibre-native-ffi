package memory

import "runtime"

// KeepAliveAll keeps values reachable until this point after a cgo call.
func KeepAliveAll(values ...any) {
	for _, value := range values {
		runtime.KeepAlive(value)
	}
}
