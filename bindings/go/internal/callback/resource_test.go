package callback

import (
	"runtime/cgo"
	"testing"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

func TestResourceTransformStateCopiesReplacementURL(t *testing.T) {
	state := newResourceTransformState(func(kind uint32, url string) (string, bool) {
		if kind != capi.ResourceKindStyle {
			t.Fatalf("kind = %d, want style", kind)
		}
		if url != "https://example.com/style.json" {
			t.Fatalf("url = %q", url)
		}
		return url + "?token=go", true
	})
	defer state.Release()

	replacement, replaced, status := invokeResourceTransformForTest(state, capi.ResourceKindStyle, "https://example.com/style.json")
	if status != capi.StatusOK || !replaced || replacement != "https://example.com/style.json?token=go" {
		t.Fatalf("invoke = %q, %v, %v", replacement, replaced, status)
	}
}

func TestResourceTransformTrampolineCopiesReplacementToThreadStorage(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "https://example.com/replacement", true
	})
	defer state.Release()

	replacement, replaced, status := invokeResourceTransformTrampolineReplacementForTest(state, capi.ResourceKindStyle, "https://example.com/style.json")
	if status != capi.StatusOK || !replaced || replacement != "https://example.com/replacement" {
		t.Fatalf("invoke = %q, %v, %v", replacement, replaced, status)
	}
}

func TestResourceTransformStateNoReplacement(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "", false
	})
	defer state.Release()

	replacement, replaced, status := invokeResourceTransformForTest(state, capi.ResourceKindTile, "https://example.com/tile.pbf")
	if status != capi.StatusOK || replaced || replacement != "" {
		t.Fatalf("invoke = %q, %v, %v", replacement, replaced, status)
	}
}

func TestResourceTransformStateRejectsEmbeddedNULReplacement(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "https://example.com/\x00bad", true
	})
	defer state.Release()

	_, _, status := invokeResourceTransformForTest(state, capi.ResourceKindStyle, "https://example.com/style.json")
	if status != capi.StatusInvalidArgument {
		t.Fatalf("status = %v, want StatusInvalidArgument", status)
	}
}

func TestResourceTransformTrampolineRecoversPanic(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		panic("boom")
	})
	defer state.Release()

	if status := invokeResourceTransformTrampolineForTest(state, capi.ResourceKindStyle, "https://example.com/style.json"); status != capi.StatusNativeError {
		t.Fatalf("status = %v, want StatusNativeError", status)
	}
}

func TestResourceProviderTrampolineRecoversPanic(t *testing.T) {
	state := &ResourceProviderState{callback: func(ResourceRequest, *ResourceRequestHandle) uint32 {
		panic("boom")
	}}
	state.handle = cgo.NewHandle(state)
	defer state.Release()

	if decision := invokeResourceProviderTrampolineForTest(state); decision != capi.ResourceProviderDecisionUnknown {
		t.Fatalf("decision = %v, want ResourceProviderDecisionUnknown", decision)
	}
}

func TestResourceTransformStateReleaseIsIdempotent(t *testing.T) {
	state := newResourceTransformState(func(uint32, string) (string, bool) {
		return "https://example.com/replacement", true
	})
	if _, _, status := invokeResourceTransformForTest(state, capi.ResourceKindStyle, "https://example.com/style.json"); status != capi.StatusOK {
		t.Fatalf("invoke status = %v", status)
	}
	state.Release()
	state.Release()
}
