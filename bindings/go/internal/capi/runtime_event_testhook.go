package capi

/*
#include <stdlib.h>
#include <maplibre_native_c/runtime.h>
*/
import "C"

import "unsafe"

func testRenderFrameEventPayloadFromC(mode uint32, needsRepaint bool, placementChanged bool, stats RenderingStats) any {
	payload := C.mln_runtime_event_render_frame{
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
	event := C.mln_runtime_event{
		payload_type: C.uint32_t(RuntimeEventPayloadRenderFrame),
		payload:      unsafe.Pointer(&payload),
		payload_size: C.size_t(unsafe.Sizeof(payload)),
	}
	return runtimeEventPayloadFromC(event)
}

func testRenderMapEventPayloadFromC(mode uint32) any {
	payload := C.mln_runtime_event_render_map{
		size: C.uint32_t(unsafe.Sizeof(C.mln_runtime_event_render_map{})),
		mode: C.uint32_t(mode),
	}
	event := C.mln_runtime_event{
		payload_type: C.uint32_t(RuntimeEventPayloadRenderMap),
		payload:      unsafe.Pointer(&payload),
		payload_size: C.size_t(unsafe.Sizeof(payload)),
	}
	return runtimeEventPayloadFromC(event)
}

func testStyleImageMissingEventPayloadFromC(imageID string) any {
	rawImageID := C.CString(imageID)
	defer C.free(unsafe.Pointer(rawImageID))
	payload := C.mln_runtime_event_style_image_missing{
		size:          C.uint32_t(unsafe.Sizeof(C.mln_runtime_event_style_image_missing{})),
		image_id:      rawImageID,
		image_id_size: C.size_t(len(imageID)),
	}
	event := C.mln_runtime_event{
		payload_type: C.uint32_t(RuntimeEventPayloadStyleImageMissing),
		payload:      unsafe.Pointer(&payload),
		payload_size: C.size_t(unsafe.Sizeof(payload)),
	}
	return runtimeEventPayloadFromC(event)
}

func testTileActionEventPayloadFromC(operation uint32, tileID TileID, sourceID string) any {
	rawSourceID := C.CString(sourceID)
	defer C.free(unsafe.Pointer(rawSourceID))
	payload := C.mln_runtime_event_tile_action{
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
	event := C.mln_runtime_event{
		payload_type: C.uint32_t(RuntimeEventPayloadTileAction),
		payload:      unsafe.Pointer(&payload),
		payload_size: C.size_t(unsafe.Sizeof(payload)),
	}
	return runtimeEventPayloadFromC(event)
}
