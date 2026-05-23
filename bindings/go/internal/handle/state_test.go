package handle

import (
	"bytes"
	"log"
	"strings"
	"sync/atomic"
	"testing"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

type testNativeHandle struct {
	value int
}

func TestStateRejectsNilPointer(t *testing.T) {
	state, err := New[testNativeHandle](nil, "test_handle")
	if err == nil {
		t.Fatal("New(nil) succeeded")
	}
	if state != nil {
		t.Fatalf("New(nil) state = %#v, want nil", state)
	}
}

func TestStateCloseIsIdempotentAfterSuccess(t *testing.T) {
	native := &testNativeHandle{value: 1}
	state, err := New(native, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	var calls atomic.Int32
	destroy := func(ptr *testNativeHandle) capi.Status {
		if ptr != native {
			t.Fatalf("destroy pointer = %p, want %p", ptr, native)
		}
		calls.Add(1)
		return capi.StatusOK
	}

	if status := state.Close(destroy); status != capi.StatusOK {
		t.Fatalf("first Close status = %d, want OK", status)
	}
	if status := state.Close(destroy); status != capi.StatusOK {
		t.Fatalf("second Close status = %d, want OK", status)
	}
	if got := calls.Load(); got != 1 {
		t.Fatalf("destroy calls = %d, want 1", got)
	}
	if !state.IsClosed() {
		t.Fatal("state is live after successful close")
	}
}

func TestStateFailedCloseLeavesHandleLiveForRetry(t *testing.T) {
	native := &testNativeHandle{value: 1}
	state, err := New(native, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	var calls atomic.Int32
	destroy := func(*testNativeHandle) capi.Status {
		if calls.Add(1) == 1 {
			return capi.StatusWrongThread
		}
		return capi.StatusOK
	}

	if status := state.Close(destroy); status != capi.StatusWrongThread {
		t.Fatalf("first Close status = %d, want wrong-thread", status)
	}
	if ptr, live := state.Ptr(); !live || ptr != native {
		t.Fatalf("Ptr() = %p, %v; want live native pointer", ptr, live)
	}
	if status := state.Close(destroy); status != capi.StatusOK {
		t.Fatalf("second Close status = %d, want OK", status)
	}
	if got := calls.Load(); got != 2 {
		t.Fatalf("destroy calls = %d, want 2", got)
	}
}

func TestStateKeepsParentsReachable(t *testing.T) {
	parent := &testNativeHandle{value: 7}
	native := &testNativeHandle{value: 1}
	state, err := New(native, "test_handle", parent)
	if err != nil {
		t.Fatal(err)
	}

	state.KeepAlive()
	if got := state.TypeName(); got != "test_handle" {
		t.Fatalf("TypeName() = %q, want test_handle", got)
	}
}

func TestStateLeakReportDoesNotDestroyHandle(t *testing.T) {
	native := &testNativeHandle{value: 1}
	state, err := New(native, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	var buf bytes.Buffer
	oldWriter := log.Writer()
	oldFlags := log.Flags()
	log.SetOutput(&buf)
	log.SetFlags(0)
	defer func() {
		log.SetOutput(oldWriter)
		log.SetFlags(oldFlags)
	}()

	state.reportLeakIfLive()
	if got := buf.String(); !strings.Contains(got, "maplibre: leaked test_handle") {
		t.Fatalf("leak report = %q, want leaked test_handle", got)
	}
	if ptr, live := state.Ptr(); !live || ptr != native {
		t.Fatalf("Ptr() after leak report = %p, %v; want live native pointer", ptr, live)
	}
}

func TestStateLeakReportIgnoresClosedHandle(t *testing.T) {
	native := &testNativeHandle{value: 1}
	state, err := New(native, "test_handle")
	if err != nil {
		t.Fatal(err)
	}
	if status := state.Close(func(*testNativeHandle) capi.Status { return capi.StatusOK }); status != capi.StatusOK {
		t.Fatalf("Close status = %d, want OK", status)
	}

	var buf bytes.Buffer
	oldWriter := log.Writer()
	oldFlags := log.Flags()
	log.SetOutput(&buf)
	log.SetFlags(0)
	defer func() {
		log.SetOutput(oldWriter)
		log.SetFlags(oldFlags)
	}()

	state.reportLeakIfLive()
	if got := buf.String(); got != "" {
		t.Fatalf("leak report after close = %q, want empty", got)
	}
}
