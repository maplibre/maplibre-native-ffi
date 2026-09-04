package callback

import (
	"errors"
	"sync/atomic"
	"testing"
	"time"
)

const (
	testStatusOK              int32  = 0
	testStatusInvalidArgument int32  = -1
	testStatusInvalidState    int32  = -2
	testStatusNativeError     int32  = -5
	testResourceKindStyle     uint32 = 1
	testResourceKindTile      uint32 = 3
	testProviderPassThrough   uint32 = 0
	testProviderUnknown       uint32 = ^uint32(0)
)

func TestResourceTransformStateCopiesReplacementURL(t *testing.T) {
	state := newResourceTransformState(func(kind uint32, url string) (string, bool) {
		if kind != testResourceKindStyle {
			t.Fatalf("kind = %d, want style", kind)
		}
		if url != "https://example.com/style.json" {
			t.Fatalf("url = %q", url)
		}
		return url + "?token=go", true
	})
	defer state.Release()

	replacement, replaced, status := invokeResourceTransformForTest(state, testResourceKindStyle, "https://example.com/style.json")
	if status != testStatusOK || !replaced || replacement != "https://example.com/style.json?token=go" {
		t.Fatalf("invoke = %q, %v, %v", replacement, replaced, status)
	}
}

func TestResourceTransformTrampolineRequiresNativeResponseContext(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "https://example.com/replacement", true
	})
	defer state.Release()

	replacement, replaced, status := invokeResourceTransformTrampolineReplacementForTest(state, testResourceKindStyle, "https://example.com/style.json")
	if status != testStatusInvalidState || replaced || replacement != "" {
		t.Fatalf("invoke = %q, %v, %v", replacement, replaced, status)
	}
}

func TestResourceTransformStateNoReplacement(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "", false
	})
	defer state.Release()

	replacement, replaced, status := invokeResourceTransformForTest(state, testResourceKindTile, "https://example.com/tile.pbf")
	if status != testStatusOK || replaced || replacement != "" {
		t.Fatalf("invoke = %q, %v, %v", replacement, replaced, status)
	}
}

func TestResourceTransformStateRejectsEmbeddedNULReplacement(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "https://example.com/\x00bad", true
	})
	defer state.Release()

	_, _, status := invokeResourceTransformForTest(state, testResourceKindStyle, "https://example.com/style.json")
	if status != testStatusInvalidArgument {
		t.Fatalf("status = %v, want StatusInvalidArgument", status)
	}
}

func TestResourceTransformTrampolineRecoversPanic(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		panic("boom")
	})
	defer state.Release()

	if status := invokeResourceTransformTrampolineForTest(state, testResourceKindStyle, "https://example.com/style.json"); status != testStatusNativeError {
		t.Fatalf("status = %v, want StatusNativeError", status)
	}
}

func TestResourceProviderTrampolineRecoversPanic(t *testing.T) {
	state := newResourceProviderState(func(ResourceRequest, *ResourceRequestHandle) uint32 {
		panic("boom")
	})
	defer state.Release()

	if decision := invokeResourceProviderTrampolineForTest(state); decision != testProviderUnknown {
		t.Fatalf("decision = %v, want ResourceProviderDecisionUnknown", decision)
	}
}

func TestResourceProviderTrampolinePreservesUnknownDecision(t *testing.T) {
	state := newResourceProviderState(func(ResourceRequest, *ResourceRequestHandle) uint32 {
		return testProviderUnknown
	})
	defer state.Release()

	if decision := invokeResourceProviderTrampolineForTest(state); decision != testProviderUnknown {
		t.Fatalf("decision = %v, want ResourceProviderDecisionUnknown", decision)
	}
}

func TestResourceRequestHandleCompleteIsTerminalAfterNativeSuccess(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	handle.decisionFinalized = true
	handle.providerOwned = true
	var completes int
	var releases int
	restore := setResourceRequestHooksForTest(func() int32 {
		completes++
		return testStatusOK
	}, func() {
		releases++
	})
	defer restore()

	if status := handle.Complete(ResourceResponse{Status: 0}); status != testStatusOK {
		t.Fatalf("first Complete status = %v, want OK", status)
	}
	if status, err := handle.CompleteChecked(ResourceResponse{Status: 1}, nil); !errors.Is(err, ErrResourceRequestCompleted) || status != 0 {
		t.Fatalf("second CompleteChecked = (%v, %v), want already completed", status, err)
	}
	handle.Close()
	if completes != 1 {
		t.Fatalf("native completes = %d, want 1", completes)
	}
	if releases != 1 {
		t.Fatalf("native releases = %d, want 1", releases)
	}
	if !handle.Completed() {
		t.Fatalf("Completed() = false, want true")
	}
}

