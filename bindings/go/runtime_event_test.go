package maplibre

import (
	"errors"
	"slices"
	"strings"
	"testing"
	"time"
)

func waitForRuntimeBarrier(t *testing.T, runtime *RuntimeHandle) {
	t.Helper()
	operation, err := runtime.Barrier()
	if _, err := awaitForTest(operation, err); err != nil {
		t.Fatalf("Barrier completion: %v", err)
	}
}

// drainQueuedRuntimeEvents drains what the queue already holds, in queue order,
// and stops at the first empty batch. It waits for no further work, so a caller
// fences the work it wants to observe before it drains.
func drainQueuedRuntimeEvents(t *testing.T, runtime *RuntimeHandle) []RuntimeEvent {
	t.Helper()
	var events []RuntimeEvent
	for range make([]struct{}, 100) {
		drained, err := drainEventsForTest(runtime)
		if err != nil {
			t.Fatalf("Close(): %v", err)
		}
		if len(drained) == 0 {
			return events
		}
		events = append(events, drained...)
	}
	t.Fatal("the runtime kept producing events")
	return nil
}

// collectRuntimeEventsUntil drains until every wanted event type has arrived,
// and returns every event it saw.
func collectRuntimeEventsUntil(t *testing.T, runtime *RuntimeHandle, wanted ...RuntimeEventType) []RuntimeEvent {
	t.Helper()
	var events []RuntimeEvent
	seen := make(map[RuntimeEventType]bool, len(wanted))
	for range make([]struct{}, 5000) {
		drained, err := drainEventsForTest(runtime)
		if err != nil {
			t.Fatalf("Close(): %v", err)
		}
		for _, event := range drained {
			seen[event.Type] = true
		}
		events = append(events, drained...)
		complete := true
		for _, eventType := range wanted {
			if !seen[eventType] {
				complete = false
				break
			}
		}
		if complete {
			return events
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("timed out waiting for runtime events %v", wanted)
	return nil
}

// waitForRuntimeEvent drains until one event of the wanted type arrives, and
// returns it.
func waitForRuntimeEvent(t *testing.T, runtime *RuntimeHandle, eventType RuntimeEventType) RuntimeEvent {
	t.Helper()
	events := collectRuntimeEventsUntil(t, runtime, eventType)
	index := slices.IndexFunc(events, func(event RuntimeEvent) bool {
		return event.Type == eventType
	})
	return events[index]
}

func eventTypes(events []RuntimeEvent) []RuntimeEventType {
	types := make([]RuntimeEventType, len(events))
	for index, event := range events {
		types[index] = event.Type
	}
	return types
}

func TestRuntimeDrainReportsStyleLoadInQueueOrder(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	drainQueuedRuntimeEvents(t, runtime)
	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(): %v", err)
	}
	// A drain never waits for worker progress, so collect successive batches
	// until the style load finishes and preserve their queue order.
	events := collectRuntimeEventsUntil(t, runtime, RuntimeEventTypeMapStyleLoaded)
	types := eventTypes(events)
	if len(types) < 2 {
		t.Fatalf("style load events = %v, want more than one event", types)
	}
	started := slices.Index(types, RuntimeEventTypeMapLoadingStarted)
	styleLoaded := slices.Index(types, RuntimeEventTypeMapStyleLoaded)
	if started < 0 || started > styleLoaded {
		t.Fatalf(
			"style load events = %v, want loading-started before style-loaded",
			types,
		)
	}
}

func TestRuntimeDrainEmptiesFreshRuntime(t *testing.T) {
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	drained, err := drainEventsForTest(runtime)
	if err != nil {
		t.Fatalf("Close(): %v", err)
	}
	if len(drained) != 0 {
		t.Fatalf("fresh runtime batch = %d events", len(drained))
	}
}

func TestRuntimeEventMasksRoundTripAndRejectUnknownBits(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	// A handle nobody narrowed selects every type this binding version defines.
	mapMask := mapEventMaskForTest(t, m)
	runtimeMask, err := runtime.GetEventMask()
	if err != nil {
		t.Fatalf("runtime EventMask(): %v", err)
	}
	if mapMask != RuntimeEventMaskAll || runtimeMask != RuntimeEventMaskAll {
		t.Fatalf("default masks = (%#x, %#x), want %#x", uint64(mapMask), uint64(runtimeMask), uint64(RuntimeEventMaskAll))
	}

	if _, err := m.SetEventMask(RuntimeEventMaskAll); err != nil {
		t.Fatalf("map SetEventMask(all): %v", err)
	}
	if err := runtime.SetEventMask(RuntimeEventMaskAll); err != nil {
		t.Fatalf("runtime SetEventMask(all): %v", err)
	}

	// A read-modify-write keeps every other bit.
	narrowed := mapMask &^ RuntimeEventMaskMapTileAction
	if _, err := m.SetEventMask(narrowed); err != nil {
		t.Fatalf("map SetEventMask(narrowed): %v", err)
	}
	waitForRuntimeBarrier(t, runtime)
	readBack := mapEventMaskForTest(t, m)
	if readBack != narrowed {
		t.Fatalf("map mask = %#x, want %#x", uint64(readBack), uint64(narrowed))
	}
	if readBack.Has(RuntimeEventMaskMapTileAction) {
		t.Fatal("map mask kept the cleared tile-action bit")
	}
	if !readBack.Has(RuntimeEventMaskMapIdle | RuntimeEventMaskMapStyleLoaded) {
		t.Fatalf("map mask = %#x, want the untouched bits kept", uint64(readBack))
	}

	restored := readBack | RuntimeEventMaskMapTileAction
	if _, err := m.SetEventMask(restored); err != nil {
		t.Fatalf("map SetEventMask(restored): %v", err)
	}
	waitForRuntimeBarrier(t, runtime)
	if readBack = mapEventMaskForTest(t, m); readBack != RuntimeEventMaskAll {
		t.Fatalf("map mask after restore = %#x, want %#x", uint64(readBack), uint64(RuntimeEventMaskAll))
	}

	unknown := RuntimeEventMaskAll | RuntimeEventMask(1)<<63
	if _, err := m.SetEventMask(unknown); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("map SetEventMask(unknown bit) error = %v, want ErrInvalidArgument", err)
	}
	if err := runtime.SetEventMask(unknown); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("runtime SetEventMask(unknown bit) error = %v, want ErrInvalidArgument", err)
	}
	// A rejected mask leaves the previous selection in place.
	if readBack = mapEventMaskForTest(t, m); readBack != RuntimeEventMaskAll {
		t.Fatalf("map mask after a rejected set = %#x, want %#x", uint64(readBack), uint64(RuntimeEventMaskAll))
	}
}

