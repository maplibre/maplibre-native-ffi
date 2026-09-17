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
		drained, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
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
		drained, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
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
	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	// A drain never waits for worker progress, so collect successive batches
	// until the style load finishes and preserve their queue order.
	events := collectRuntimeEventsUntil(t, runtime, RuntimeEventMapStyleLoaded)
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
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	drained, err := runtime.DrainEvents()
	if err != nil {
		t.Fatalf("DrainEvents(): %v", err)
	}
	if len(drained) != 0 {
		t.Fatalf("fresh runtime batch = %d events", len(drained))
	}
}

func TestRuntimeEventMasksRoundTripAndRejectUnknownBits(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	// A handle nobody narrowed selects every type this binding version defines.
	mapMask := mapEventMaskForTest(t, m)
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
	runtimeOptions := NewRuntimeOptions("", ":memory:")
	runtimeOptions.EventMask = RuntimeEventMaskAll | RuntimeEventMask(1)<<63
	if runtime, err := NewRuntimeWithOptions(runtimeOptions); !errors.Is(err, ErrInvalidArgument) {
		if err == nil {
			_ = closeRuntimeForTest(runtime)
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

	if mask := mapEventMaskForTest(t, m); mask.Has(RuntimeEventMaskMapStyleLoaded) {
		t.Fatalf("MapSnapshot.EventMask = %#x, want the style-loaded bit cleared", uint64(mask))
	}

	style, err := m.SetStyleJSON([]byte(emptyStyleJSON))
	if _, err := awaitForTest(style, err); err != nil {
		t.Fatalf("SetStyleJSON completion: %v", err)
	}
	// The style load and a runtime barrier both finish before the drain, so
	// every event the load produced is queued by the time the drain reads it.
	waitForRuntimeBarrier(t, runtime)
	types := eventTypes(drainQueuedRuntimeEvents(t, runtime))
	if !slices.Contains(types, RuntimeEventMapLoadingStarted) {
		t.Fatalf("drained event types = %v, want the loading-started event the mask kept", types)
	}
	if slices.Contains(types, RuntimeEventMapStyleLoaded) {
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
	var kept []RuntimeEvent
	for range make([]struct{}, 5000) {
		drained, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		if slices.Contains(eventTypes(drained), RuntimeEventMapLoadingFailed) {
			kept = drained
			break
		}
		time.Sleep(time.Millisecond)
	}
	failureIndex := slices.Index(eventTypes(kept), RuntimeEventMapLoadingFailed)
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
	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	collectRuntimeEventsUntil(t, runtime, RuntimeEventMapStyleLoaded)
	if _, err := m.SetStyleURL("also-unsupported://style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	collectRuntimeEventsUntil(t, runtime, RuntimeEventMapLoadingFailed)

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
	if len(decoded) != 3 {
		t.Fatalf("decoded %d events, want 3", len(decoded))
	}
	frame, ok := decoded[0].Payload.(RuntimeEventRenderFramePayload)
	if !ok || frame.Mode != RenderModeFull || !frame.NeedsRepaint {
		t.Fatalf("render frame payload = %+v", decoded[0].Payload)
	}
	tile, ok := decoded[1].Payload.(RuntimeEventTileActionPayload)
	if !ok || tile.Operation != TileOperationLoadFromCache || tile.TileID.Wrap != -1 || tile.TileID.CanonicalX != 7 {
		t.Fatalf("tile action payload = %+v", decoded[1].Payload)
	}
	if decoded[1].Message != "roads" {
		t.Fatalf("tile action message = %q, want the source ID", decoded[1].Message)
	}
	transition, ok := decoded[2].Payload.(RuntimeEventCameraTransitionFinishedPayload)
	if !ok || transition.TransitionID != 7 {
		t.Fatalf("camera transition payload = %+v", decoded[2].Payload)
	}
}

func TestRuntimeEventKnownPayloadsDecodeFromTheUnion(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
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
	if len(decoded) != 6 {
		t.Fatalf("decoded %d events, want 6", len(decoded))
	}
	renderMap, ok := decoded[0].Payload.(RuntimeEventRenderMapPayload)
	if !ok || renderMap.Mode != RenderModePartial {
		t.Fatalf("render map payload = %+v", decoded[0].Payload)
	}
	transition, ok := decoded[1].Payload.(RuntimeEventCameraTransitionFinishedPayload)
	if !ok || transition.TransitionID != 99 {
		t.Fatalf("transition payload = %+v", decoded[1].Payload)
	}
	regionStatus, ok := decoded[2].Payload.(RuntimeEventOfflineRegionStatusPayload)
	if !ok || regionStatus.RegionID != 5 || regionStatus.Status.CompletedTileCount != 12 || !regionStatus.Status.Complete {
		t.Fatalf("region status payload = %+v", decoded[2].Payload)
	}
	responseError, ok := decoded[3].Payload.(RuntimeEventOfflineRegionResponseErrorPayload)
	if !ok || responseError.Reason != ResourceErrorReasonConnection {
		t.Fatalf("response error payload = %+v", decoded[3].Payload)
	}
	if decoded[3].Message != "connection reset" {
		t.Fatalf("response error message = %q", decoded[3].Message)
	}
	tileLimit, ok := decoded[4].Payload.(RuntimeEventOfflineRegionTileCountLimitPayload)
	if !ok || tileLimit.Limit != 6000 {
		t.Fatalf("tile count limit payload = %+v", decoded[4].Payload)
	}
	// A style-image-missing event carries the image ID as its message, and its
	// payload type of none carries no payload value at all.
	missing := decoded[5]
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
		if err := closeRuntimeForTest(runtime); err != nil {
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
	if len(decoded) != 1 {
		t.Fatalf("decoded %d events, want 1", len(decoded))
	}
	event := decoded[0]
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
	if len(decoded) != 2 {
		t.Fatalf("decoded %d events, want 2", len(decoded))
	}
	if decoded[0].Source.Type != RuntimeEventSourceMap || decoded[0].Source.MapID != mapID {
		t.Fatalf("source = %+v, want map %d", decoded[0].Source, mapID)
	}
	if decoded[0].Source.RawID != uint64(mapID) {
		t.Fatalf("resolved source raw ID = %d, want %d", decoded[0].Source.RawID, uint64(mapID))
	}
	// An event from a map this runtime does not know reports no map identity, and
	// still carries the native id the C API delivered.
	if decoded[1].Source.MapID != 0 {
		t.Fatalf("unknown map source ID = %d, want 0", decoded[1].Source.MapID)
	}
	if decoded[1].Source.RawID != uint64(mapID)+1 {
		t.Fatalf("unresolved source raw ID = %d, want %d", decoded[1].Source.RawID, uint64(mapID)+1)
	}
}
