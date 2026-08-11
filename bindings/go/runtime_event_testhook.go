package maplibre

/*
#include <stdlib.h>

#include "internal/cgo_runtime_shim.h"
*/
import "C"

import "unsafe"

// Test seams for the event decoder. Tests cannot use cgo, so the payload union
// writes and the synthesized batch layout live here.

// runtimeEventForTest is one synthesized event and its message, which the batch
// builder copies into a message arena.
type runtimeEventForTest struct {
	raw     C.mln_runtime_event
	message string
}

func newRuntimeEventForTest(eventType RuntimeEventType, sourceType RuntimeEventSourceType, source uint64, code int32) runtimeEventForTest {
	return runtimeEventForTest{raw: C.mln_runtime_event{
		_type:       C.uint32_t(eventType),
		source_type: C.uint32_t(sourceType),
		source:      C.uint64_t(source),
		code:        C.int32_t(code),
	}}
}

func (event runtimeEventForTest) withMessage(message string) runtimeEventForTest {
	event.message = message
	return event
}

func (event runtimeEventForTest) withRenderFrame(payload RuntimeEventRenderFramePayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_render_frame(event.raw, C.mln_runtime_event_render_frame{
		mode:              C.uint32_t(payload.RawMode),
		needs_repaint:     C.bool(payload.NeedsRepaint),
		placement_changed: C.bool(payload.PlacementChanged),
		stats: C.mln_rendering_stats{
			encoding_time:         C.double(payload.Stats.EncodingTime),
			rendering_time:        C.double(payload.Stats.RenderingTime),
			frame_count:           C.int64_t(payload.Stats.FrameCount),
			draw_call_count:       C.int64_t(payload.Stats.DrawCallCount),
			total_draw_call_count: C.int64_t(payload.Stats.TotalDrawCallCount),
		},
	})
	return event
}

func (event runtimeEventForTest) withRenderMap(payload RuntimeEventRenderMapPayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_render_map(event.raw, C.mln_runtime_event_render_map{
		mode: C.uint32_t(payload.RawMode),
	})
	return event
}

func (event runtimeEventForTest) withTileAction(payload RuntimeEventTileActionPayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_tile_action(event.raw, C.mln_runtime_event_tile_action{
		operation: C.uint32_t(payload.RawOperation),
		tile_id: C.mln_tile_id{
			overscaled_z: C.uint32_t(payload.TileID.OverscaledZ),
			wrap:         C.int32_t(payload.TileID.Wrap),
			canonical_z:  C.uint32_t(payload.TileID.CanonicalZ),
			canonical_x:  C.uint32_t(payload.TileID.CanonicalX),
			canonical_y:  C.uint32_t(payload.TileID.CanonicalY),
		},
	})
	return event
}

func (event runtimeEventForTest) withCameraTransitionFinished(payload RuntimeEventCameraTransitionFinishedPayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_camera_transition_finished(event.raw, C.mln_runtime_event_camera_transition_finished{
		transition_id: C.uint64_t(payload.TransitionID),
	})
	return event
}

func (event runtimeEventForTest) withOfflineRegionStatus(payload RuntimeEventOfflineRegionStatusPayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_offline_region_status(event.raw, C.mln_runtime_event_offline_region_status{
		region_id: C.mln_offline_region_id(payload.RegionID),
		status: C.mln_offline_region_status{
			size:                               C.uint32_t(unsafe.Sizeof(C.mln_offline_region_status{})),
			download_state:                     C.uint32_t(payload.Status.RawDownloadState),
			completed_resource_count:           C.uint64_t(payload.Status.CompletedResourceCount),
			completed_resource_size:            C.uint64_t(payload.Status.CompletedResourceSize),
			completed_tile_count:               C.uint64_t(payload.Status.CompletedTileCount),
			required_tile_count:                C.uint64_t(payload.Status.RequiredTileCount),
			completed_tile_size:                C.uint64_t(payload.Status.CompletedTileSize),
			required_resource_count:            C.uint64_t(payload.Status.RequiredResourceCount),
			required_resource_count_is_precise: C.bool(payload.Status.RequiredResourceCountIsPrecise),
			complete:                           C.bool(payload.Status.Complete),
		},
	})
	return event
}

func (event runtimeEventForTest) withOfflineRegionResponseError(payload RuntimeEventOfflineRegionResponseErrorPayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_offline_region_response_error(event.raw, C.mln_runtime_event_offline_region_response_error{
		region_id: C.mln_offline_region_id(payload.RegionID),
		reason:    C.uint32_t(payload.RawReason),
	})
	return event
}

func (event runtimeEventForTest) withOfflineRegionTileCountLimit(payload RuntimeEventOfflineRegionTileCountLimitPayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_offline_region_tile_count_limit(event.raw, C.mln_runtime_event_offline_region_tile_count_limit{
		region_id: C.mln_offline_region_id(payload.RegionID),
		limit:     C.uint64_t(payload.Limit),
	})
	return event
}

