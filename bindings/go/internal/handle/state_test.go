package handle

import (
	"bytes"
	"log"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
)

const (
	testStatusOK          int32 = 0
	testStatusWrongThread int32 = -3
)

// A synthetic handle for close-once tests. It reaches only the fake destroy
// functions below, never the C API.
type testNativeHandle uint64

const testHandle testNativeHandle = 0x0200_0000_0000_002a

func TestStateRejectsTheNullHandle(t *testing.T) {
	state, err := New[testNativeHandle](0, "test_handle")
	if err == nil {
		t.Fatal("New(0) succeeded")
	}
	if state != nil {
		t.Fatalf("New(0) state = %#v, want nil", state)
	}
}

func TestStateCloseIsIdempotentAfterSuccess(t *testing.T) {
	state, err := New(testHandle, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	var calls atomic.Int32
	destroy := func(handle testNativeHandle) int32 {
		if handle != testHandle {
			t.Fatalf("destroy handle = %#x, want %#x", handle, testHandle)
		}
		calls.Add(1)
		return testStatusOK
	}

	if status := state.Close(destroy); status != testStatusOK {
		t.Fatalf("first Close status = %d, want OK", status)
	}
	if status := state.Close(destroy); status != testStatusOK {
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
	state, err := New(testHandle, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	var calls atomic.Int32
	destroy := func(testNativeHandle) int32 {
		if calls.Add(1) == 1 {
			return testStatusWrongThread
		}
		return testStatusOK
	}

	if status := state.Close(destroy); status != testStatusWrongThread {
		t.Fatalf("first Close status = %d, want wrong-thread", status)
	}
	if handle, live := state.Handle(); !live || handle != testHandle {
		t.Fatalf("Handle() = %#x, %v; want the live handle", handle, live)
	}
	if status := state.Close(destroy); status != testStatusOK {
		t.Fatalf("second Close status = %d, want OK", status)
	}
	if got := calls.Load(); got != 2 {
		t.Fatalf("destroy calls = %d, want 2", got)
	}
}

func TestStateConcurrentCloseDestroysOnce(t *testing.T) {
	state, err := New(testHandle, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	const goroutines = 8
	var calls atomic.Int32
	var wg sync.WaitGroup
	start := make(chan struct{})
	for i := 0; i < goroutines; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			<-start
			if status := state.Close(func(testNativeHandle) int32 {
				calls.Add(1)
				return testStatusOK
			}); status != testStatusOK {
				t.Errorf("Close status = %d, want OK", status)
			}
		}()
	}
	close(start)
	wg.Wait()

	if got := calls.Load(); got != 1 {
		t.Fatalf("destroy calls = %d, want 1", got)
	}
}

func TestStateLeakReportDoesNotDestroyHandle(t *testing.T) {
	state, err := New(testHandle, "test_handle")
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
	if handle, live := state.Handle(); !live || handle != testHandle {
		t.Fatalf("Handle() after leak report = %#x, %v; want the live handle", handle, live)
	}
}

func TestStateLeakReportIgnoresClosedHandle(t *testing.T) {
	state, err := New(testHandle, "test_handle")
	if err != nil {
		t.Fatal(err)
	}
	if status := state.Close(func(testNativeHandle) int32 { return testStatusOK }); status != testStatusOK {
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
