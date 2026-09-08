package callback

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"runtime/cgo"
	"unsafe"
)

//export goMaplibreCustomGeometryFetchTile
func goMaplibreCustomGeometryFetchTile(userData unsafe.Pointer, tileID C.mln_canonical_tile_id) {
	defer func() { _ = recover() }()

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*CustomGeometrySourceState)
	if !ok || state == nil {
		return
	}
	if !state.beginInvoke() {
		return
	}
	defer state.endInvoke()
	state.invokeFetch(canonicalTileIDFromC(tileID))
}

// goMaplibreCustomGeometryReleaseUserData frees one source's callback state. The
// C API invokes it once on native map execution: on explicit removal, when a
// style load drops the source, or when the map closes.
//
//export goMaplibreCustomGeometryReleaseUserData
func goMaplibreCustomGeometryReleaseUserData(userData unsafe.Pointer) {
	defer func() { _ = recover() }()

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*CustomGeometrySourceState)
	if !ok || state == nil {
		return
	}
	state.release()
}

//export goMaplibreCustomGeometryCancelTile
func goMaplibreCustomGeometryCancelTile(userData unsafe.Pointer, tileID C.mln_canonical_tile_id) {
	defer func() { _ = recover() }()

	handle := cgo.Handle(uintptr(userData))
	state, ok := handle.Value().(*CustomGeometrySourceState)
	if !ok || state == nil {
		return
	}
	if !state.beginInvoke() {
		return
	}
	defer state.endInvoke()
	state.invokeCancel(canonicalTileIDFromC(tileID))
}
