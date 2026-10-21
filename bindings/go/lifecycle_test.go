package maplibre

import (
	"errors"
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
	_, err := f.m.SetStyleJson([]byte(emptyStyle))
	var closed *Error
	if !errors.As(err, &closed) || closed.Status != StatusInvalidState || closed.Diagnostic() == "" {
		t.Fatalf("SetStyleJson after close = %v, want native invalid state", err)
	}

	data, err := GeojsonSourceDataCreate([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatal(err)
	}
	for range 2 {
		if err := data.Close(); err != nil {
			t.Fatalf("GeojsonSourceDataHandle.Close: %v", err)
		}
	}
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
	if native.Diagnostic() == "" {
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
	awaitCommitted(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
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
		if _, err := session.GetCapabilities(); !errors.Is(err, StatusInvalidArgument) {
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
	data, err := GeojsonSourceDataCreate([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
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
	m := await(t, submitted(runtime.CreateMap(DefaultMapOptions())))
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
	awaitCommitted(t, submitted(m.SetStyleJson([]byte(emptyStyle))))

	// Closing the map leaves nothing that holds the runtime.
	await(t, submitted(m.Close()))
	m = nil
	awaitUnreachable(t, parent, "the runtime once its map closed")
}
