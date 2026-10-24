//go:build mlntest

package maplibre

// The recorder behind the test suite's native registrations. Their C
// callbacks, in the preamble of testhook.go, stand in for host code in C and
// report to the exports below. A file that exports a Go function keeps only
// declarations in its preamble, so the callbacks live in another file.

import "C"

import (
	"runtime/cgo"
	"sync/atomic"
	"unsafe"
)

// nativeRecorderForTest is the context of a native registration whose
// callbacks are C functions. It counts their calls and offers each callback
// and each release to a channel.
type nativeRecorderForTest struct {
	// cell is the C address that the registration carries as its user data.
	cell     unsafe.Pointer
	calls    chan struct{}
	released chan struct{}
	called   atomic.Int64
	unlocked atomic.Int64
	releases atomic.Int64
}

// newNativeRecorderForTest offers each callback to calls, which may be nil.
// The recorder lives until the process exits, because native code may still
// release it after a failed test ends.
func newNativeRecorderForTest(calls chan struct{}) *nativeRecorderForTest {
	recorder := &nativeRecorderForTest{calls: calls, released: make(chan struct{}, 2)}
	recorder.cell = bindingHandleCell(cgo.NewHandle(recorder))
	return recorder
}

func nativeRecorderOf(userData unsafe.Pointer) *nativeRecorderForTest {
	return bindingHandleOf(userData).Value().(*nativeRecorderForTest)
}

//export mlnGoTestNativeCall
func mlnGoTestNativeCall(userData unsafe.Pointer) {
	recorder := nativeRecorderOf(userData)
	recorder.called.Add(1)
	if recorder.calls != nil {
		select {
		case recorder.calls <- struct{}{}:
		default:
		}
	}
}

//export mlnGoTestNativeUnlock
func mlnGoTestNativeUnlock(userData unsafe.Pointer) {
	nativeRecorderOf(userData).unlocked.Add(1)
}

//export mlnGoTestNativeRelease
func mlnGoTestNativeRelease(userData unsafe.Pointer) {
	recorder := nativeRecorderOf(userData)
	recorder.releases.Add(1)
	select {
	case recorder.released <- struct{}{}:
	default:
	}
}
