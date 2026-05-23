package maplibre

import (
	"testing"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

func TestRuntimeEventKnownMapPayloads(t *testing.T) {
	t.Run("render frame", func(t *testing.T) {
		event := runtimeEventFromCAPI(capi.RuntimeEvent{
			PayloadType: capi.RuntimeEventPayloadRenderFrame,
			Payload: capi.RuntimeEventRenderFramePayload{
				Mode:             capi.RenderModeFull,
				NeedsRepaint:     true,
				PlacementChanged: true,
				Stats: capi.RenderingStats{
					EncodingTime:       1.25,
					RenderingTime:      2.5,
					FrameCount:         3,
					DrawCallCount:      4,
					TotalDrawCallCount: 5,
				},
			},
		})
		payload, ok := event.Payload.(RuntimeEventRenderFramePayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventRenderFramePayload", event.Payload)
		}
		if payload.Mode != RenderModeFull || payload.RawMode != capi.RenderModeFull || !payload.NeedsRepaint || !payload.PlacementChanged {
			t.Fatalf("render frame payload = %+v", payload)
		}
		if payload.Stats.EncodingTime != 1.25 || payload.Stats.RenderingTime != 2.5 || payload.Stats.FrameCount != 3 || payload.Stats.DrawCallCount != 4 || payload.Stats.TotalDrawCallCount != 5 {
			t.Fatalf("rendering stats = %+v", payload.Stats)
		}
	})

	t.Run("render map", func(t *testing.T) {
		event := runtimeEventFromCAPI(capi.RuntimeEvent{
			PayloadType: capi.RuntimeEventPayloadRenderMap,
			Payload:     capi.RuntimeEventRenderMapPayload{Mode: capi.RenderModePartial},
		})
		payload, ok := event.Payload.(RuntimeEventRenderMapPayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventRenderMapPayload", event.Payload)
		}
		if payload.Mode != RenderModePartial || payload.RawMode != capi.RenderModePartial {
			t.Fatalf("render map payload = %+v", payload)
		}
	})

	t.Run("style image missing", func(t *testing.T) {
		event := runtimeEventFromCAPI(capi.RuntimeEvent{
			PayloadType: capi.RuntimeEventPayloadStyleImageMissing,
			Payload:     capi.RuntimeEventStyleImageMissingPayload{ImageID: "marker-1"},
		})
		payload, ok := event.Payload.(RuntimeEventStyleImageMissingPayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventStyleImageMissingPayload", event.Payload)
		}
		if payload.ImageID != "marker-1" {
			t.Fatalf("style image payload = %+v", payload)
		}
	})

	t.Run("tile action", func(t *testing.T) {
		event := runtimeEventFromCAPI(capi.RuntimeEvent{
			PayloadType: capi.RuntimeEventPayloadTileAction,
			Payload: capi.RuntimeEventTileActionPayload{
				Operation: capi.TileOperationLoadFromCache,
				TileID: capi.TileID{
					OverscaledZ: 9,
					Wrap:        -1,
					CanonicalZ:  8,
					CanonicalX:  7,
					CanonicalY:  6,
				},
				SourceID: "roads",
			},
		})
		payload, ok := event.Payload.(RuntimeEventTileActionPayload)
		if !ok {
			t.Fatalf("Payload type = %T, want RuntimeEventTileActionPayload", event.Payload)
		}
		if payload.Operation != TileOperationLoadFromCache || payload.RawOperation != capi.TileOperationLoadFromCache || payload.SourceID != "roads" {
			t.Fatalf("tile action payload = %+v", payload)
		}
		if payload.TileID.OverscaledZ != 9 || payload.TileID.Wrap != -1 || payload.TileID.CanonicalZ != 8 || payload.TileID.CanonicalX != 7 || payload.TileID.CanonicalY != 6 {
			t.Fatalf("tile id = %+v", payload.TileID)
		}
	})
}
