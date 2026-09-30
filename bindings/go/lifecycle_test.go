package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
)

func TestClosingTwiceIsSafe(t *testing.T) {
	f := newFixture(t)
	await(t, submitted(f.m.Close()))
	// The second close finds nothing to release and completes at once.
	second, err := f.m.Close()
	if err != nil {
		t.Fatalf("second map close: %v", err)
	}
	select {
	case <-second.Done():
	default:
		t.Fatal("the second close submitted native work")
	}
	if _, err := f.m.Id(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("Id() after close = %v, want ErrInvalidState", err)
	}
	if _, err := f.m.SetStyleJson([]byte(emptyStyle)); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetStyleJson after close = %v, want ErrInvalidState", err)
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
	if teardown != nil || !errors.As(err, &native) || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("runtime close with a live map = (%v, %v), want nil and ErrInvalidState", teardown, err)
	}
	if native.Diagnostic() != "handle still owns live or pending children" {
		t.Fatalf("refusal diagnostic = %q", native.Diagnostic())
	}
	await(t, submitted(f.runtime.Barrier()))
	await(t, submitted(f.m.Close()))
	await(t, submitted(f.runtime.Close()))
}

// A nil pointer or a zero value of a handle type fails with the binding's
// error instead of reaching native, whether it is the receiver or an input,
// and a closed input handle fails before its call crosses into C.
func TestNilZeroAndClosedHandlesAreRejected(t *testing.T) {
	for _, session := range []*RenderSessionHandle{nil, {}} {
		if _, err := session.GetCapabilities(); !errors.Is(err, ErrInvalidState) {
			t.Fatalf("receiver %#v: %v, want ErrInvalidState", session, err)
		}
	}
	for _, frame := range []*AcquiredFrameHandle{nil, {}} {
		err := frame.WithProducerSync(func(GpuSyncView) error {
			t.Fatal("a view callback ran for an invalid frame")
			return nil
		})
		if !errors.Is(err, ErrInvalidState) {
			t.Fatalf("frame %#v: %v, want ErrInvalidState", frame, err)
		}
	}

	f := newFixture(t)
	for _, input := range []*GeojsonSourceDataHandle{nil, {}} {
		if future, err := f.m.AddGeojsonSourceData("source", input); future != nil || !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("input %#v: (%v, %v), want nil and ErrInvalidArgument", input, future, err)
		}
	}
	data, err := GeojsonSourceDataCreate([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatal(err)
	}
	if err := data.Close(); err != nil {
		t.Fatal(err)
	}
	if future, err := f.m.AddGeojsonSourceData("source", data); future != nil || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("closed input: (%v, %v), want nil and ErrInvalidState", future, err)
	}
}

// A map holds its runtime: with no other reference left, collecting the
// runtime's wrapper neither disposes the runtime nor breaks the map, and the
// runtime's cleanup runs only once the map is gone too.
func TestChildKeepsItsParentAlive(t *testing.T) {
	// No fixture: its cleanup would keep the runtime reachable. The map's
	// cleanup closes it if the test fails, and the collector then reclaims the
	// runtime.
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatal(err)
	}
	runtimeDisposed := disposalSignal(runtime.bindingOwner)
	m := await(t, submitted(runtime.MapCreate(DefaultMapOptions())))
	t.Cleanup(func() {
		if m != nil {
			await(t, submitted(m.Close()))
		}
	})
	runtime = nil

	// Two collections, each confirmed by a sentinel's cleanup: an unreachable
	// runtime would be found by the first and cleaned up by the second.
	for range 2 {
		collected := make(chan struct{})
		stdruntime.AddCleanup(new(int), func(done chan struct{}) { close(done) }, collected)
		awaitCollected(t, collected, "a sentinel cleanup")
	}
	select {
	case <-runtimeDisposed:
		t.Fatal("cleanup disposed a runtime whose map is live")
	default:
	}
	awaitCommitted(t, submitted(m.SetStyleJson([]byte(emptyStyle))))

	// Closing the map leaves nothing that holds the runtime.
	await(t, submitted(m.Close()))
	m = nil
	awaitCollected(t, runtimeDisposed, "the runtime's disposal once its map closed")
}
