package callback

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"runtime/cgo"
	"unsafe"
)

//export goMaplibreCustomMVTVectorFetchTile
func goMaplibreCustomMVTVectorFetchTile(userData unsafe.Pointer, tileID C.mln_canonical_tile_id) {
	defer func() { _ = recover() }()

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*CustomMVTVectorSourceState)
	if !ok || state == nil {
		return
	}
	if !state.beginInvoke() {
		return
	}
	defer state.endInvoke()
	state.invokeFetch(canonicalTileIDFromC(tileID))
}

// goMaplibreCustomMVTVectorReleaseUserData frees one source's callback state.
// The C API invokes it once, on the map owner thread, when it stops
// referencing the state: on explicit removal, when a style load drops the
// source, or when the map is destroyed.
//
//export goMaplibreCustomMVTVectorReleaseUserData
func goMaplibreCustomMVTVectorReleaseUserData(userData unsafe.Pointer) {
	defer func() { _ = recover() }()

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*CustomMVTVectorSourceState)
	if !ok || state == nil {
		return
	}
	state.release()
}

//export goMaplibreCustomMVTVectorCancelTile
func goMaplibreCustomMVTVectorCancelTile(userData unsafe.Pointer, tileID C.mln_canonical_tile_id) {
	defer func() { _ = recover() }()

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*CustomMVTVectorSourceState)
	if !ok || state == nil {
		return
	}
	if !state.beginInvoke() {
		return
	}
	defer state.endInvoke()
	state.invokeCancel(canonicalTileIDFromC(tileID))
}
