package handle

import (
	"bytes"
	"errors"
	"log"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
)

// A synthetic handle for close-once tests. It reaches only the fake destroy
// functions below, never the C API.
type testNativeHandle uint64

const testHandle testNativeHandle = 0x0200_0000_0000_002a

var errWrongThread = errors.New("wrong thread")

// captureLog redirects the standard logger for one test and returns the buffer
// it writes into.
func captureLog(t *testing.T) *bytes.Buffer {
	t.Helper()
	var buf bytes.Buffer
	oldWriter := log.Writer()
	oldFlags := log.Flags()
	log.SetOutput(&buf)
	log.SetFlags(0)
	t.Cleanup(func() {
		log.SetOutput(oldWriter)
		log.SetFlags(oldFlags)
	})
	return &buf
}

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
	destroy := func(handle testNativeHandle) error {
		if handle != testHandle {
			t.Fatalf("destroy handle = %#x, want %#x", handle, testHandle)
		}
		calls.Add(1)
		return nil
	}

	if err := state.Close(destroy); err != nil {
		t.Fatalf("first Close: %v", err)
	}
	if err := state.Close(destroy); err != nil {
		t.Fatalf("second Close: %v", err)
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
	destroy := func(testNativeHandle) error {
		if calls.Add(1) == 1 {
			return errWrongThread
		}
		return nil
	}

	if err := state.Close(destroy); !errors.Is(err, errWrongThread) {
		t.Fatalf("first Close error = %v, want wrong thread", err)
	}
	if handle, live := state.Handle(); !live || handle != testHandle {
		t.Fatalf("Handle() = %#x, %v; want the live handle", handle, live)
	}
	if err := state.Close(destroy); err != nil {
		t.Fatalf("second Close: %v", err)
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
			if err := state.Close(func(testNativeHandle) error {
				calls.Add(1)
				return nil
			}); err != nil {
				t.Errorf("Close: %v", err)
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

	buf := captureLog(t)
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
	if err := state.Close(func(testNativeHandle) error { return nil }); err != nil {
		t.Fatalf("Close: %v", err)
	}

	buf := captureLog(t)
	state.reportLeakIfLive()
	if got := buf.String(); got != "" {
		t.Fatalf("leak report after close = %q, want empty", got)
	}
}
