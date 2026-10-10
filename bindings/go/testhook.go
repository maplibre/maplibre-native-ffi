//go:build mlntest

package maplibre

// Test seams for the hand-written runtime. A _test.go file cannot use cgo, so
// the C values these tests synthesize are built here. The mlntest build tag
// keeps this file out of every build but the test suite's, which
// `mise run //bindings/go:test` selects.

/*
#include <stdlib.h>

#include "binding_callback.h"
#include "maplibre_native_c.h"

// Receives an image the way a native call does, so that cgo checks the Go
// pointers inside it.
static void mln_go_test_receive_image(const mln_premultiplied_rgba8_image* image) { (void)image; }
*/
import "C"

import (
	"fmt"
	"runtime/cgo"
	"unsafe"
)

// runtimeEventForTest is one synthesized event and its message, which the batch
// builder copies into a message arena.
type runtimeEventForTest struct {
	raw     C.mln_runtime_event
	message string
}

func newRuntimeEventForTest(eventType RuntimeEventType, sourceType RuntimeEventSourceType, source uint64) runtimeEventForTest {
	return runtimeEventForTest{raw: C.mln_runtime_event{
		_type:       C.uint32_t(eventType),
		source_type: C.uint32_t(sourceType),
		source:      C.uint64_t(source),
	}}
}

func (event runtimeEventForTest) withMessage(message string) runtimeEventForTest {
	event.message = message
	return event
}

// withPayload writes payload into the event's payload union, as native does
// for the arm payloadType names.
func withPayload[T any](event runtimeEventForTest, payloadType C.uint32_t, payload T) runtimeEventForTest {
	event.raw.payload_type = payloadType
	*(*T)(unsafe.Pointer(&event.raw.payload)) = payload
	return event
}

func (event runtimeEventForTest) withRenderFrame(payload RuntimeEventRenderFrame) runtimeEventForTest {
	return withPayload(event, C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME, C.mln_runtime_event_render_frame{
		mode:          C.uint32_t(payload.Mode),
		needs_repaint: C.bool(payload.NeedsRepaint),
	})
}

func (event runtimeEventForTest) withRenderMap(payload RuntimeEventRenderMap) runtimeEventForTest {
	return withPayload(event, C.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP, C.mln_runtime_event_render_map{
		mode: C.uint32_t(payload.Mode),
	})
}

func (event runtimeEventForTest) withTileAction(payload RuntimeEventTileAction) runtimeEventForTest {
	return withPayload(event, C.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION, C.mln_runtime_event_tile_action{
		operation: C.uint32_t(payload.Operation),
		tile_id: C.mln_tile_id{
			overscaled_z: C.uint32_t(payload.TileId.OverscaledZ),
			wrap:         C.int32_t(payload.TileId.Wrap),
			canonical_z:  C.uint32_t(payload.TileId.CanonicalZ),
			canonical_x:  C.uint32_t(payload.TileId.CanonicalX),
			canonical_y:  C.uint32_t(payload.TileId.CanonicalY),
		},
	})
}

func (event runtimeEventForTest) withCameraTransitionFinished(payload RuntimeEventCameraTransitionFinished) runtimeEventForTest {
	return withPayload(event, C.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED, C.mln_runtime_event_camera_transition_finished{
		transition_id: C.uint64_t(payload.TransitionId),
	})
}

func (event runtimeEventForTest) withOfflineRegionStatus(payload RuntimeEventOfflineRegionStatus) runtimeEventForTest {
	return withPayload(event, C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS, C.mln_runtime_event_offline_region_status{
		region_id: C.mln_offline_region_id(payload.RegionId),
		status: C.mln_offline_region_status{
			size:                 C.uint32_t(unsafe.Sizeof(C.mln_offline_region_status{})),
			download_state:       C.uint32_t(payload.Status.DownloadState),
			completed_tile_count: C.uint64_t(payload.Status.CompletedTileCount),
			complete:             C.bool(payload.Status.Complete),
		},
	})
}

func (event runtimeEventForTest) withOfflineRegionResponseError(payload RuntimeEventOfflineRegionResponseError) runtimeEventForTest {
	return withPayload(event, C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR, C.mln_runtime_event_offline_region_response_error{
		region_id: C.mln_offline_region_id(payload.RegionId),
		reason:    C.uint32_t(payload.Reason),
	})
}

func (event runtimeEventForTest) withOfflineRegionTileCountLimit(payload RuntimeEventOfflineRegionTileCountLimit) runtimeEventForTest {
	return withPayload(event, C.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT, C.mln_runtime_event_offline_region_tile_count_limit{
		region_id: C.mln_offline_region_id(payload.RegionId),
		limit:     C.uint64_t(payload.Limit),
	})
}

// withRawPayload fills the payload window with bytes under a payload type this
// binding version does not define.
func (event runtimeEventForTest) withRawPayload(payloadType RuntimeEventPayloadType, bytes []byte) runtimeEventForTest {
	event.raw.payload_type = C.uint32_t(payloadType)
	window := event.raw.payload[:]
	clear(window)
	copy(window, bytes)
	return event
}

// runtimeEventPayloadWindowSizeForTest is the size of the payload union this
// binding compiled against.
func runtimeEventPayloadWindowSizeForTest() int {
	return len(C.mln_runtime_event{}.payload)
}

