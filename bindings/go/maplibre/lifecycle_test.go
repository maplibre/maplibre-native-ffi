package maplibre

import (
	"errors"
	"fmt"
	stdruntime "runtime"
	"testing"
	"weak"
)

func TestClosingTwiceIsSafe(t *testing.T) {
	f := newFixture(t)
	await(t, submitted(f.m.Close()))
	if _, err := f.m.Close(); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("second map close: %v, want native invalid state", err)
	}
	_, err := f.m.SetStyleJson(emptyStyle)
	var closed *Error
	if !errors.As(err, &closed) || closed.Status != StatusInvalidState || closed.Diagnostic == "" {
		t.Fatalf("SetStyleJson after close = %v, want native invalid state", err)
	}
	if err := f.m.Dispose(); err != nil {
		t.Fatalf("Dispose after close: %v, want a no-op", err)
	}
}

type handleMembers interface {
	Id() uint64
	IsClosed() bool
	String() string
}

// checkHandleMembers checks that handle has an id and reports closed in
// IsClosed and in String.
func checkHandleMembers(t *testing.T, handle handleMembers, typeName string, closed bool) {
	t.Helper()
	if handle.Id() == 0 {
		t.Errorf("%s.Id() = 0, want the native handle value", typeName)
	}
	if got := handle.IsClosed(); got != closed {
		t.Errorf("%s.IsClosed() = %t, want %t", typeName, got, closed)
	}
	want := fmt.Sprintf("%s(id=%#x, closed=%t)", typeName, handle.Id(), closed)
	if got := handle.String(); got != want {
		t.Errorf("%s.String() = %q, want %q", typeName, got, want)
	}
}

// Dispose and a void release each close their handle once: a second call does
// nothing, and the id and String outlive the handle.
func TestDisposeAndReleaseCloseOnce(t *testing.T) {
	f := newFixture(t)
	checkHandleMembers(t, f.m, "MapHandle", false)
	checkHandleMembers(t, f.runtime, "RuntimeHandle", false)
	for range 2 {
		if err := f.m.Dispose(); err != nil {
			t.Fatalf("MapHandle.Dispose: %v", err)
		}
		if err := f.runtime.Dispose(); err != nil {
			t.Fatalf("RuntimeHandle.Dispose: %v", err)
		}
	}
	checkHandleMembers(t, f.m, "MapHandle", true)
	checkHandleMembers(t, f.runtime, "RuntimeHandle", true)

	data, err := GeojsonSourceDataCreate(`{"type":"FeatureCollection","features":[]}`, nil)
	if err != nil {
		t.Fatal(err)
	}
	checkHandleMembers(t, data, "GeojsonSourceDataHandle", false)
	for range 2 {
		if err := data.Close(); err != nil {
			t.Fatalf("GeojsonSourceDataHandle.Close: %v", err)
		}
	}
	checkHandleMembers(t, data, "GeojsonSourceDataHandle", true)
}

// A runtime refuses to close while its map is live. The refusal leaves the
// runtime open and usable, and a close after the map's succeeds.
func TestRefusedCloseLeavesTheHandleUsable(t *testing.T) {
	f := newFixture(t)
	teardown, err := f.runtime.Close()
	var native *Error
	if teardown != nil || !errors.As(err, &native) || !errors.Is(err, StatusInvalidState) {
		t.Fatalf("runtime close with a live map = (%v, %v), want nil and StatusInvalidState", teardown, err)
	}
	if native.Diagnostic == "" {
		t.Fatal("the refusal carried no diagnostic")
	}
	await(t, submitted(f.runtime.ClearResourceTransform()))
	await(t, submitted(f.m.Close()))
	await(t, submitted(f.runtime.Close()))
}

// A handle's id is the source that its events carry, and it stays readable
// after close, so a host can route events that it drains later.
func TestHandleIdNamesItsEventsAndOutlivesClose(t *testing.T) {
	f := newFixture(t)
	await(t, submitted(f.m.SetStyleJson(emptyStyle)))
	event := f.awaitEvent(t, "the style load", isStyleLoaded)
	id := f.m.Id()
	if event.Source != id {
		t.Fatalf("event source = %#x, want the map id %#x", event.Source, id)
	}
	await(t, submitted(f.m.Close()))
	if got := f.m.Id(); got != id {
		t.Fatalf("Id() after close = %#x, want %#x", got, id)
	}
}

// A nil pointer or a zero value of a handle type fails with the binding's
// error instead of reaching native, whether it is the receiver or an input,
// and native reports a closed input handle stale.
func TestNilZeroAndClosedHandlesAreRejected(t *testing.T) {
	for _, session := range []*RenderSessionHandle{nil, {}} {
		if _, err := session.Capabilities(); !errors.Is(err, StatusInvalidArgument) {
			t.Fatalf("receiver %#v: %v, want StatusInvalidArgument", session, err)
		}
	}
	for _, frame := range []*AcquiredFrameHandle{nil, {}} {
		err := frame.WithProducerSync(func(GpuSyncView) error {
			t.Fatal("a view callback ran for an invalid frame")
			return nil
		})
		if !errors.Is(err, StatusInvalidArgument) {
			t.Fatalf("frame %#v: %v, want StatusInvalidArgument", frame, err)
		}
	}

	f := newFixture(t)
	for _, input := range []*GeojsonSourceDataHandle{nil, {}} {
		if future, err := f.m.AddGeojsonSourceData("source", input); future != nil || !errors.Is(err, StatusInvalidArgument) {
			t.Fatalf("input %#v: (%v, %v), want nil and StatusInvalidArgument", input, future, err)
		}
	}
	data, err := GeojsonSourceDataCreate(`{"type":"FeatureCollection","features":[]}`, nil)
	if err != nil {
		t.Fatal(err)
	}
	if err := data.Close(); err != nil {
		t.Fatal(err)
	}
	if future, err := f.m.AddGeojsonSourceData("source", data); future != nil || !errors.Is(err, StatusInvalidState) {
		t.Fatalf("closed input: (%v, %v), want nil and StatusInvalidState", future, err)
	}
}

// A map holds its runtime: with no other reference left, a collection neither
// makes the runtime unreachable nor breaks the map, and the runtime becomes
// unreachable, and so due for disposal, only once the map is gone too.
func TestChildKeepsItsParentAlive(t *testing.T) {
	// No fixture: its cleanup would keep the runtime reachable. The map's
	// cleanup closes it if the test fails, and the collector then reclaims the
	// runtime.
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatal(err)
	}
	parent := weak.Make(runtime.bindingOwner)
	m := createMap(t, runtime, testMapOptions())
	t.Cleanup(func() {
		if m != nil {
			await(t, submitted(m.Close()))
		}
	})
	runtime = nil

	// A completed collection clears a weak pointer to anything it found
	// unreachable.
	stdruntime.GC()
	if parent.Value() == nil {
		t.Fatal("the runtime became unreachable while its map is live")
	}
	await(t, submitted(m.SetStyleJson(emptyStyle)))

	// Closing the map leaves nothing that holds the runtime.
	await(t, submitted(m.Close()))
	m = nil
	awaitUnreachable(t, parent, "the runtime once its map closed")
}
