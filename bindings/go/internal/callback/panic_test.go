package callback

import (
	"sync/atomic"
	"testing"
	"time"
)

const (
	testLogSeverityInfo uint32 = 1
	testLogEventGeneral uint32 = 0
)

func TestLogCallbackTrampolineRecoversPanic(t *testing.T) {
	if consumed := invokeLogCallbackForTest(func(uint32, uint32, int64, string) bool {
		panic("boom")
	}); consumed != 0 {
		t.Fatalf("consumed = %d, want 0", consumed)
	}
}

func TestCustomGeometryTrampolineRecoversPanic(t *testing.T) {
	defer func() {
		if recovered := recover(); recovered != nil {
			t.Fatalf("custom geometry trampoline propagated panic: %v", recovered)
		}
	}()
	invokeCustomGeometryFetchForTest(func(CanonicalTileID) {
		panic("boom")
	})
}

func TestLogCallbackTrampolineReturnsConsumed(t *testing.T) {
	if consumed := invokeLogCallbackForTest(func(severity uint32, event uint32, code int64, message string) bool {
		if severity != testLogSeverityInfo || event != testLogEventGeneral || code != 0 || message != "test message" {
			t.Fatalf("record = (%d, %d, %d, %q)", severity, event, code, message)
		}
		return true
	}); consumed != 1 {
		t.Fatalf("consumed = %d, want 1", consumed)
	}
}

func TestLogCallbackSetSerializesNativeAndGoState(t *testing.T) {
	oldSet := setNativeLogCallback
	oldClear := clearNativeLogCallback
	t.Cleanup(func() {
		_ = ClearLogCallback()
		setNativeLogCallback = oldSet
		clearNativeLogCallback = oldClear
	})

	entered := make(chan struct{})
	release := make(chan struct{})
	var calls atomic.Int32
	setNativeLogCallback = func() int32 {
		if calls.Add(1) == 1 {
			close(entered)
			<-release
		}
		return 0
	}
	clearNativeLogCallback = func() int32 { return 0 }

	firstDone := make(chan int32, 1)
	secondDone := make(chan int32, 1)
	go func() {
		firstDone <- SetLogCallback(func(uint32, uint32, int64, string) bool { return false })
	}()
	<-entered
	go func() {
		secondDone <- SetLogCallback(func(uint32, uint32, int64, string) bool { return true })
	}()
	select {
	case status := <-secondDone:
		close(release)
		t.Fatalf("second SetLogCallback completed while first native install was in flight: %d", status)
	case <-time.After(25 * time.Millisecond):
	}
	close(release)
	if status := <-firstDone; status != 0 {
		t.Fatalf("first SetLogCallback status = %d, want 0", status)
	}
	if status := <-secondDone; status != 0 {
		t.Fatalf("second SetLogCallback status = %d, want 0", status)
	}
	if calls.Load() != 2 {
		t.Fatalf("native set calls = %d, want 2", calls.Load())
	}
}

func TestCustomGeometryReleaseWaitsForActiveCallback(t *testing.T) {
	entered := make(chan struct{})
	unblock := make(chan struct{})
	released := make(chan struct{})

	state := newCustomGeometrySourceStateForTest(func(CanonicalTileID) {
		close(entered)
		<-unblock
	})

	go invokeCustomGeometryFetchStateForTest(state)
	<-entered

	go func() {
		state.Release()
		close(released)
	}()

	select {
	case <-released:
		t.Fatal("Release returned while callback was active")
	default:
	}

	close(unblock)
	<-released
	state.Release()
}
