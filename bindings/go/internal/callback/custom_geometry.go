package callback

/*
#cgo CFLAGS: -std=c2x
#include <stdlib.h>
#include <stdint.h>
#include "../cgo_shim.h"

extern void goMaplibreCustomGeometryFetchTile(void* user_data, mln_canonical_tile_id tile_id);
extern void goMaplibreCustomGeometryCancelTile(void* user_data, mln_canonical_tile_id tile_id);
*/
import "C"
import (
	"runtime/cgo"
	"sync"
	"unsafe"
)

// CanonicalTileID identifies one canonical tile for custom geometry callbacks.
type CanonicalTileID struct {
	Z uint32
	X uint32
	Y uint32
}

// CustomGeometryTileCallback is the internal shape for custom geometry tile callbacks.
type CustomGeometryTileCallback func(CanonicalTileID)

// CustomGeometrySourceOptions contains semantic custom geometry source options.
type CustomGeometrySourceOptions struct {
	FetchTile  CustomGeometryTileCallback
	CancelTile CustomGeometryTileCallback
	Fields     uint32
	MinZoom    float64
	MaxZoom    float64
	Tolerance  float64
	TileSize   uint32
	Buffer     uint32
	Clip       bool
	Wrap       bool
}

// CustomGeometrySourceState owns map/style-scoped custom geometry callback state.
type CustomGeometrySourceState struct {
	fetchTile  CustomGeometryTileCallback
	cancelTile CustomGeometryTileCallback
	handle     cgo.Handle
	once       sync.Once
	mu         sync.Mutex
	cond       *sync.Cond
	active     uint64
	released   bool
}

func newCustomGeometrySourceState(options CustomGeometrySourceOptions) *CustomGeometrySourceState {
	state := &CustomGeometrySourceState{fetchTile: options.FetchTile, cancelTile: options.CancelTile}
	state.cond = sync.NewCond(&state.mu)
	state.handle = cgo.NewHandle(state)
	return state
}

// AddCustomGeometrySource installs a custom geometry source callback descriptor.
func AddCustomGeometrySource(m unsafe.Pointer, sourceID string, options CustomGeometrySourceOptions) (*CustomGeometrySourceState, int32) {
	if options.FetchTile == nil {
		return nil, int32(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	state := newCustomGeometrySourceState(options)

	sourceData := C.CBytes([]byte(sourceID))
	defer C.free(sourceData)
	sourceView := C.mln_string_view{data: (*C.char)(sourceData), size: C.size_t(len(sourceID))}

	raw := C.mln_custom_geometry_source_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.fetch_tile = (C.mln_custom_geometry_source_tile_callback)(C.goMaplibreCustomGeometryFetchTile)
	if options.CancelTile != nil {
		raw.cancel_tile = (C.mln_custom_geometry_source_tile_callback)(C.goMaplibreCustomGeometryCancelTile)
	}
	raw.user_data = C.mln_go_handle_to_pointer(C.uintptr_t(state.handle))
	raw.min_zoom = C.double(options.MinZoom)
	raw.max_zoom = C.double(options.MaxZoom)
	raw.tolerance = C.double(options.Tolerance)
	raw.tile_size = C.uint32_t(options.TileSize)
	raw.buffer = C.uint32_t(options.Buffer)
	raw.clip = C.bool(options.Clip)
	raw.wrap = C.bool(options.Wrap)

	status := int32(C.mln_map_add_custom_geometry_source((*C.mln_map)(m), sourceView, &raw))
	if status != int32(C.MLN_STATUS_OK) {
		state.Release()
		return nil, status
	}
	return state, int32(C.MLN_STATUS_OK)
}

// Release frees callback state after native no longer references it.
func (state *CustomGeometrySourceState) Release() {
	if state == nil {
		return
	}
	state.once.Do(func() {
		state.mu.Lock()
		state.released = true
		for state.active > 0 {
			state.cond.Wait()
		}
		state.mu.Unlock()
		state.handle.Delete()
	})
}

func (state *CustomGeometrySourceState) beginInvoke() bool {
	if state == nil {
		return false
	}
	state.mu.Lock()
	defer state.mu.Unlock()
	if state.released {
		return false
	}
	state.active++
	return true
}

func (state *CustomGeometrySourceState) endInvoke() {
	state.mu.Lock()
	state.active--
	if state.active == 0 {
		state.cond.Broadcast()
	}
	state.mu.Unlock()
}

func (state *CustomGeometrySourceState) invokeFetch(tileID CanonicalTileID) {
	if state != nil && state.fetchTile != nil {
		state.fetchTile(tileID)
	}
}

func (state *CustomGeometrySourceState) invokeCancel(tileID CanonicalTileID) {
	if state != nil && state.cancelTile != nil {
		state.cancelTile(tileID)
	}
}

func canonicalTileIDFromC(tileID C.mln_canonical_tile_id) CanonicalTileID {
	return CanonicalTileID{Z: uint32(tileID.z), X: uint32(tileID.x), Y: uint32(tileID.y)}
}

func invokeCustomGeometryFetchForTest(callback CustomGeometryTileCallback) {
	state := newCustomGeometrySourceState(CustomGeometrySourceOptions{FetchTile: callback})
	defer state.Release()
	goMaplibreCustomGeometryFetchTile(
		C.mln_go_handle_to_pointer(C.uintptr_t(state.handle)),
		C.mln_canonical_tile_id{z: 1, x: 2, y: 3},
	)
}

func newCustomGeometrySourceStateForTest(callback CustomGeometryTileCallback) *CustomGeometrySourceState {
	return newCustomGeometrySourceState(CustomGeometrySourceOptions{FetchTile: callback})
}

func invokeCustomGeometryFetchStateForTest(state *CustomGeometrySourceState) {
	goMaplibreCustomGeometryFetchTile(
		C.mln_go_handle_to_pointer(C.uintptr_t(state.handle)),
		C.mln_canonical_tile_id{z: 1, x: 2, y: 3},
	)
}
