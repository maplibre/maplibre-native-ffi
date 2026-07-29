package handle

import (
	"bytes"
	"log"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"
)

const (
	testStatusOK          int32 = 0
	testStatusWrongThread int32 = -3
)

// A synthetic handle type for close-once tests. It reaches only the fake
// destroy functions below, never the C API, and the safe public API cannot
// build one.
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

func TestStateCloseWaitsForActiveBorrowBeforeDestroy(t *testing.T) {
	state, err := New(testHandle, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	borrow, live := state.Borrow()
	if !live {
		t.Fatal("Borrow() failed for live state")
	}
	if handle := borrow.Handle(); handle != testHandle {
		t.Fatalf("borrow Handle() = %#x, want %#x", handle, testHandle)
	}

	destroyCalled := make(chan struct{})
	closeDone := make(chan int32)
	go func() {
		closeDone <- state.Close(func(handle testNativeHandle) int32 {
			if handle != testHandle {
				t.Errorf("destroy handle = %#x, want %#x", handle, testHandle)
			}
			close(destroyCalled)
			return testStatusOK
		})
	}()

	select {
	case <-destroyCalled:
		t.Fatal("destroy ran before active borrow was released")
	default:
	}
	deadline := time.After(2 * time.Second)
	for {
		if borrow, live := state.Borrow(); live {
			borrow.Release()
			select {
			case <-destroyCalled:
				t.Fatal("destroy ran before active borrow was released")
			case <-deadline:
				t.Fatal("Close did not enter releasing state")
			case <-time.After(100 * time.Microsecond):
				continue
			}
		}
		break
	}

	borrow.Release()
	if status := <-closeDone; status != testStatusOK {
		t.Fatalf("Close status = %d, want OK", status)
	}
	select {
	case <-destroyCalled:
	default:
		t.Fatal("destroy did not run after borrow release")
	}
}

func TestStateFailedCloseAllowsBorrowRetry(t *testing.T) {
	state, err := New(testHandle, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	if status := state.Close(func(testNativeHandle) int32 { return testStatusWrongThread }); status != testStatusWrongThread {
		t.Fatalf("Close status = %d, want wrong-thread", status)
	}
	borrow, live := state.Borrow()
	if !live {
		t.Fatal("Borrow() failed after failed close")
	}
	if handle := borrow.Handle(); handle != testHandle {
		t.Fatalf("borrow Handle() = %#x, want %#x", handle, testHandle)
	}
	borrow.Release()
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

func TestStateCloseFailsWithLiveChildrenAndRetries(t *testing.T) {
	state, err := New(testHandle, "test_handle")
	if err != nil {
		t.Fatal(err)
	}

	child := state.AddChild()
	if status := state.Close(func(testNativeHandle) int32 {
		t.Fatal("destroy called while child is live")
		return testStatusOK
	}, testStatusWrongThread); status != testStatusWrongThread {
		t.Fatalf("Close status = %d, want live-child status", status)
	}
	if borrow, live := state.Borrow(); !live {
		t.Fatal("Borrow() failed after live-child close failure")
	} else {
		borrow.Release()
	}

	child.Release()
	if status := state.Close(func(testNativeHandle) int32 { return testStatusOK }, testStatusWrongThread); status != testStatusOK {
		t.Fatalf("Close after child release status = %d, want OK", status)
	}
}

func TestStateKeepsParentsReachable(t *testing.T) {
	const parentHandle testNativeHandle = 0x0100_0000_0000_0007
	parent := parentHandle
	native := testHandle
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
