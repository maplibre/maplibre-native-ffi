package capi

import "testing"

func TestRuntimeEventPayloadFromCMapPayloads(t *testing.T) {
	t.Run("render frame", func(t *testing.T) {
		copied, ok := testRenderFrameEventPayloadFromC(RenderModeFull, true, true, RenderingStats{
			EncodingTime:       1.25,
			RenderingTime:      2.5,
			FrameCount:         3,
			DrawCallCount:      4,
			TotalDrawCallCount: 5,
		}).(RuntimeEventRenderFramePayload)
		if !ok {
			t.Fatalf("payload type = %T, want RuntimeEventRenderFramePayload", copied)
		}
		if copied.Mode != RenderModeFull || !copied.NeedsRepaint || !copied.PlacementChanged {
			t.Fatalf("render frame payload = %+v", copied)
		}
		if copied.Stats.EncodingTime != 1.25 || copied.Stats.RenderingTime != 2.5 || copied.Stats.FrameCount != 3 || copied.Stats.DrawCallCount != 4 || copied.Stats.TotalDrawCallCount != 5 {
			t.Fatalf("rendering stats = %+v", copied.Stats)
		}
	})

	t.Run("render map", func(t *testing.T) {
		copied, ok := testRenderMapEventPayloadFromC(RenderModePartial).(RuntimeEventRenderMapPayload)
		if !ok {
			t.Fatalf("payload type = %T, want RuntimeEventRenderMapPayload", copied)
		}
		if copied.Mode != RenderModePartial {
			t.Fatalf("render map payload = %+v", copied)
		}
	})

	t.Run("style image missing", func(t *testing.T) {
		copied, ok := testStyleImageMissingEventPayloadFromC("marker-1").(RuntimeEventStyleImageMissingPayload)
		if !ok {
			t.Fatalf("payload type = %T, want RuntimeEventStyleImageMissingPayload", copied)
		}
		if copied.ImageID != "marker-1" {
			t.Fatalf("style image payload = %+v", copied)
		}
	})

	t.Run("tile action", func(t *testing.T) {
		copied, ok := testTileActionEventPayloadFromC(TileOperationLoadFromCache, TileID{
			OverscaledZ: 9,
			Wrap:        -1,
			CanonicalZ:  8,
			CanonicalX:  7,
			CanonicalY:  6,
		}, "roads").(RuntimeEventTileActionPayload)
		if !ok {
			t.Fatalf("payload type = %T, want RuntimeEventTileActionPayload", copied)
		}
		if copied.Operation != TileOperationLoadFromCache || copied.SourceID != "roads" {
			t.Fatalf("tile action payload = %+v", copied)
		}
		if copied.TileID.OverscaledZ != 9 || copied.TileID.Wrap != -1 || copied.TileID.CanonicalZ != 8 || copied.TileID.CanonicalX != 7 || copied.TileID.CanonicalY != 6 {
			t.Fatalf("tile id = %+v", copied.TileID)
		}
	})
}
