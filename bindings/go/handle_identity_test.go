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
	defer closeRuntimeForTest(runtime)

	first, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	released, err := first.ptr()
	if err != nil {
		t.Fatalf("ptr(): %v", err)
	}
	if err := first.Close(); err != nil {
		t.Fatalf("first.Close(): %v", err)
	}

	// The released slot is the one the next map takes, so the replayed id
	// names a retired generation of a slot that is live again.
	second, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("second NewMap(): %v", err)
	}
	defer second.Close()

	err = mapSizeByIDForTest(released)
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("replaying a released id: err = %v, want invalid argument", err)
	}
	if !strings.Contains(err.Error(), "stale") {
		t.Fatalf("diagnostic = %v, want it to name the id stale", err)
	}

	if _, _, _, err := second.Size(); err != nil {
		t.Fatalf("second.Size(): %v", err)
	}
}
