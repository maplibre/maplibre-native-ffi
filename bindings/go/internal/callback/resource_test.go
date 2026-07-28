package callback

import (
	"errors"
	"testing"
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

// Clearing or replacing a provider retires its state once native code can no
// longer invoke it, so a later inbound call must stop at the trampoline instead
// of reaching freed Go callback state.
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
