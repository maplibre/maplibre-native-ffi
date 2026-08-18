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

// drainAllRuntimeEvents waits for autonomous work to settle, then returns
// everything it drained in queue order.
func drainAllRuntimeEvents(t *testing.T, runtime *RuntimeHandle) []RuntimeEvent {
	t.Helper()
	var events []RuntimeEvent
	for range make([]struct{}, 100) {
		time.Sleep(time.Millisecond)
		batch, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		if len(batch.Events) == 0 {
			return events
		}
		events = append(events, batch.Events...)
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
		time.Sleep(time.Millisecond)
		batch, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		for _, event := range batch.Events {
			seen[event.Type] = true
		}
		events = append(events, batch.Events...)
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

const emptyStyleJSON = `{"version":8,"sources":{},"layers":[]}`

// newRuntimeAndMap creates a runtime and one map on the calling OS thread, and
// registers their close.
func newRuntimeAndMap(t *testing.T, options *MapOptions) (*RuntimeHandle, *MapHandle) {
	t.Helper()

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	var m *MapHandle
	if options == nil {
		m, err = awaitForTest(runtime.NewMap())
	} else {
		m, err = awaitForTest(runtime.NewMapWithOptions(*options))
	}
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	t.Cleanup(func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	})
	return runtime, m
}

func TestRuntimeDrainReportsStyleLoadInQueueOrder(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	drainAllRuntimeEvents(t, runtime)
	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	// A drain never waits for worker progress, so collect successive owned
	// batches until the style load finishes and preserve their queue order.
	var events []RuntimeEvent
	for range make([]struct{}, 5000) {
		time.Sleep(time.Millisecond)
		batch, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		events = append(events, batch.Events...)
		if slices.Contains(eventTypes(batch.Events), RuntimeEventMapStyleLoaded) {
			break
		}
		time.Sleep(time.Millisecond)
	}
	types := eventTypes(events)
	if len(types) < 2 {
		t.Fatalf("style load events = %v, want more than one event", types)
	}
	started := slices.Index(types, RuntimeEventMapLoadingStarted)
	styleLoaded := slices.Index(types, RuntimeEventMapStyleLoaded)
	if started < 0 || started > styleLoaded {
		t.Fatalf(
			"style load events = %v, want loading-started before style-loaded",
			types,
		)
	}
}

func TestRuntimeDrainEmptiesFreshRuntime(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	batch, err := runtime.DrainEvents()
	if err != nil {
		t.Fatalf("DrainEvents(): %v", err)
	}
	if len(batch.Events) != 0 {
		t.Fatalf("fresh runtime batch = %d events", len(batch.Events))
	}
}

func TestRuntimeEventMasksRoundTripAndRejectUnknownBits(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	// A handle nobody narrowed selects every type this binding version defines.
	mapMask, err := m.EventMask()
	if err != nil {
		t.Fatalf("map EventMask(): %v", err)
	}
	runtimeMask, err := runtime.EventMask()
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
	readBack, err := m.EventMask()
	if err != nil {
		t.Fatalf("map EventMask(): %v", err)
	}
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
	if readBack, err = m.EventMask(); err != nil || readBack != RuntimeEventMaskAll {
		t.Fatalf("map mask after restore = (%#x, %v), want %#x", uint64(readBack), err, uint64(RuntimeEventMaskAll))
	}

	unknown := RuntimeEventMaskAll | RuntimeEventMask(1)<<63
	if _, err := m.SetEventMask(unknown); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("map SetEventMask(unknown bit) error = %v, want ErrInvalidArgument", err)
	}
	if err := runtime.SetEventMask(unknown); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("runtime SetEventMask(unknown bit) error = %v, want ErrInvalidArgument", err)
	}
	// A rejected mask leaves the previous selection in place.
	if readBack, err = m.EventMask(); err != nil || readBack != RuntimeEventMaskAll {
		t.Fatalf("map mask after a rejected set = (%#x, %v), want %#x", uint64(readBack), err, uint64(RuntimeEventMaskAll))
	}
}