func TestResourceRequestHandleCompleteRejectsBeforeValidationAfterSuccess(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)

	var completes int
	restore := setResourceRequestHooksForTest(func() int32 {
		completes++
		return testStatusOK
	}, nil)
	defer restore()

	status, err := handle.CompleteChecked(ResourceResponse{Status: 0}, nil)
	if err != nil || status != testStatusOK {
		t.Fatalf("first CompleteChecked = (%v, %v), want OK nil", status, err)
	}
	validationErr := errors.New("invalid second response")
	status, err = handle.CompleteChecked(ResourceResponse{Status: 0}, func() error {
		return validationErr
	})
	if !errors.Is(err, ErrResourceRequestCompleted) || status != 0 {
		t.Fatalf("second CompleteChecked = (%v, %v), want already completed", status, err)
	}
	if completes != 1 {
		t.Fatalf("native completes = %d, want 1", completes)
	}
}

func TestResourceRequestHandleCompleteNativeFailureConsumesCompletion(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	handle.decisionFinalized = true
	handle.providerOwned = true
	var completes int
	var releases int
	restore := setResourceRequestHooksForTest(func() int32 {
		completes++
		return testStatusNativeError
	}, func() {
		releases++
	})
	defer restore()

	if status := handle.Complete(ResourceResponse{Status: 0}); status != testStatusNativeError {
		t.Fatalf("first Complete status = %v, want NativeError", status)
	}
	if !handle.Completed() {
		t.Fatalf("Completed() after native failure = false, want true")
	}
	if status, err := handle.CompleteChecked(ResourceResponse{Status: 0}, nil); !errors.Is(err, ErrResourceRequestCompleted) || status != 0 {
		t.Fatalf("second CompleteChecked = (%v, %v), want already completed", status, err)
	}
	if completes != 1 {
		t.Fatalf("native completes = %d, want 1", completes)
	}
	if releases != 1 {
		t.Fatalf("native releases = %d, want 1", releases)
	}
}

func TestResourceRequestHandleCancelledAfterCloseFailsBeforeNative(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)

	handle.Close()
	status, cancelled, err := handle.CancelledChecked()
	if !errors.Is(err, ErrResourceRequestClosed) || status != 0 || cancelled {
		t.Fatalf("CancelledChecked after Close = (%v, %v, %v), want closed", status, cancelled, err)
	}
}

// A call that arrives after the provider state is released must stop at the
// trampoline rather than reach freed Go callback state.
func TestResourceProviderReleasedStateStopsReachingCallback(t *testing.T) {
	var calls int
	state := newResourceProviderState(func(ResourceRequest, *ResourceRequestHandle) uint32 {
		calls++
		return testProviderPassThrough
	})

	if decision := invokeResourceProviderTrampolineForTest(state); decision != testProviderPassThrough {
		t.Fatalf("decision = %v, want pass through", decision)
	}
	state.Release()
	state.Release()

	if decision := invokeResourceProviderTrampolineForTest(state); decision != testProviderUnknown {
		t.Fatalf("decision after release = %v, want ResourceProviderDecisionUnknown", decision)
	}
	if calls != 1 {
		t.Fatalf("callback calls = %d, want 1", calls)
	}
}

func TestResourceTransformStateReleaseIsIdempotent(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "https://example.com/replacement", true
	})
	if _, _, status := invokeResourceTransformForTest(state, testResourceKindStyle, "https://example.com/style.json"); status != testStatusOK {
		t.Fatalf("invoke status = %v", status)
	}
	state.Release()
	state.Release()
}

// BND-198: a released request rejects a cancel callback registration before the
// call reaches native code.
func TestResourceRequestSetCancelCallbackAfterCloseFailsBeforeNative(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)

	var sets int
	restore := setResourceRequestCancelHookForTest(func(*resourceCancelState) int32 {
		sets++
		return testStatusOK
	})
	defer restore()

	handle.Close()
	status, err := handle.SetCancelCallbackChecked(func() {})
	if !errors.Is(err, ErrResourceRequestClosed) || status != 0 {
		t.Fatalf("SetCancelCallbackChecked after Close = (%v, %v), want closed", status, err)
	}
	if sets != 0 {
		t.Fatalf("native registrations = %d, want 0", sets)
	}
}

