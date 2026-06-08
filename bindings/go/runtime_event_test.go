package maplibre

import "testing"

func TestRuntimeEventKnownMapPayloads(t *testing.T) {
	t.Run("render frame", func(t *testing.T) {
		event := runtimeEventRenderFrameForTest(uint32(RenderModeFull), true, true, RenderingStats{
			EncodingTime:       1.25,
			RenderingTime:      2.5,
			FrameCount:         3,
			DrawCallCount:      4,
			TotalDrawCallCount: 5,
		})
		payload, ok := event.Payload.(RuntimeEventRenderFramePayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventRenderFramePayload", event.Payload)
		}
		if payload.Mode != RenderModeFull || payload.RawMode != uint32(RenderModeFull) || !payload.NeedsRepaint || !payload.PlacementChanged {
			t.Fatalf("render frame payload = %+v", payload)
		}
		if payload.Stats.EncodingTime != 1.25 || payload.Stats.RenderingTime != 2.5 || payload.Stats.FrameCount != 3 || payload.Stats.DrawCallCount != 4 || payload.Stats.TotalDrawCallCount != 5 {
			t.Fatalf("rendering stats = %+v", payload.Stats)
		}
	})

	t.Run("render map", func(t *testing.T) {
		event := runtimeEventRenderMapForTest(uint32(RenderModePartial))
		payload, ok := event.Payload.(RuntimeEventRenderMapPayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventRenderMapPayload", event.Payload)
		}
		if payload.Mode != RenderModePartial || payload.RawMode != uint32(RenderModePartial) {
			t.Fatalf("render map payload = %+v", payload)
		}
	})

	t.Run("style image missing", func(t *testing.T) {
		event := runtimeEventStyleImageMissingForTest("marker-1")
		payload, ok := event.Payload.(RuntimeEventStyleImageMissingPayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventStyleImageMissingPayload", event.Payload)
		}
		if payload.ImageID != "marker-1" {
			t.Fatalf("style image payload = %+v", payload)
		}
	})

	t.Run("tile action", func(t *testing.T) {
		event := runtimeEventTileActionForTest(uint32(TileOperationLoadFromCache), TileID{
			OverscaledZ: 9,
			Wrap:        -1,
			CanonicalZ:  8,
			CanonicalX:  7,
			CanonicalY:  6,
		}, "roads")
		payload, ok := event.Payload.(RuntimeEventTileActionPayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventTileActionPayload", event.Payload)
		}
		if payload.Operation != TileOperationLoadFromCache || payload.RawOperation != uint32(TileOperationLoadFromCache) || payload.SourceID != "roads" {
			t.Fatalf("tile action payload = %+v", payload)
		}
		if payload.TileID.OverscaledZ != 9 || payload.TileID.Wrap != -1 || payload.TileID.CanonicalZ != 8 || payload.TileID.CanonicalX != 7 || payload.TileID.CanonicalY != 6 {
			t.Fatalf("tile id = %+v", payload.TileID)
		}
	})
}

func TestRuntimeEventUnknownPayloadPreservesBytes(t *testing.T) {
	event := runtimeEventUnknownPayloadForTest(0x7fff_0001, []byte{1, 2, 3})
	payload, ok := event.Payload.(RuntimeEventUnknownPayload)
	if !ok {
		t.Fatalf("Payload type = %T, want RuntimeEventUnknownPayload", event.Payload)
	}
	if event.PayloadType != RuntimeEventPayloadType(0x7fff_0001) || event.PayloadSize != 3 {
		t.Fatalf("event payload metadata = (%d, %d)", event.PayloadType, event.PayloadSize)
	}
	if len(payload.Bytes) != 3 || payload.Bytes[0] != 1 || payload.Bytes[1] != 2 || payload.Bytes[2] != 3 {
		t.Fatalf("unknown payload bytes = %v", payload.Bytes)
	}
}

func TestRuntimeEventMapSourceUsesRuntimeLocalID(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	event := runtimeEventMapSourceForTest(runtime, m)
	if event.Source.Type != RuntimeEventSourceMap {
		t.Fatalf("source type = %v, want map", event.Source.Type)
	}
	if event.Source.MapID == 0 || event.Source.MapID != m.id {
		t.Fatalf("source map ID = %d, want %d", event.Source.MapID, m.id)
	}
}
