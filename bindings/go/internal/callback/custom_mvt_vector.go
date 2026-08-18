package callback

/*
#cgo CFLAGS: -std=c2x
#include <stdlib.h>
#include <stdint.h>
#include "../cgo_shim.h"

extern void goMaplibreCustomMVTVectorFetchTile(void* user_data, mln_canonical_tile_id tile_id);
extern void goMaplibreCustomMVTVectorCancelTile(void* user_data, mln_canonical_tile_id tile_id);
extern void goMaplibreCustomMVTVectorReleaseUserData(void* user_data);
*/
import "C"

import (
	"runtime/cgo"
	"sync"
	"sync/atomic"
)

// liveCustomMVTVectorSources counts the callback states this package holds a
// cgo handle for. CustomMVTVectorSourceLiveCountForTest reads it.
var liveCustomMVTVectorSources atomic.Int64

// CustomMVTVectorSourceLiveCountForTest reports how many custom MVT vector
// callback states are still alive, which is how a test observes that the C
// API's release callback freed one.
func CustomMVTVectorSourceLiveCountForTest() int64 {
	return liveCustomMVTVectorSources.Load()
}

// CustomMVTVectorTileCallback is the internal shape for custom MVT vector tile callbacks.
type CustomMVTVectorTileCallback func(CanonicalTileID)

// CustomMVTVectorSourceOptions contains semantic custom MVT vector source options.
type CustomMVTVectorSourceOptions struct {
	FetchTile  CustomMVTVectorTileCallback
	CancelTile CustomMVTVectorTileCallback
	Fields     uint32
	MinZoom    float64
	MaxZoom    float64
}

// CustomMVTVectorSourceState owns map/style-scoped custom MVT vector callback state.
type CustomMVTVectorSourceState struct {
	fetchTile  CustomMVTVectorTileCallback
	cancelTile CustomMVTVectorTileCallback
	handle     cgo.Handle
	once       sync.Once
	mu         sync.Mutex
	cond       *sync.Cond
	active     uint64
	released   bool
}

func newCustomMVTVectorSourceState(options CustomMVTVectorSourceOptions) *CustomMVTVectorSourceState {
	state := &CustomMVTVectorSourceState{fetchTile: options.FetchTile, cancelTile: options.CancelTile}
	state.cond = sync.NewCond(&state.mu)
	state.handle = cgo.NewHandle(state)
	liveCustomMVTVectorSources.Add(1)
	return state
}

// AddCustomMVTVectorSource installs a custom MVT vector source callback descriptor.
// The C API owns the state from a successful add onwards and releases it through
// the release callback, so a caller keeps nothing.
func AddCustomMVTVectorSource(m uint64, sourceID string, options CustomMVTVectorSourceOptions) int32 {
	if options.FetchTile == nil {
		return int32(C.MLN_STATUS_INVALID_ARGUMENT)
	}
	state := newCustomMVTVectorSourceState(options)

	sourceData := C.CBytes([]byte(sourceID))
	defer C.free(sourceData)
	sourceView := C.mln_buffer_view{data: sourceData, size: C.size_t(len(sourceID))}

	raw := C.mln_custom_mvt_vector_source_options_default()
	raw.fields = C.uint32_t(options.Fields)
	raw.fetch_tile = C.mln_custom_mvt_vector_source_tile_callback(C.goMaplibreCustomMVTVectorFetchTile)
	if options.CancelTile != nil {
		raw.cancel_tile = C.mln_custom_mvt_vector_source_tile_callback(C.goMaplibreCustomMVTVectorCancelTile)
	}
	raw.user_data = C.mln_go_handle_to_pointer(C.uintptr_t(state.handle))
	raw.release_user_data = C.mln_custom_mvt_vector_source_release_callback(C.goMaplibreCustomMVTVectorReleaseUserData)
	raw.min_zoom = C.double(options.MinZoom)
	raw.max_zoom = C.double(options.MaxZoom)

	status := int32(C.mln_map_add_custom_mvt_vector_source(C.mln_map(m), sourceView, &raw))
	if status != int32(C.MLN_STATUS_OK) {
		// A failed add owes no release callback, so this call frees the state.
		state.release()
		return status
	}
	return int32(C.MLN_STATUS_OK)
}

func (state *CustomMVTVectorSourceState) release() {
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
		liveCustomMVTVectorSources.Add(-1)
	})
}

func (state *CustomMVTVectorSourceState) beginInvoke() bool {
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

func (state *CustomMVTVectorSourceState) endInvoke() {
	state.mu.Lock()
	state.active--
	if state.active == 0 {
		state.cond.Broadcast()
	}
	state.mu.Unlock()
}

func (state *CustomMVTVectorSourceState) invokeFetch(tileID CanonicalTileID) {
	if state != nil && state.fetchTile != nil {
		state.fetchTile(tileID)
	}
}

func (state *CustomMVTVectorSourceState) invokeCancel(tileID CanonicalTileID) {
	if state != nil && state.cancelTile != nil {
		state.cancelTile(tileID)
	}
}

func invokeCustomMVTVectorFetchForTest(callback CustomMVTVectorTileCallback) {
	state := newCustomMVTVectorSourceState(CustomMVTVectorSourceOptions{FetchTile: callback})
	defer state.release()
	goMaplibreCustomMVTVectorFetchTile(
		C.mln_go_handle_to_pointer(C.uintptr_t(state.handle)),
		C.mln_canonical_tile_id{z: 1, x: 2, y: 3},
	)
}

func newCustomMVTVectorSourceStateForTest(callback CustomMVTVectorTileCallback) *CustomMVTVectorSourceState {
	return newCustomMVTVectorSourceState(CustomMVTVectorSourceOptions{FetchTile: callback})
}

func invokeCustomMVTVectorFetchStateForTest(state *CustomMVTVectorSourceState) {
	goMaplibreCustomMVTVectorFetchTile(
		C.mln_go_handle_to_pointer(C.uintptr_t(state.handle)),
		C.mln_canonical_tile_id{z: 1, x: 2, y: 3},
	)
}
