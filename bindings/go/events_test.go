//go:build mlntest

package maplibre

import (
	"testing"
	"unsafe"
)

// A later C API version may grow the event struct and send types this binding
// does not name. Its batch reports the wider stride, and the decoder steps by
// that stride, so every event after the first still decodes, message and all,
// and an unknown event type, source type, or payload arm keeps its raw value.
func TestEventDecoderReadsAStridedBatchWithUnknownValues(t *testing.T) {
	window := make([]byte, runtimeEventPayloadWindowSizeForTest())
	for index := range window {
		window[index] = byte(index + 1)
	}
	wider := unsafe.Sizeof(runtimeEventForTest{}.raw) + 16
	decoded := decodeEventsForTest(wider, []runtimeEventForTest{
		newRuntimeEventForTest(RuntimeEventTypeMapRenderFrameFinished, RuntimeEventSourceTypeMap, 1).
			withRenderFrame(RuntimeEventRenderFrame{Mode: RenderModeFull, NeedsRepaint: true}),
		newRuntimeEventForTest(RuntimeEventTypeMapTileAction, RuntimeEventSourceTypeMap, 1).
			withMessage("roads").
			withTileAction(RuntimeEventTileAction{
				Operation: TileOperationLoadFromCache,
				TileId:    TileId{OverscaledZ: 9, Wrap: -1, CanonicalZ: 8, CanonicalX: 7, CanonicalY: 6},
			}),
		newRuntimeEventForTest(RuntimeEventType(0x7fff_0001), RuntimeEventSourceType(0x7fff_0002), 0x7fff_0004).
			withGeneration(0x1_0000_0003).
			withRawPayload(RuntimeEventPayloadType(0x7fff_0003), window),
		newRuntimeEventForTest(RuntimeEventTypeMapCameraTransitionFinished, RuntimeEventSourceTypeMap, 1).
			withCameraTransitionFinished(RuntimeEventCameraTransitionFinished{TransitionId: 7}),
	})
	if len(decoded) != 4 {
		t.Fatalf("decoded %d events, want 4", len(decoded))
	}
	if frame, ok := decoded[0].Payload.(RuntimeEventPayloadRenderFrameVariant); !ok || frame.Value.Mode != RenderModeFull || !frame.Value.NeedsRepaint {
		t.Fatalf("render frame payload = %+v", decoded[0].Payload)
	}
	tile, ok := decoded[1].Payload.(RuntimeEventPayloadTileActionVariant)
	if !ok || tile.Value.Operation != TileOperationLoadFromCache || tile.Value.TileId.Wrap != -1 || tile.Value.TileId.CanonicalX != 7 {
		t.Fatalf("tile action payload = %+v", decoded[1].Payload)
	}
	if decoded[1].Message != "roads" {
		t.Fatalf("tile action message = %q, want the source ID", decoded[1].Message)
	}
	unknown := decoded[2]
	if unknown.Type != RuntimeEventType(0x7fff_0001) || unknown.SourceType != RuntimeEventSourceType(0x7fff_0002) || unknown.Source != 0x7fff_0004 || unknown.Generation != 0x1_0000_0003 {
		t.Fatalf("unknown event = %+v, want its raw type, source type, source, and generation", unknown)
	}
	if payload, ok := unknown.Payload.(UnknownVariant); !ok || payload.Tag != 0x7fff_0003 {
		t.Fatalf("unknown payload = %#v, want UnknownVariant with the raw tag", unknown.Payload)
	}
	if transition, ok := decoded[3].Payload.(RuntimeEventPayloadCameraTransitionFinishedVariant); !ok || transition.Value.TransitionId != 7 {
		t.Fatalf("camera transition payload = %+v", decoded[3].Payload)
	}
}

// Every union arm decodes to its own variant type, and an event whose payload
// type carries no value decodes to a nil payload.
func TestEventDecoderReadsEachPayloadArm(t *testing.T) {
	status := OfflineRegionStatus{DownloadState: OfflineRegionDownloadStateActive, CompletedTileCount: 12, Complete: true}
	decoded := decodeEventsForTest(0, []runtimeEventForTest{
		newRuntimeEventForTest(RuntimeEventTypeMapRenderMapFinished, RuntimeEventSourceTypeMap, 1).
			withRenderMap(RuntimeEventRenderMap{Mode: RenderModePartial}),
		newRuntimeEventForTest(RuntimeEventTypeOfflineRegionStatusChanged, RuntimeEventSourceTypeRuntime, 0).
			withOfflineRegionStatus(RuntimeEventOfflineRegionStatus{RegionId: 5, Status: status}),
		newRuntimeEventForTest(RuntimeEventTypeOfflineRegionResponseError, RuntimeEventSourceTypeRuntime, 0).
			withMessage("connection reset").
			withOfflineRegionResponseError(RuntimeEventOfflineRegionResponseError{RegionId: 5, Reason: ResourceErrorReasonConnection}),
		newRuntimeEventForTest(RuntimeEventTypeOfflineRegionTileCountLimitExceeded, RuntimeEventSourceTypeRuntime, 0).
			withOfflineRegionTileCountLimit(RuntimeEventOfflineRegionTileCountLimit{RegionId: 5, Limit: 6000}),
		newRuntimeEventForTest(RuntimeEventTypeMapStyleImageMissing, RuntimeEventSourceTypeMap, 1).
			withMessage("marker-1"),
	})
	if len(decoded) != 5 {
		t.Fatalf("decoded %d events, want 5", len(decoded))
	}
	if renderMap, ok := decoded[0].Payload.(RuntimeEventPayloadRenderMapVariant); !ok || renderMap.Value.Mode != RenderModePartial {
		t.Fatalf("render map payload = %+v", decoded[0].Payload)
	}
	region, ok := decoded[1].Payload.(RuntimeEventPayloadOfflineRegionStatusVariant)
	if !ok || region.Value.RegionId != 5 || region.Value.Status.CompletedTileCount != 12 || !region.Value.Status.Complete {
		t.Fatalf("region status payload = %+v", decoded[1].Payload)
	}
	responseError, ok := decoded[2].Payload.(RuntimeEventPayloadOfflineRegionResponseErrorVariant)
	if !ok || responseError.Value.Reason != ResourceErrorReasonConnection || decoded[2].Message != "connection reset" {
		t.Fatalf("response error event = %+v", decoded[2])
	}
	if limit, ok := decoded[3].Payload.(RuntimeEventPayloadOfflineRegionTileCountLimitVariant); !ok || limit.Value.Limit != 6000 {
		t.Fatalf("tile count limit payload = %+v", decoded[3].Payload)
	}
	if missing := decoded[4]; missing.Payload != nil || missing.Message != "marker-1" {
		t.Fatalf("style-image-missing event = %+v", missing)
	}
}
