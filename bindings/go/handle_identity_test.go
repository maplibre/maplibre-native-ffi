package maplibre

import (
	"errors"
	"strings"
	"testing"
)

// BND-045: a released map id, replayed after a new map exists, is reported
// stale rather than naming the new map.
func TestReleasedMapIDReplayedAfterANewMapReportsItStale(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	first, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	released, err := first.ptr()
	if err != nil {
		t.Fatalf("ptr(): %v", err)
	}
	if err := closeMapForTest(first); err != nil {
		t.Fatalf("first.Close(): %v", err)
	}

	// The released slot is the one the next map takes, so the replayed id
	// names a retired generation of a slot that is live again.
	second, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("second NewMap(): %v", err)
	}
	defer func() { _ = closeMapForTest(second) }()

	err = mapSnapshotByIDForTest(released)
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("replaying a released id: err = %v, want invalid argument", err)
	}
	if !strings.Contains(err.Error(), "stale") {
		t.Fatalf("diagnostic = %v, want it to name the id stale", err)
	}

	if _, err := second.Snapshot(); err != nil {
		t.Fatalf("second.Snapshot(): %v", err)
	}
}

// BND-047: a handle of one kind passed to another kind's entry point is
// rejected, and the diagnostic names both kinds.
func TestHandleOfAnotherKindIsRejectedByKind(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	raw, err := m.ptr()
	if err != nil {
		t.Fatalf("ptr(): %v", err)
	}
	err = runtimeEventMaskByIDForTest(nativeRuntime(raw))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("passing a map id to a runtime entry point: err = %v, want invalid argument", err)
	}
	diagnostic := err.Error()
	if !strings.Contains(diagnostic, "mln_map") || !strings.Contains(diagnostic, "mln_runtime") {
		t.Fatalf("diagnostic = %q, want it to name both handle kinds", diagnostic)
	}
}