// BND-198: a request cancelled before registration runs the callback while the
// registration is still in flight, and that callback can close the same request
// without deadlocking against the registration.
func TestResourceRequestCancelCallbackRunsDuringRegistrationAndMayClose(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	live := ResourceCancelStateLiveCountForTest()

	restore := setResourceRequestCancelHookForTest(func(state *resourceCancelState) int32 {
		invokeResourceRequestCancelTrampolineForTest(state)
		return testStatusOK
	})
	defer restore()

	var calls int
	status, err := handle.SetCancelCallbackChecked(func() {
		calls++
		handle.Close()
	})
	if err != nil || status != testStatusOK {
		t.Fatalf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	if calls != 1 {
		t.Fatalf("cancel callback calls = %d, want 1", calls)
	}
	if got := ResourceCancelStateLiveCountForTest(); got != live {
		t.Fatalf("live cancel states = %d, want %d", got, live)
	}
}

// BND-198: a panic inside the cancel callback stops at the trampoline rather
// than unwinding into C.
func TestResourceRequestCancelTrampolineRecoversPanic(t *testing.T) {
	defer func() {
		if recovered := recover(); recovered != nil {
			t.Fatalf("cancel trampoline propagated panic: %v", recovered)
		}
	}()

	restore := setResourceRequestCancelHookForTest(func(state *resourceCancelState) int32 {
		invokeResourceRequestCancelTrampolineForTest(state)
		return testStatusOK
	})
	defer restore()

	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	if status, err := handle.SetCancelCallbackChecked(func() {
		panic("boom")
	}); err != nil || status != testStatusOK {
		t.Fatalf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	handle.Close()
}

// BND-198: a replacement keeps the replaced registration alive until the native
// call that replaces it returns, because native code may be invoking it.
func TestResourceRequestCancelCallbackReplacementOutlivesNativeCall(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	live := ResourceCancelStateLiveCountForTest()

	var previous *resourceCancelState
	var replacedCalls int
	restore := setResourceRequestCancelHookForTest(func(state *resourceCancelState) int32 {
		if previous != nil {
			// Stands in for a native cancellation that is already running the
			// replaced registration while the replacement is installed.
			invokeResourceRequestCancelTrampolineForTest(previous)
		}
		previous = state
		return testStatusOK
	})
	defer restore()

	if status, err := handle.SetCancelCallbackChecked(func() { replacedCalls++ }); err != nil || status != testStatusOK {
		t.Fatalf("first SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	if status, err := handle.SetCancelCallbackChecked(func() {}); err != nil || status != testStatusOK {
		t.Fatalf("second SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	if replacedCalls != 1 {
		t.Fatalf("replaced callback calls = %d, want 1", replacedCalls)
	}
	if got := ResourceCancelStateLiveCountForTest(); got != live+1 {
		t.Fatalf("live cancel states after replacement = %d, want %d", got, live+1)
	}

	handle.Close()
	if got := ResourceCancelStateLiveCountForTest(); got != live {
		t.Fatalf("live cancel states after Close = %d, want %d", got, live)
	}
}

// BND-197, BND-198: a close that races a running cancel callback releases the
// request without deadlocking, and the callback's use of the same handle
// reports the closed error rather than blocking on the release.
func TestResourceRequestCloseDuringCancelCallbackDoesNotDeadlock(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	handle.decisionFinalized = true
	handle.providerOwned = true

	var registered *resourceCancelState
	restoreCancel := setResourceRequestCancelHookForTest(func(state *resourceCancelState) int32 {
		registered = state
		return testStatusOK
	})
	defer restoreCancel()

	releasing := make(chan struct{})
	callbackDone := make(chan struct{})
	restoreRequest := setResourceRequestHooksForTest(func() int32 {
		return testStatusInvalidState
	}, func() {
		// Stands in for the C API release, which waits for a cancel callback
		// running on another thread.
		close(releasing)
		<-callbackDone
	})
	defer restoreRequest()

	var completeErr error
	if status, err := handle.SetCancelCallbackChecked(func() {
		<-releasing
		_, completeErr = handle.CompleteChecked(ResourceResponse{Status: 0}, nil)
		handle.Close()
		close(callbackDone)
	}); err != nil || status != testStatusOK {
		t.Fatalf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}

	go invokeResourceRequestCancelTrampolineForTest(registered)

	closed := make(chan struct{})
	go func() {
		defer close(closed)
		handle.Close()
	}()
	select {
	case <-closed:
	case <-time.After(10 * time.Second):
		t.Fatalf("Close deadlocked against the running cancel callback")
	}
	<-callbackDone

	if !errors.Is(completeErr, ErrResourceRequestClosed) {
		t.Fatalf("CompleteChecked from the cancel callback = %v, want closed", completeErr)
	}
}

// BND-197: a close that races a cancel callback registration waits for the
// registration to leave the C API before it releases the request.
func TestResourceRequestCloseDrainsInFlightCancelRegistration(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	handle.decisionFinalized = true
	handle.providerOwned = true

	registering := make(chan struct{})
	finishRegistration := make(chan struct{})
	restoreCancel := setResourceRequestCancelHookForTest(func(*resourceCancelState) int32 {
		close(registering)
		<-finishRegistration
		return testStatusOK
	})
	defer restoreCancel()

	var released atomic.Bool
	restoreRequest := setResourceRequestHooksForTest(nil, func() {
		released.Store(true)
	})
	defer restoreRequest()

	registrationDone := make(chan struct{})
	go func() {
		defer close(registrationDone)
		if status, err := handle.SetCancelCallbackChecked(func() {}); err != nil || status != testStatusOK {
			t.Errorf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
		}
	}()
	<-registering

	closed := make(chan struct{})
	go func() {
		defer close(closed)
		handle.Close()
	}()
	// A close that fails to drain the registration would release here.
	time.Sleep(50 * time.Millisecond)
	if released.Load() {
		t.Fatalf("release ran while a registration was still in the C API")
	}

	close(finishRegistration)
	<-registrationDone
	<-closed
	if !released.Load() {
		t.Fatalf("release did not run after the registration returned")
	}
}