// Both option constructors validate the mask before native sees it, so an
// unknown bit is rejected at creation as well as by the setters.
func TestOptionsEventMaskRejectsUnknownBits(t *testing.T) {
	runtimeOptions := NewRuntimeOptions("", ":memory:")
	runtimeOptions.EventMask = RuntimeEventMaskAll | RuntimeEventMask(1)<<63
	if runtime, err := NewRuntimeWithOptions(runtimeOptions); !errors.Is(err, ErrInvalidArgument) {
		if err == nil {
			_ = runtime.Close()
		}
		t.Fatalf("NewRuntimeWithOptions(unknown bit) error = %v, want ErrInvalidArgument", err)
	}

	runtime, _ := newRuntimeAndMap(t, nil)
	mapOptions := NewMapOptions(64, 64, 1)
	mapOptions.EventMask = RuntimeEventMaskAll | RuntimeEventMask(1)<<63
	if _, err := runtime.NewMapWithOptions(mapOptions); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewMapWithOptions(unknown bit) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapOptionsEventMaskSuppressesClearedTypesFromCreation(t *testing.T) {
	options := NewMapOptions(64, 64, 1)
	options.EventMask = RuntimeEventMaskAll &^ RuntimeEventMaskMapStyleLoaded
	runtime, m := newRuntimeAndMap(t, &options)

	mask, err := m.EventMask()
	if err != nil {
		t.Fatalf("EventMask(): %v", err)
	}
	if mask.Has(RuntimeEventMaskMapStyleLoaded) {
		t.Fatalf("EventMask() = %#x, want the style-loaded bit cleared", uint64(mask))
	}

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	// Loading-started and style-loaded arrive in one batch, so a run that saw the
	// first would have seen the second.
	events := collectRuntimeEventsUntil(t, runtime, RuntimeEventMapLoadingStarted)
	events = append(events, drainAllRuntimeEvents(t, runtime)...)
	if types := eventTypes(events); slices.Contains(types, RuntimeEventMapStyleLoaded) {
		t.Fatalf("drained event types = %v, want no style-loaded event", types)
	}
}

func TestRuntimeDrainAndMaskSettersMigrateAcrossGoroutines(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	results := make(chan error, 3)
	go func() {
		_, err := runtime.DrainEvents()
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
	if _, err := m.SetStyleURL("unsupported://style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	var kept RuntimeEventBatch
	for range make([]struct{}, 5000) {
		time.Sleep(time.Millisecond)
		batch, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		if slices.Contains(eventTypes(batch.Events), RuntimeEventMapLoadingFailed) {
			kept = batch
			break
		}
		time.Sleep(time.Millisecond)
	}
	failure := kept.Events[slices.Index(eventTypes(kept.Events), RuntimeEventMapLoadingFailed)]
	if failure.Message == "" {
		t.Fatal("the loading failure carried no message")
	}
	snapshotTypes := eventTypes(kept.Events)
	snapshotMessages := make([]string, len(kept.Events))
	for index, event := range kept.Events {
		snapshotMessages[index] = event.Message
	}

	// Two more style loads reuse the runtime's event and message storage.
	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	collectRuntimeEventsUntil(t, runtime, RuntimeEventMapStyleLoaded)
	if _, err := m.SetStyleURL("also-unsupported://style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	collectRuntimeEventsUntil(t, runtime, RuntimeEventMapLoadingFailed)

	if !slices.Equal(eventTypes(kept.Events), snapshotTypes) {
		t.Fatalf("kept event types = %v, want %v", eventTypes(kept.Events), snapshotTypes)
	}
	for index, event := range kept.Events {
		if event.Message != snapshotMessages[index] {
			t.Fatalf("kept event %d message = %q, want %q", index, event.Message, snapshotMessages[index])
		}
	}
	if !strings.Contains(failure.Message, "unsupported://style.json") {
		t.Fatalf("kept failure message = %q, want the style URL it named", failure.Message)
	}
}

func TestRuntimeEventDecoderUsesTheBatchStride(t *testing.T) {
	runtime, _ := newRuntimeAndMap(t, nil)
	stride := runtimeEventSizeForTest()

	// A batch whose stride exceeds the compiled event size is what a later C API
	// version looks like, and every event after the first misdecodes when the
	// decoder strides by its own struct size.
	batch := newRuntimeEventBatchForTest(stride+16, []runtimeEventForTest{
		newRuntimeEventForTest(RuntimeEventMapRenderFrameFinished, RuntimeEventSourceMap, 0, 0).
			withRenderFrame(RuntimeEventRenderFramePayload{RawMode: uint32(RenderModeFull), NeedsRepaint: true}),
		newRuntimeEventForTest(RuntimeEventMapTileAction, RuntimeEventSourceMap, 0, 0).
			withMessage("roads").
			withTileAction(RuntimeEventTileActionPayload{
				RawOperation: uint32(TileOperationLoadFromCache),
				TileID:       TileID{OverscaledZ: 9, Wrap: -1, CanonicalZ: 8, CanonicalX: 7, CanonicalY: 6},
			}),
		newRuntimeEventForTest(RuntimeEventMapCameraTransitionFinished, RuntimeEventSourceMap, 0, 0).
			withCameraTransitionFinished(RuntimeEventCameraTransitionFinishedPayload{
				TransitionID: 7,
			}),
	})
	defer batch.free()

	decoded := runtime.decodeForTest(batch)
	if len(decoded.Events) != 3 {
		t.Fatalf("decoded %d events, want 3", len(decoded.Events))
	}
	frame, ok := decoded.Events[0].Payload.(RuntimeEventRenderFramePayload)
	if !ok || frame.Mode != RenderModeFull || !frame.NeedsRepaint {
		t.Fatalf("render frame payload = %+v", decoded.Events[0].Payload)
	}
	tile, ok := decoded.Events[1].Payload.(RuntimeEventTileActionPayload)
	if !ok || tile.Operation != TileOperationLoadFromCache || tile.TileID.Wrap != -1 || tile.TileID.CanonicalX != 7 {
		t.Fatalf("tile action payload = %+v", decoded.Events[1].Payload)
	}
	if decoded.Events[1].Message != "roads" {
		t.Fatalf("tile action message = %q, want the source ID", decoded.Events[1].Message)
	}
	transition, ok := decoded.Events[2].Payload.(RuntimeEventCameraTransitionFinishedPayload)
	if !ok || transition.TransitionID != 7 {
		t.Fatalf("camera transition payload = %+v", decoded.Events[2].Payload)
	}
}

func TestRuntimeEventKnownPayloadsDecodeFromTheUnion(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	status := offlineRegionStatusForTest(uint32(OfflineRegionDownloadActive))
	status.CompletedTileCount = 12
	status.Complete = true
	batch := newRuntimeEventBatchForTest(0, []runtimeEventForTest{
		newRuntimeEventForTest(RuntimeEventMapRenderMapFinished, RuntimeEventSourceMap, 0, 0).
			withRenderMap(RuntimeEventRenderMapPayload{RawMode: uint32(RenderModePartial)}),
		newRuntimeEventForTest(RuntimeEventMapCameraTransitionFinished, RuntimeEventSourceMap, 0, 0).
			withCameraTransitionFinished(RuntimeEventCameraTransitionFinishedPayload{TransitionID: 99}),
		newRuntimeEventForTest(RuntimeEventOfflineRegionStatusChanged, RuntimeEventSourceRuntime, 0, 0).
			withOfflineRegionStatus(RuntimeEventOfflineRegionStatusPayload{RegionID: 5, Status: status}),
		newRuntimeEventForTest(RuntimeEventOfflineRegionResponseError, RuntimeEventSourceRuntime, 0, 0).
			withMessage("connection reset").
			withOfflineRegionResponseError(RuntimeEventOfflineRegionResponseErrorPayload{RegionID: 5, RawReason: uint32(ResourceErrorReasonConnection)}),
		newRuntimeEventForTest(RuntimeEventOfflineRegionTileCountLimitExceeded, RuntimeEventSourceRuntime, 0, 0).
			withOfflineRegionTileCountLimit(RuntimeEventOfflineRegionTileCountLimitPayload{RegionID: 5, Limit: 6000}),
		newRuntimeEventForTest(RuntimeEventMapStyleImageMissing, RuntimeEventSourceMap, 0, 0).
			withMessage("marker-1"),
	})
	defer batch.free()

	decoded := runtime.decodeForTest(batch)
	if len(decoded.Events) != 6 {
		t.Fatalf("decoded %d events, want 6", len(decoded.Events))
	}
	renderMap, ok := decoded.Events[0].Payload.(RuntimeEventRenderMapPayload)
	if !ok || renderMap.Mode != RenderModePartial {
		t.Fatalf("render map payload = %+v", decoded.Events[0].Payload)
	}
	transition, ok := decoded.Events[1].Payload.(RuntimeEventCameraTransitionFinishedPayload)
	if !ok || transition.TransitionID != 99 {
		t.Fatalf("transition payload = %+v", decoded.Events[1].Payload)
	}
	regionStatus, ok := decoded.Events[2].Payload.(RuntimeEventOfflineRegionStatusPayload)
	if !ok || regionStatus.RegionID != 5 || regionStatus.Status.CompletedTileCount != 12 || !regionStatus.Status.Complete {
		t.Fatalf("region status payload = %+v", decoded.Events[2].Payload)
	}
	responseError, ok := decoded.Events[3].Payload.(RuntimeEventOfflineRegionResponseErrorPayload)
	if !ok || responseError.Reason != ResourceErrorReasonConnection {
		t.Fatalf("response error payload = %+v", decoded.Events[3].Payload)
	}
	if decoded.Events[3].Message != "connection reset" {
		t.Fatalf("response error message = %q", decoded.Events[3].Message)
	}
	tileLimit, ok := decoded.Events[4].Payload.(RuntimeEventOfflineRegionTileCountLimitPayload)
	if !ok || tileLimit.Limit != 6000 {
		t.Fatalf("tile count limit payload = %+v", decoded.Events[4].Payload)
	}
	// A style-image-missing event carries the image ID as its message, and its
	// payload type of none carries no payload value at all.
	missing := decoded.Events[5]
	if missing.Payload != nil || missing.PayloadType != RuntimeEventPayloadNone || missing.Message != "marker-1" {
		t.Fatalf("style-image-missing event = %+v", missing)
	}
}

func TestRuntimeEventUnknownDomainsPreserveRawValues(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	window := make([]byte, runtimeEventPayloadWindowSizeForTest())
	for index := range window {
		window[index] = byte(index + 1)
	}
	batch := newRuntimeEventBatchForTest(0, []runtimeEventForTest{
		newRuntimeEventForTest(RuntimeEventType(0x7fff_0001), RuntimeEventSourceType(0x7fff_0002), 0x7fff_0004, 0).
			withRawPayload(RuntimeEventPayloadType(0x7fff_0003), window),
	})
	defer batch.free()

	decoded := runtime.decodeForTest(batch)
	if len(decoded.Events) != 1 {
		t.Fatalf("decoded %d events, want 1", len(decoded.Events))
	}
	event := decoded.Events[0]
	if event.Type != RuntimeEventType(0x7fff_0001) || event.SourceType != RuntimeEventSourceType(0x7fff_0002) {
		t.Fatalf("unknown event domains = (%d, %d)", event.Type, event.SourceType)
	}
	// A source type this build does not name still reports the native source id.
	if event.Source.Type != RuntimeEventSourceType(0x7fff_0002) || event.Source.RawID != 0x7fff_0004 {
		t.Fatalf("unknown source = %+v, want type %d and raw ID %d", event.Source, 0x7fff_0002, 0x7fff_0004)
	}
	if event.Source.MapID != 0 {
		t.Fatalf("unknown source map ID = %d, want 0", event.Source.MapID)
	}
	if event.PayloadType != RuntimeEventPayloadType(0x7fff_0003) {
		t.Fatalf("unknown payload type = %d", event.PayloadType)
	}
	payload, ok := event.Payload.(RuntimeEventUnknownPayload)
	if !ok {
		t.Fatalf("Payload type = %T, want RuntimeEventUnknownPayload", event.Payload)
	}
	if !slices.Equal(payload.Bytes, window) {
		t.Fatalf("unknown payload bytes = %v, want %v", payload.Bytes, window)
	}

	// The copy is independent of the native window it came from.
	source := batch.payloadWindow(0)
	for index := range source {
		source[index] = 0xff
	}
	if !slices.Equal(payload.Bytes, window) {
		t.Fatalf("unknown payload bytes changed with the source: %v", payload.Bytes)
	}
}

func TestRuntimeEventMapSourceUsesRuntimeLocalID(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	mapID, err := m.ID()
	if err != nil {
		t.Fatalf("ID(): %v", err)
	}
	batch := newRuntimeEventBatchForTest(0, []runtimeEventForTest{
		newRuntimeEventForTest(RuntimeEventMapIdle, RuntimeEventSourceMap, uint64(mapID), 0),
		newRuntimeEventForTest(RuntimeEventMapIdle, RuntimeEventSourceMap, uint64(mapID)+1, 0),
	})
	defer batch.free()

	decoded := runtime.decodeForTest(batch)
	if len(decoded.Events) != 2 {
		t.Fatalf("decoded %d events, want 2", len(decoded.Events))
	}
	if decoded.Events[0].Source.Type != RuntimeEventSourceMap || decoded.Events[0].Source.MapID != mapID {
		t.Fatalf("source = %+v, want map %d", decoded.Events[0].Source, mapID)
	}
	if decoded.Events[0].Source.RawID != uint64(mapID) {
		t.Fatalf("resolved source raw ID = %d, want %d", decoded.Events[0].Source.RawID, uint64(mapID))
	}
	// An event from a map this runtime does not know reports no map identity, and
	// still carries the native id the C API delivered.
	if decoded.Events[1].Source.MapID != 0 {
		t.Fatalf("unknown map source ID = %d, want 0", decoded.Events[1].Source.MapID)
	}
	if decoded.Events[1].Source.RawID != uint64(mapID)+1 {
		t.Fatalf("unresolved source raw ID = %d, want %d", decoded.Events[1].Source.RawID, uint64(mapID)+1)
	}
}
