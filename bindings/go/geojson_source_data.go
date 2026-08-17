package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

import (
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

type nativeGeoJSONSourceData uint64

// GeoJSONSourceDataHandle owns prepared GeoJSON source data: one UTF-8 GeoJSON
// document parsed and tiled (or clustered) into the index a GeoJSON source
// consumes, with the source options baked in. The prepared data is immutable,
// so a live handle is safe to share across goroutines.
type GeoJSONSourceDataHandle struct {
	state *handle.State[nativeGeoJSONSourceData]
}

// NewGeoJSONSourceData prepares GeoJSON source data for installation on a map.
// data holds one complete UTF-8 GeoJSON document and options may be nil for
// defaults; both are copied into the prepared data before the call returns.
// Preparation touches no runtime or map and is callable from any goroutine, so
// the expensive parse and tiling can run concurrently with map work.
//
// AddGeoJSONSourceData and SetGeoJSONSourceData borrow the handle, so one
// prepared value may be installed on any number of sources and closed at any
// time afterward; closing it never invalidates a source it was installed on.
func NewGeoJSONSourceData(data []byte, options *StyleGeoJSONSourceOptions) (*GeoJSONSourceDataHandle, error) {
	rawData := newCBufferView(data)
	defer rawData.free()
	rawOptions, err := newCStyleGeoJSONSourceOptions(options)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	defer rawOptions.free()

	var prepared nativeGeoJSONSourceData
	if err := checkNative(func() int32 {
		var raw C.mln_geojson_source_data = C.MLN_HANDLE_NULL
		status := int32(C.mln_geojson_source_data_create(rawData.raw(), rawOptions.ptr(), &raw))
		if status == int32(C.MLN_STATUS_OK) {
			prepared = nativeGeoJSONSourceData(raw)
		}
		return status
	}); err != nil {
		return nil, err
	}
	state, err := handle.New(prepared, "GeoJSONSourceDataHandle")
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &GeoJSONSourceDataHandle{state: state}, nil
}

func (data *GeoJSONSourceDataHandle) ptr() (nativeGeoJSONSourceData, func(), error) {
	if data == nil || data.state == nil {
		return 0, nil, newBindingError(ErrInvalidArgument, "GeoJSONSourceDataHandle is nil")
	}
	borrow, live := data.state.Borrow()
	if !live {
		return 0, nil, newBindingError(ErrInvalidArgument, "GeoJSONSourceDataHandle is closed")
	}
	return borrow.Handle(), borrow.Release, nil
}

// Close releases this prepared data. Close is callable from any goroutine, and
// a successful close makes later calls no-ops. Sources the data was installed
// on keep their own reference, so closing never invalidates a source.
func (data *GeoJSONSourceDataHandle) Close() error {
	if data == nil || data.state == nil {
		return newBindingError(ErrInvalidArgument, "GeoJSONSourceDataHandle is nil")
	}
	return checkNative(func() int32 {
		return data.state.Close(func(native nativeGeoJSONSourceData) int32 {
			C.mln_geojson_source_data_destroy(C.mln_geojson_source_data(native))
			return int32(C.MLN_STATUS_OK)
		})
	})
}