// Both option constructors validate the mask before native sees it, so an
// unknown bit is rejected at creation as well as by the setters.
func TestOptionsEventMaskRejectsUnknownBits(t *testing.T) {
	runtimeOptions := runtimeOptionsForTest("", ":memory:")
	runtimeOptions.EventMask = RuntimeEventMaskAll | RuntimeEventMask(1)<<63
	if runtime, err := RuntimeCreate(runtimeOptions); !errors.Is(err, ErrInvalidArgument) {
		if err == nil {
			_ = closeRuntimeForTest(runtime)
		}
		t.Fatalf("RuntimeCreate(unknown bit) error = %v, want ErrInvalidArgument", err)
	}

	runtime, _ := newRuntimeAndMap(t, nil)
	mapOptions := mapOptionsForTest(64, 64, 1)
	mapOptions.EventMask = RuntimeEventMaskAll | RuntimeEventMask(1)<<63
	if _, err := runtime.MapCreate(mapOptions); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewMapWithOptions(unknown bit) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapOptionsEventMaskSuppressesClearedTypesFromCreation(t *testing.T) {
	options := mapOptionsForTest(64, 64, 1)
	options.EventMask = RuntimeEventMaskAll &^ RuntimeEventMaskMapStyleLoaded
	runtime, m := newRuntimeAndMap(t, &options)

	if mask := mapEventMaskForTest(t, m); mask.Has(RuntimeEventMaskMapStyleLoaded) {
		t.Fatalf("MapSnapshot.EventMask = %#x, want the style-loaded bit cleared", uint64(mask))
	}

	style, err := m.SetStyleJson([]byte(emptyStyleJSON))
	if _, err := awaitForTest(style, err); err != nil {
		t.Fatalf("SetStyleJSON completion: %v", err)
	}
	// The style load and a runtime barrier both finish before the drain, so
	// every event the load produced is queued by the time the drain reads it.
	waitForRuntimeBarrier(t, runtime)
	types := eventTypes(drainQueuedRuntimeEvents(t, runtime))
	if !slices.Contains(types, RuntimeEventTypeMapLoadingStarted) {
		t.Fatalf("drained event types = %v, want the loading-started event the mask kept", types)
	}
	if slices.Contains(types, RuntimeEventTypeMapStyleLoaded) {
		t.Fatalf("drained event types = %v, want no style-loaded event", types)
	}
}

func TestRuntimeDrainAndMaskSettersMigrateAcrossGoroutines(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	results := make(chan error, 3)
	go func() {
		_, err := drainEventsForTest(runtime)
		results <- err
		results <- runtime.SetEventMask(RuntimeEventMaskAll)
		_, err = m.SetEventMask(RuntimeEventMaskAll)
		results <- err
	}()
	for i := 0; i < 3; i++ {
		if err := <-results; err != nil {
			t.Fatalf("any-thread runtime/map call %d: %v", i, err)
		}
	}
}

func TestRuntimeEventCopiesSurviveTheNextDrain(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	// A failed style load carries text, so this batch holds arena-backed values.
	if _, err := m.SetStyleUrl("unsupported://style.json"); err != nil {
		t.Fatalf("SetStyleUrl(): %v", err)
	}
	var kept []RuntimeEvent
	for range make([]struct{}, 5000) {
		drained, err := drainEventsForTest(runtime)
		if err != nil {
			t.Fatalf("Close(): %v", err)
		}
		if slices.Contains(eventTypes(drained), RuntimeEventTypeMapLoadingFailed) {
			kept = drained
			break
		}
		time.Sleep(time.Millisecond)
	}
	failureIndex := slices.Index(eventTypes(kept), RuntimeEventTypeMapLoadingFailed)
	if failureIndex < 0 {
		t.Fatal("the map did not report a loading failure before the deadline")
	}
	failure := kept[failureIndex]
	if failure.Message == "" {
		t.Fatal("the loading failure carried no message")
	}
	snapshotTypes := eventTypes(kept)
	snapshotMessages := make([]string, len(kept))
	for index, event := range kept {
		snapshotMessages[index] = event.Message
	}

	// Two more style loads reuse the runtime's event and message storage.
	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(): %v", err)
	}
	collectRuntimeEventsUntil(t, runtime, RuntimeEventTypeMapStyleLoaded)
	if _, err := m.SetStyleUrl("also-unsupported://style.json"); err != nil {
		t.Fatalf("SetStyleUrl(): %v", err)
	}
	collectRuntimeEventsUntil(t, runtime, RuntimeEventTypeMapLoadingFailed)

	if !slices.Equal(eventTypes(kept), snapshotTypes) {
		t.Fatalf("kept event types = %v, want %v", eventTypes(kept), snapshotTypes)
	}
	for index, event := range kept {
		if event.Message != snapshotMessages[index] {
			t.Fatalf("kept event %d message = %q, want %q", index, event.Message, snapshotMessages[index])
		}
	}
	if !strings.Contains(failure.Message, "unsupported://style.json") {
		t.Fatalf("kept failure message = %q, want the style URL it named", failure.Message)
	}
}
