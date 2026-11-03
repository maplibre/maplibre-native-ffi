package maplibre

import (
	"errors"
	"fmt"
	stdruntime "runtime"
	"testing"
)

// Once a close is accepted, a second close and any other call fail with
// StatusInvalidState and a diagnostic, and Dispose does nothing.
func TestCallsAfterCloseFailWithInvalidState(t *testing.T) {
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

	data, err := NewGeoJsonSourceData(`{"type":"FeatureCollection","features":[]}`, nil)
	if err != nil {
		t.Fatal(err)
	}
	checkHandleMembers(t, data, "GeoJsonSourceDataHandle", false)
	for range 2 {
		if err := data.Close(); err != nil {
			t.Fatalf("GeoJsonSourceDataHandle.Close: %v", err)
		}
	}
	checkHandleMembers(t, data, "GeoJsonSourceDataHandle", true)
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
	for _, input := range []*GeoJsonSourceDataHandle{nil, {}} {
		if future, err := f.m.AddGeoJsonSourceData("source", input); future != nil || !errors.Is(err, StatusInvalidArgument) {
			t.Fatalf("input %#v: (%v, %v), want nil and StatusInvalidArgument", input, future, err)
		}
	}
	data, err := NewGeoJsonSourceData(`{"type":"FeatureCollection","features":[]}`, nil)
	if err != nil {
		t.Fatal(err)
	}
	if err := data.Close(); err != nil {
		t.Fatal(err)
	}
	if future, err := f.m.AddGeoJsonSourceData("source", data); future != nil || !errors.Is(err, StatusInvalidState) {
		t.Fatalf("closed input: (%v, %v), want nil and StatusInvalidState", future, err)
	}
}

// A map holds its runtime. With no other reference to the runtime left, it
// survives a collection: the map's commands still run, and the event handler
// that the runtime holds still receives the map's events.
func TestChildKeepsItsParentAlive(t *testing.T) {
	// No fixture: its cleanup would keep the runtime reachable.
	runtime, err := NewRuntime(&RuntimeOptions{CachePath: ":memory:"})
	if err != nil {
		t.Fatal(err)
	}
	f := &fixture{pendingEvents: make(chan RuntimeEvent, 4096)}
	if err := runtime.Discarding().SetEventHandler(RuntimeEventHandler{Callback: func(events []RuntimeEvent) {
		for _, event := range events {
			offer(f.pendingEvents, event)
		}
	}}); err != nil {
		t.Fatal(err)
	}
	f.m = createMap(t, runtime, testMapOptions())
	// The map is the runtime's only owner, so the teardown reaches the runtime
	// through it.
	t.Cleanup(func() {
		parent := f.m.bindingOwner.parent.(*RuntimeHandle)
		if future, err := f.m.Close(); !errors.Is(err, StatusInvalidState) {
			await(t, submitted(future, err))
		}
		await(t, submitted(parent.Close()))
	})
	runtime = nil

	stdruntime.GC()
	await(t, submitted(f.m.SetStyleJson(emptyStyle)))
	f.awaitEvent(t, "the style load", isStyleLoaded)
}