// decodeEventsForTest lays events out stride bytes apart in C memory, the way a
// drain returns them, and runs the batch through the drain's copy path. A
// stride wider than the compiled event is what a later C API version sends.
func decodeEventsForTest(stride uintptr, events []runtimeEventForTest) []RuntimeEvent {
	stride = max(stride, unsafe.Sizeof(C.mln_runtime_event{}))
	storage := C.calloc(C.size_t(max(len(events), 1)), C.size_t(stride))
	defer C.free(storage)
	var arena []byte
	for index, event := range events {
		raw := event.raw
		if event.message != "" {
			raw.message_offset = C.uint64_t(len(arena))
			raw.message_size = C.uint32_t(len(event.message))
			arena = append(append(arena, event.message...), 0)
		}
		*(*C.mln_runtime_event)(unsafe.Add(storage, uintptr(index)*stride)) = raw
	}
	var messages unsafe.Pointer
	if len(arena) != 0 {
		messages = C.CBytes(arena)
		defer C.free(messages)
	}
	return copyEventBatchView(C.mln_event_batch_view{
		size:          C.uint32_t(unsafe.Sizeof(C.mln_event_batch_view{})),
		event_size:    C.uint32_t(stride),
		events:        (*C.mln_runtime_event)(storage),
		event_count:   C.size_t(len(events)),
		messages:      (*C.char)(messages),
		messages_size: C.size_t(len(arena)),
	}).Events
}

// int32CompletionForTest is a future whose bridge converts one int32 value,
// with deliver standing in for native's completion callback.
func int32CompletionForTest(convert func(int32) (int32, error)) (*Future[int32], func(status int32, value *int32)) {
	state := &futureState[int32]{ready: make(chan struct{})}
	bridge := &completionBridge[int32]{state: state, convert: func(result *C.mln_completion_result) (int32, error) {
		value, err := completionValue[C.int32_t](result)
		if err != nil {
			return 0, err
		}
		return convert(int32(value))
	}}
	deliver := func(status int32, value *int32) {
		result := C.mln_completion_result{status: C.int32_t(status)}
		if value != nil {
			native := C.int32_t(*value)
			result.value = unsafe.Pointer(&native)
			result.value_count = 1
		}
		bridge.complete(&result)
	}
	return &Future[int32]{state: state}, deliver
}

// rejectedSubmissionForTest starts a completion that native refuses, and
// returns the handle the binding passed as the completion's user data.
func rejectedSubmissionForTest() (cgo.Handle, error) {
	var submitted cgo.Handle
	_, err := startCompletion(func(completion *C.mln_completion, _ *C.mln_diagnostic) int32 {
		submitted = bindingHandleOf(completion.user_data)
		return int32(C.MLN_STATUS_INVALID_STATE)
	}, completionUnit)
	return submitted, err
}

// staleCallbackCellForTest returns user data that names a handle the binding
// already deleted, and the function that frees it.
func staleCallbackCellForTest() (unsafe.Pointer, func()) {
	handle := cgo.NewHandle(struct{}{})
	cell := bindingHandleCell(handle)
	handle.Delete()
	return cell, func() { C.binding_handle_free(cell) }
}

// failWithoutDiagnosticForTest runs a native call that fails and writes no
// diagnostic, through the pooled diagnostic every call shares.
func failWithoutDiagnosticForTest(status int32) error {
	return checkNative(func(*C.mln_diagnostic) int32 { return status })
}

// failWithFullDiagnosticForTest runs a native call that fails and fills the
// whole diagnostic buffer with no terminating NUL, as a truncated message can.
// It returns the buffer's capacity and the call's error.
func failWithFullDiagnosticForTest(status int32) (int, error) {
	err := checkNative(func(diagnostic *C.mln_diagnostic) int32 {
		for index := range diagnostic.message {
			diagnostic.message[index] = 'x'
		}
		return status
	})
	return int(C.MLN_DIAGNOSTIC_MESSAGE_CAPACITY), err
}

// passImageToCForTest hands a native image to C. The default cgo check fails
// the call when the image holds a Go pointer that nothing pinned.
func passImageToCForTest(image *C.mln_premultiplied_rgba8_image) (err error) {
	defer func() {
		if failure := recover(); failure != nil {
			err = fmt.Errorf("%v", failure)
		}
	}()
	C.mln_go_test_receive_image(image)
	return nil
}

// mapCreateFailingAfterAdoptionForTest creates a map through a completion
// whose conversion adopts the native map and then panics: a conversion that
// fails while it owns a value.
func mapCreateFailingAfterAdoptionForTest(receiver *RuntimeHandle) (*Future[*MapHandle], error) {
	return bindingCall(func() *Future[*MapHandle] {
		arena := &bindingArena{}
		defer arena.close()
		raw, done := receiver.bindingAcquire(false)
		defer done()
		options := (*C.mln_map_options)(arena.allocate(unsafe.Sizeof(C.mln_map_options{})))
		*options = nativeMapOptions(DefaultMapOptions(), arena)
		future, err := startCompletion(func(completion *C.mln_completion, diagnostic *C.mln_diagnostic) int32 {
			return int32(C.mln_map_create(C.mln_runtime(raw), options, completion, diagnostic))
		}, func(result *C.mln_completion_result) (*MapHandle, error) {
			value, err := completionValue[C.mln_map](result)
			if err != nil {
				return nil, err
			}
			adoptMapHandle(uint64(value), receiver)
			panic("conversion failed after adopting the map")
		})
		if err != nil {
			panic(bindingFailure{err})
		}
		return future
	})
}
