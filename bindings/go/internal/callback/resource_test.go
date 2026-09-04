package callback

import (
	"errors"
	stdruntime "runtime"
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

// BND-198: a closed request rejects a cancel callback before the call reaches
// native code, and a request with a registration rejects a second one the same
// way, leaving the first registration in place.
func TestResourceRequestSetCancelCallbackRejectsClosedAndSecondRegistration(t *testing.T) {
	var sets int
	restore := setResourceRequestCancelHookForTest(func(uint64) (int32, bool) {
		sets++
		return testStatusOK, false
	})
	defer restore()

	closed := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(closed)
	closed.Close()
	if status, err := closed.SetCancelCallbackChecked(func() {}); !errors.Is(err, ErrResourceRequestClosed) || status != 0 {
		t.Fatalf("SetCancelCallbackChecked after Close = (%v, %v), want closed", status, err)
	}
	if sets != 0 {
		t.Fatalf("native registrations after closed handle = %d, want 0", sets)
	}

	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	var firstCalls, secondCalls int
	if status, err := handle.SetCancelCallbackChecked(func() { firstCalls++ }); err != nil || status != testStatusOK {
		t.Fatalf("first SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	if status, err := handle.SetCancelCallbackChecked(func() { secondCalls++ }); !errors.Is(err, ErrResourceRequestCancelCallbackRegistered) || status != 0 {
		t.Fatalf("second SetCancelCallbackChecked = (%v, %v), want already registered", status, err)
	}
	if sets != 1 {
		t.Fatalf("native registrations = %d, want 1", sets)
	}
	invokeResourceRequestCancelTrampolineForTest(handle.cancelToken)
	if firstCalls != 1 || secondCalls != 0 {
		t.Fatalf("callback calls = (%d, %d), want the first registration to run once", firstCalls, secondCalls)
	}
	handle.Close()
}

// BND-198: when native code reports that the request was already cancelled, the
// callback runs before registration returns, and it may close the same request
// from inside itself.
func TestResourceRequestCancelCallbackRunsBeforeRegistrationWhenAlreadyCancelled(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	handle.decisionFinalized = true
	handle.providerOwned = true

	var registeredToken uint64
	restoreCancel := setResourceRequestCancelHookForTest(func(token uint64) (int32, bool) {
		registeredToken = token
		return testStatusOK, true
	})
	defer restoreCancel()
	var releases int
	restoreRequest := setResourceRequestHooksForTest(nil, func() { releases++ })
	defer restoreRequest()

	var calls int
	status, err := handle.SetCancelCallbackChecked(func() {
		calls++
		handle.Close()
	})
	if err != nil || status != testStatusOK {
		t.Fatalf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	if calls != 1 {
		t.Fatalf("cancel callback calls = %d, want 1 before registration returned", calls)
	}
	if releases != 1 {
		t.Fatalf("releases = %d, want 1 from the callback's Close", releases)
	}
	if lookupCancelToken(registeredToken) != nil {
		t.Fatalf("registry still resolves the token native code never stored")
	}
	if handle.cancelToken != 0 || handle.cancelCallback != nil {
		t.Fatalf("handle kept a registration native code never stored")
	}
}

// BND-198: the trampoline runs the callback once, contains its panic, and
// ignores a token the request already released.
func TestResourceRequestCancelTrampolineRunsOnceAndRecoversPanic(t *testing.T) {
	defer func() {
		if recovered := recover(); recovered != nil {
			t.Fatalf("cancel trampoline propagated panic: %v", recovered)
		}
	}()
	restore := setResourceRequestCancelHookForTest(func(uint64) (int32, bool) {
		return testStatusOK, false
	})
	defer restore()

	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	handle.decisionFinalized = true
	handle.providerOwned = true
	var calls int
	if status, err := handle.SetCancelCallbackChecked(func() {
		calls++
		panic("boom")
	}); err != nil || status != testStatusOK {
		t.Fatalf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	token := handle.cancelToken

	invokeResourceRequestCancelTrampolineForTest(token)
	invokeResourceRequestCancelTrampolineForTest(token)
	if calls != 1 {
		t.Fatalf("cancel callback calls = %d, want 1", calls)
	}

	handle.Close()
	if lookupCancelToken(token) != nil {
		t.Fatalf("registry still resolves the token after Close")
	}
	invokeResourceRequestCancelTrampolineForTest(token)
	if calls != 1 {
		t.Fatalf("cancel callback calls after Close = %d, want 1", calls)
	}
}

// BND-198: a close that races a running cancel callback releases the request
// with no lock held, so the callback's use of the same handle reports the
// closed error instead of deadlocking against the release that waits for it.
func TestResourceRequestCloseDuringCancelCallbackDoesNotDeadlock(t *testing.T) {
	handle := newResourceRequestHandleForTest()
	defer freeResourceRequestHandleForTest(handle)
	handle.decisionFinalized = true
	handle.providerOwned = true

	restoreCancel := setResourceRequestCancelHookForTest(func(uint64) (int32, bool) {
		return testStatusOK, false
	})
	defer restoreCancel()

	releasing := make(chan struct{})
	callbackDone := make(chan struct{})
	var releases int
	restoreRequest := setResourceRequestHooksForTest(func() int32 {
		return testStatusInvalidState
	}, func() {
		// Stands in for the C API release, which waits for a cancel callback
		// running on another thread.
		releases++
		close(releasing)
		<-callbackDone
	})
	defer restoreRequest()

	callbackStarted := make(chan struct{})
	var completeErr error
	if status, err := handle.SetCancelCallbackChecked(func() {
		close(callbackStarted)
		<-releasing
		_, completeErr = handle.CompleteChecked(ResourceResponse{}, nil)
		handle.Close()
		close(callbackDone)
	}); err != nil || status != testStatusOK {
		t.Fatalf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	go invokeResourceRequestCancelTrampolineForTest(handle.cancelToken)
	<-callbackStarted

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
	if releases != 1 {
		t.Fatalf("releases = %d, want 1", releases)
	}
}

// BND-198: the registry resolves the native token through a weak reference, so
// a registration whose callback captures its own handle does not keep that
// handle alive.
func TestResourceRequestCancelRegistryDoesNotRootHandle(t *testing.T) {
	restore := setResourceRequestCancelHookForTest(func(uint64) (int32, bool) {
		return testStatusOK, false
	})
	defer restore()

	handle := newResourceRequestHandleForTest()
	if status, err := handle.SetCancelCallbackChecked(func() { handle.Close() }); err != nil || status != testStatusOK {
		t.Fatalf("SetCancelCallbackChecked = (%v, %v), want OK nil", status, err)
	}
	token := handle.cancelToken
	if lookupCancelToken(token) != handle {
		t.Fatalf("registry does not resolve the live handle")
	}
	handle = nil

	stdruntime.GC()
	stdruntime.GC()
	if lookupCancelToken(token) != nil {
		t.Fatalf("registry kept the handle reachable after its owner dropped it")
	}
	unregisterCancelToken(token)
}
