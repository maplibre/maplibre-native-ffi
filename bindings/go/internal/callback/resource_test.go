package callback

import (
	"runtime/cgo"
	"testing"
)

const (
	testStatusOK              int32  = 0
	testStatusInvalidArgument int32  = -1
	testStatusInvalidState    int32  = -2
	testStatusNativeError     int32  = -5
	testResourceKindStyle     uint32 = 1
	testResourceKindTile      uint32 = 3
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
	state := &ResourceProviderState{callback: func(ResourceRequest, *ResourceRequestHandle) uint32 {
		panic("boom")
	}}
	state.handle = cgo.NewHandle(state)
	defer state.Release()

	if decision := invokeResourceProviderTrampolineForTest(state); decision != testProviderUnknown {
		t.Fatalf("decision = %v, want ResourceProviderDecisionUnknown", decision)
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