func (event runtimeEventForTest) withOfflineOperationCompleted(payload RuntimeEventOfflineOperationCompletedPayload) runtimeEventForTest {
	event.raw = C.mln_go_runtime_event_with_offline_operation_completed(event.raw, C.mln_runtime_event_offline_operation_completed{
		operation_id:   C.mln_offline_operation_id(payload.OperationID),
		operation_kind: C.uint32_t(payload.OperationKind),
		result_kind:    C.uint32_t(payload.ResultKind),
		result_status:  C.int32_t(payload.ResultStatus),
		found:          C.bool(payload.Found),
	})
	return event
}

// withRawPayload writes opaque bytes into this event's payload window, which is
// how a test synthesizes a payload type this binding version does not define.
func (event runtimeEventForTest) withRawPayload(payloadType RuntimeEventPayloadType, bytes []byte) runtimeEventForTest {
	event.raw.payload_type = C.uint32_t(payloadType)
	window := event.raw.payload[:]
	for index := range window {
		window[index] = 0
	}
	copy(window, bytes)
	return event
}

// runtimeEventBatchForTest owns the C memory of one synthesized batch, so the
// decoder walks native storage exactly as it does after a drain.
type runtimeEventBatchForTest struct {
	events   unsafe.Pointer
	messages unsafe.Pointer
	stride   uintptr
	raw      C.mln_runtime_event_batch
}

// newRuntimeEventBatchForTest lays events out stride bytes apart. A stride wider
// than this binding's compiled event size is what proves the decoder reads the
// batch's own stride.
func newRuntimeEventBatchForTest(stride uintptr, remainingCount uint64, events []runtimeEventForTest) *runtimeEventBatchForTest {
	if stride < runtimeEventSizeForTest() {
		stride = runtimeEventSizeForTest()
	}
	batch := &runtimeEventBatchForTest{stride: stride}
	var arena []byte
	if len(events) > 0 {
		batch.events = C.calloc(C.size_t(len(events)), C.size_t(stride))
	}
	for index, event := range events {
		raw := event.raw
		if len(event.message) > 0 {
			raw.message_offset = C.uint32_t(len(arena))
			raw.message_size = C.uint32_t(len(event.message))
			arena = append(arena, event.message...)
			arena = append(arena, 0)
		}
		*(*C.mln_runtime_event)(unsafe.Add(batch.events, uintptr(index)*stride)) = raw
	}
	if len(arena) > 0 {
		batch.messages = C.CBytes(arena)
	}
	batch.raw = C.mln_runtime_event_batch{
		size:            C.uint32_t(unsafe.Sizeof(C.mln_runtime_event_batch{})),
		event_size:      C.uint32_t(stride),
		events:          (*C.mln_runtime_event)(batch.events),
		event_count:     C.size_t(len(events)),
		messages:        (*C.char)(batch.messages),
		messages_size:   C.size_t(len(arena)),
		remaining_count: C.size_t(remainingCount),
	}
	return batch
}

func (batch *runtimeEventBatchForTest) free() {
	if batch.events != nil {
		C.free(batch.events)
		batch.events = nil
	}
	if batch.messages != nil {
		C.free(batch.messages)
		batch.messages = nil
	}
}

// payloadWindow aliases one synthesized event's payload bytes, so a test can
// mutate the source a decoded copy came from.
func (batch *runtimeEventBatchForTest) payloadWindow(index int) []byte {
	offset := uintptr(index)*batch.stride + runtimeEventPayloadOffset
	return unsafe.Slice((*byte)(unsafe.Add(batch.events, offset)), batch.stride-runtimeEventPayloadOffset)
}

// decodeForTest runs the batch through the same copy path DrainEvents uses.
func (runtime *RuntimeHandle) decodeForTest(batch *runtimeEventBatchForTest) RuntimeEventBatch {
	return runtime.copyEventBatch(batch.raw)
}

// runtimeEventSizeForTest is this binding's compiled event size, which a layout
// test compares against the stride the C API reports.
func runtimeEventSizeForTest() uintptr {
	return unsafe.Sizeof(C.mln_runtime_event{})
}

func runtimeEventPayloadWindowSizeForTest() uintptr {
	return runtimeEventSizeForTest() - runtimeEventPayloadOffset
}

// nativeRuntimeEventStrideForTest drains this runtime and reports the event
// stride the C API filled in.
func nativeRuntimeEventStrideForTest(runtime *RuntimeHandle) (uintptr, error) {
	ptr, release, err := runtime.ptr()
	if err != nil {
		return 0, err
	}
	defer release()
	defer runtime.state.KeepAlive()

	batch, err := drainRawEvents(ptr, 0)
	if err != nil {
		return 0, err
	}
	return uintptr(batch.event_size), nil
}

func offlineRegionStatusForTest(downloadState uint32) OfflineRegionStatus {
	raw := C.mln_offline_region_status{
		size:           C.uint32_t(unsafe.Sizeof(C.mln_offline_region_status{})),
		download_state: C.uint32_t(downloadState),
	}
	return offlineRegionStatusFromC(raw)
}
