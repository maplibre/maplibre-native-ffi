package maplibre

/*
#include <stdlib.h>
#include "maplibre_native_c.h"
*/
import "C"

import "unsafe"

func runtimeEventWithPayloadForTest(payloadType uint32, payload unsafe.Pointer, payloadSize uintptr) *RuntimeEvent {
	raw := C.mln_runtime_event{
		size:         C.uint32_t(unsafe.Sizeof(C.mln_runtime_event{})),
		payload_type: C.uint32_t(payloadType),
		payload:      payload,
		payload_size: C.size_t(payloadSize),
	}
	return runtimeEventFromC(raw)
}

func runtimeEventUnknownPayloadForTest(payloadType uint32, payload []byte) *RuntimeEvent {
	var payloadPtr unsafe.Pointer
	if len(payload) > 0 {
		payloadPtr = unsafe.Pointer(&payload[0])
	}
	return runtimeEventWithPayloadForTest(payloadType, payloadPtr, uintptr(len(payload)))
}

func runtimeEventKnownPayloadSizesForTest() map[RuntimeEventPayloadType]uintptr {
	return map[RuntimeEventPayloadType]uintptr{
		RuntimeEventPayloadRenderFrame:                 unsafe.Sizeof(C.mln_runtime_event_render_frame{}),
		RuntimeEventPayloadRenderMap:                   unsafe.Sizeof(C.mln_runtime_event_render_map{}),
		RuntimeEventPayloadStyleImageMissing:           unsafe.Sizeof(C.mln_runtime_event_style_image_missing{}),
		RuntimeEventPayloadTileAction:                  unsafe.Sizeof(C.mln_runtime_event_tile_action{}),
		RuntimeEventPayloadOfflineRegionStatus:         unsafe.Sizeof(C.mln_runtime_event_offline_region_status{}),
		RuntimeEventPayloadOfflineRegionResponseError:  unsafe.Sizeof(C.mln_runtime_event_offline_region_response_error{}),
		RuntimeEventPayloadOfflineRegionTileCountLimit: unsafe.Sizeof(C.mln_runtime_event_offline_region_tile_count_limit{}),
		RuntimeEventPayloadOfflineOperationCompleted:   unsafe.Sizeof(C.mln_runtime_event_offline_operation_completed{}),
	}
}

func runtimeEventMapSourceForTest(runtime *RuntimeHandle, m *MapHandle) *RuntimeEvent {
	ptr, _ := m.state.Ptr()
	raw := C.mln_runtime_event{
		size:        C.uint32_t(unsafe.Sizeof(C.mln_runtime_event{})),
		source_type: C.uint32_t(C.MLN_RUNTIME_EVENT_SOURCE_MAP),
		source:      unsafe.Pointer(ptr),
	}
	return runtime.runtimeEventFromC(raw)
}

func runtimeEventRenderFrameForTest(mode uint32, needsRepaint bool, placementChanged bool, stats RenderingStats) *RuntimeEvent {
	raw := C.mln_runtime_event_render_frame{
		size:              C.uint32_t(unsafe.Sizeof(C.mln_runtime_event_render_frame{})),
		mode:              C.uint32_t(mode),
		needs_repaint:     C.bool(needsRepaint),
		placement_changed: C.bool(placementChanged),
		stats: C.mln_rendering_stats{
			size:                  C.uint32_t(unsafe.Sizeof(C.mln_rendering_stats{})),
			encoding_time:         C.double(stats.EncodingTime),
			rendering_time:        C.double(stats.RenderingTime),
			frame_count:           C.int64_t(stats.FrameCount),
			draw_call_count:       C.int64_t(stats.DrawCallCount),
			total_draw_call_count: C.int64_t(stats.TotalDrawCallCount),
		},
	}
	return runtimeEventWithPayloadForTest(uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME), unsafe.Pointer(&raw), unsafe.Sizeof(raw))
}

func runtimeEventRenderMapForTest(mode uint32) *RuntimeEvent {
	raw := C.mln_runtime_event_render_map{
		size: C.uint32_t(unsafe.Sizeof(C.mln_runtime_event_render_map{})),
		mode: C.uint32_t(mode),
	}
	return runtimeEventWithPayloadForTest(uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP), unsafe.Pointer(&raw), unsafe.Sizeof(raw))
}

func offlineRegionStatusForTest(downloadState uint32) OfflineRegionStatus {
	raw := C.mln_offline_region_status{
		size:           C.uint32_t(unsafe.Sizeof(C.mln_offline_region_status{})),
		download_state: C.uint32_t(downloadState),
	}
	return offlineRegionStatusFromC(raw)
}

func runtimeEventStyleImageMissingForTest(imageID string) *RuntimeEvent {
	rawImageID := C.CString(imageID)
	defer C.free(unsafe.Pointer(rawImageID))
	raw := C.mln_runtime_event_style_image_missing{
		size:          C.uint32_t(unsafe.Sizeof(C.mln_runtime_event_style_image_missing{})),
		image_id:      rawImageID,
		image_id_size: C.size_t(len(imageID)),
	}
	return runtimeEventWithPayloadForTest(uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING), unsafe.Pointer(&raw), unsafe.Sizeof(raw))
}

func runtimeEventTileActionForTest(operation uint32, tileID TileID, sourceID string) *RuntimeEvent {
	rawSourceID := C.CString(sourceID)
	defer C.free(unsafe.Pointer(rawSourceID))
	raw := C.mln_runtime_event_tile_action{
		size:      C.uint32_t(unsafe.Sizeof(C.mln_runtime_event_tile_action{})),
		operation: C.uint32_t(operation),
		tile_id: C.mln_tile_id{
			overscaled_z: C.uint32_t(tileID.OverscaledZ),
			wrap:         C.int32_t(tileID.Wrap),
			canonical_z:  C.uint32_t(tileID.CanonicalZ),
			canonical_x:  C.uint32_t(tileID.CanonicalX),
			canonical_y:  C.uint32_t(tileID.CanonicalY),
		},
		source_id:      rawSourceID,
		source_id_size: C.size_t(len(sourceID)),
	}
	return runtimeEventWithPayloadForTest(uint32(C.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION), unsafe.Pointer(&raw), unsafe.Sizeof(raw))
}
