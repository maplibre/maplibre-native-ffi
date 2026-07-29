package maplibre

import (
	"errors"
	"strings"
	"testing"
)

// BND-045: a released map id, replayed after a new map exists, is reported
// stale rather than naming the new map.
func TestReleasedMapIDReplayedAfterANewMapReportsItStale(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer runtime.Close()

	first, err := runtime.NewMap()
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	released, release, err := first.nativeMapIDForTest()
	if err != nil {
		t.Fatalf("nativeMapIDForTest(): %v", err)
	}
	release()
	if err := first.Close(); err != nil {
		t.Fatalf("first.Close(): %v", err)
	}

	// The released slot is the one the next map takes, so this is the case a
	// pointer handle could not tell apart from a live map.
	second, err := runtime.NewMap()
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

	// The live map is unaffected by the replay.
	if _, _, _, err := second.Size(); err != nil {
		t.Fatalf("second.Size(): %v", err)
	}
}

// BND-047: a map id passed to a runtime operation is rejected on its kind.
func TestMapIDPassedToARuntimeOperationReportsInvalidArgument(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer runtime.Close()

	m, err := runtime.NewMap()
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer m.Close()

	mapID, release, err := m.nativeMapIDForTest()
	if err != nil {
		t.Fatalf("nativeMapIDForTest(): %v", err)
	}
	defer release()

	err = pumpRuntimeWithMapIDForTest(mapID)
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("wrong-kind id: err = %v, want invalid argument", err)
	}
	if message := err.Error(); !strings.Contains(message, "map") ||
		!strings.Contains(message, "runtime") {
		t.Fatalf("diagnostic = %q, want it to name both kinds", message)
	}
}

// BND-049: a live id called from another thread reports wrong-thread rather
// than stale.
func TestLiveMapIDCalledFromAnotherThreadReportsWrongThread(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer runtime.Close()

	m, err := runtime.NewMap()
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer m.Close()

	live, release, err := m.nativeMapIDForTest()
	if err != nil {
		t.Fatalf("nativeMapIDForTest(): %v", err)
	}
	defer release()

	done := make(chan error, 1)
	go func() { done <- mapSizeByIDForTest(live) }()
	got := <-done

	// The id is live, so the owner-thread rule decides rather than identity.
	if !errors.Is(got, ErrWrongThread) {
		t.Fatalf("cross-thread call: err = %v, want wrong thread", got)
	}
	if strings.Contains(got.Error(), "stale") {
		t.Fatalf("diagnostic = %v, want a wrong-thread message", got)
	}
}
