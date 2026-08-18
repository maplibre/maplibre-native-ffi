package maplibre

/*
#include <stdlib.h>

#include "internal/cgo_offline_shim.h"
*/
import "C"

import "unsafe"

// OfflineRegionID identifies a native offline region.
type OfflineRegionID int64

// OfflineRegionDownloadState controls native offline region downloading.
type OfflineRegionDownloadState uint32

const (
	OfflineRegionDownloadInactive OfflineRegionDownloadState = OfflineRegionDownloadState(C.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE)
	OfflineRegionDownloadActive   OfflineRegionDownloadState = OfflineRegionDownloadState(C.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE)
)

// OfflineRegionDefinition describes an offline region to create.
type OfflineRegionDefinition interface {
	offlineRegionDefinition()
}

// OfflineTilePyramidRegionDefinition describes a tile-pyramid offline region.
type OfflineTilePyramidRegionDefinition struct {
	StyleURL          string
	Bounds            LatLngBounds
	MinZoom           float64
	MaxZoom           float64
	PixelRatio        float32
	IncludeIdeographs bool
}

func (OfflineTilePyramidRegionDefinition) offlineRegionDefinition() {}

func (definition OfflineTilePyramidRegionDefinition) validate() error {
	return validateCStringArgument("offline region style URL", definition.StyleURL)
}

// OfflineGeometryRegionDefinition describes a geometry offline region.
type OfflineGeometryRegionDefinition struct {
	StyleURL          string
	Geometry          []byte
	MinZoom           float64
	MaxZoom           float64
	PixelRatio        float32
	IncludeIdeographs bool
}

func (OfflineGeometryRegionDefinition) offlineRegionDefinition() {}

func (definition OfflineGeometryRegionDefinition) validate() error {
	return validateCStringArgument("offline region style URL", definition.StyleURL)
}

// OfflineRegionInfo is a copied offline region snapshot.
type OfflineRegionInfo struct {
	ID                OfflineRegionID
	Definition        OfflineRegionDefinition
	RawDefinitionType uint32
	Metadata          []byte
}

// OfflineRegionStatus is a copied offline region status snapshot.
type OfflineRegionStatus struct {
	DownloadState                  OfflineRegionDownloadState
	RawDownloadState               uint32
	CompletedResourceCount         uint64
	CompletedResourceSize          uint64
	CompletedTileCount             uint64
	RequiredTileCount              uint64
	CompletedTileSize              uint64
	RequiredResourceCount          uint64
	RequiredResourceCountIsPrecise bool
	Complete                       bool
}

type cOfflineTilePyramidRegionDefinition struct {
	styleURL unsafe.Pointer
	raw      C.mln_offline_region_definition
}

func newCOfflineTilePyramidRegionDefinition(definition OfflineTilePyramidRegionDefinition) cOfflineTilePyramidRegionDefinition {
	styleURL := C.CString(definition.StyleURL)
	return cOfflineTilePyramidRegionDefinition{
		styleURL: unsafe.Pointer(styleURL),
		raw: C.mln_go_offline_tile_pyramid_region_definition(
			styleURL,
			cLatLngBounds(definition.Bounds),
			C.double(definition.MinZoom),
			C.double(definition.MaxZoom),
			C.float(definition.PixelRatio),
			C.bool(definition.IncludeIdeographs),
		),
	}
}

func (definition cOfflineTilePyramidRegionDefinition) free() {
	C.free(definition.styleURL)
}

type cOfflineGeometryRegionDefinition struct {
	styleURL unsafe.Pointer
	geometry cBufferView
	raw      C.mln_offline_region_definition
}

func newCOfflineGeometryRegionDefinition(definition OfflineGeometryRegionDefinition) (cOfflineGeometryRegionDefinition, error) {
	styleURL := C.CString(definition.StyleURL)
	geometry := newCBufferView(definition.Geometry)
	return cOfflineGeometryRegionDefinition{
		styleURL: unsafe.Pointer(styleURL),
		geometry: geometry,
		raw: C.mln_go_offline_geometry_region_definition(
			styleURL,
			geometry.raw(),
			C.double(definition.MinZoom),
			C.double(definition.MaxZoom),
			C.float(definition.PixelRatio),
			C.bool(definition.IncludeIdeographs),
		),
	}, nil
}

func (definition cOfflineGeometryRegionDefinition) free() {
	definition.geometry.free()
	C.free(definition.styleURL)
}

func (definition cOfflineGeometryRegionDefinition) copyDefinition() (OfflineRegionDefinition, error) {
	return offlineRegionDefinitionFromC(&definition.raw)
}

func metadataPointer(metadata []byte) *C.uint8_t {
	if len(metadata) == 0 {
		return nil
	}
	return (*C.uint8_t)(unsafe.Pointer(&metadata[0]))
}

func startRuntimeCompletion[T any](
	runtime *RuntimeHandle,
	start func(C.mln_runtime, *C.mln_completion) int32,
	convert func(*C.mln_completion_result) (T, error),
) (*Future[T], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer runtime.state.KeepAlive()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return start(C.mln_runtime(ptr), completion)
	}, convert)
}

func completionOfflineRegion(result *C.mln_completion_result) (OfflineRegionInfo, error) {
	raw, err := completionValue[C.mln_offline_region_info](result)
	if err != nil {
		return OfflineRegionInfo{}, err
	}
	return offlineRegionInfoFromC(raw)
}

func completionOptionalOfflineRegion(result *C.mln_completion_result) (*OfflineRegionInfo, error) {
	if result.value_count == 0 {
		return nil, nil
	}
	value, err := completionOfflineRegion(result)
	return &value, err
}

func completionOfflineRegions(result *C.mln_completion_result) ([]OfflineRegionInfo, error) {
	raw, err := completionSlice[C.mln_offline_region_info](result)
	if err != nil {
		return nil, err
	}
	regions := make([]OfflineRegionInfo, len(raw))
	for index := range raw {
		regions[index], err = offlineRegionInfoFromC(raw[index])
		if err != nil {
			return nil, err
		}
	}
	return regions, nil
}

func completionOfflineStatus(result *C.mln_completion_result) (OfflineRegionStatus, error) {
	raw, err := completionValue[C.mln_offline_region_status](result)
	return offlineRegionStatusFromC(raw), err
}

// CreateOfflineRegion starts creating an offline region.
func (runtime *RuntimeHandle) CreateOfflineRegion(definition OfflineRegionDefinition, metadata []byte) (*Future[OfflineRegionInfo], error) {
	switch region := definition.(type) {
	case OfflineTilePyramidRegionDefinition:
		if err := region.validate(); err != nil {
			return nil, err
		}
		return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
			rawDefinition := newCOfflineTilePyramidRegionDefinition(region)
			defer rawDefinition.free()
			return int32(C.mln_runtime_offline_region_create(
				handle,
				&rawDefinition.raw,
				metadataPointer(metadata),
				C.size_t(len(metadata)),
				out,
			))
		}, completionOfflineRegion)
	case OfflineGeometryRegionDefinition:
		if err := region.validate(); err != nil {
			return nil, err
		}
		rawDefinition, err := newCOfflineGeometryRegionDefinition(region)
		if err != nil {
			return nil, err
		}
		defer rawDefinition.free()
		return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
			return int32(C.mln_runtime_offline_region_create(
				handle,
				&rawDefinition.raw,
				metadataPointer(metadata),
				C.size_t(len(metadata)),
				out,
			))
		}, completionOfflineRegion)
	default:
		return nil, newBindingError(ErrInvalidArgument, "unsupported offline region definition")
	}
}

// OfflineRegion starts getting an offline region snapshot by ID.
func (runtime *RuntimeHandle) OfflineRegion(id OfflineRegionID) (*Future[*OfflineRegionInfo], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_region_get(handle, C.mln_offline_region_id(id), out))
	}, completionOptionalOfflineRegion)
}

// OfflineRegions starts listing offline regions.
func (runtime *RuntimeHandle) OfflineRegions() (*Future[[]OfflineRegionInfo], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_regions_list(handle, out))
	}, completionOfflineRegions)
}

// MergeOfflineRegionsDatabase starts merging offline regions from another
// database path.
func (runtime *RuntimeHandle) MergeOfflineRegionsDatabase(path string) (*Future[[]OfflineRegionInfo], error) {
	if err := validateCStringArgument("offline side database path", path); err != nil {
		return nil, err
	}
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		rawPath := C.CString(path)
		defer C.free(unsafe.Pointer(rawPath))
		return int32(C.mln_runtime_offline_regions_merge_database(handle, rawPath, out))
	}, completionOfflineRegions)
}

// UpdateOfflineRegionMetadata starts updating offline region metadata.
func (runtime *RuntimeHandle) UpdateOfflineRegionMetadata(id OfflineRegionID, metadata []byte) (*Future[OfflineRegionInfo], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_region_update_metadata(
			handle,
			C.mln_offline_region_id(id),
			metadataPointer(metadata),
			C.size_t(len(metadata)),
			out,
		))
	}, completionOfflineRegion)
}

// OfflineRegionStatus starts getting offline region status.
func (runtime *RuntimeHandle) OfflineRegionStatus(id OfflineRegionID) (*Future[OfflineRegionStatus], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_region_get_status(handle, C.mln_offline_region_id(id), out))
	}, completionOfflineStatus)
}

// SetOfflineRegionObserved starts setting offline event observation state.
func (runtime *RuntimeHandle) SetOfflineRegionObserved(id OfflineRegionID, observed bool) (*Future[struct{}], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_region_set_observed(handle, C.mln_offline_region_id(id), C.bool(observed), out))
	}, completionUnit)
}

// SetOfflineRegionDownloadState starts setting offline region download
// state.
func (runtime *RuntimeHandle) SetOfflineRegionDownloadState(id OfflineRegionID, state OfflineRegionDownloadState) (*Future[struct{}], error) {
	raw, err := rawOfflineRegionDownloadState(state)
	if err != nil {
		return nil, err
	}
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_region_set_download_state(handle, C.mln_offline_region_id(id), C.uint32_t(raw), out))
	}, completionUnit)
}

// InvalidateOfflineRegion starts invalidating cached resources for a
// region.
func (runtime *RuntimeHandle) InvalidateOfflineRegion(id OfflineRegionID) (*Future[struct{}], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_region_invalidate(handle, C.mln_offline_region_id(id), out))
	}, completionUnit)
}

// DeleteOfflineRegion starts deleting an offline region.
func (runtime *RuntimeHandle) DeleteOfflineRegion(id OfflineRegionID) (*Future[struct{}], error) {
	return startRuntimeCompletion(runtime, func(handle C.mln_runtime, out *C.mln_completion) int32 {
		return int32(C.mln_runtime_offline_region_delete(handle, C.mln_offline_region_id(id), out))
	}, completionUnit)
}

func offlineRegionInfoFromC(info C.mln_offline_region_info) (OfflineRegionInfo, error) {
	definitionType := uint32(C.mln_go_offline_region_info_definition_type(&info))
	metadata, ok := goByteSlice(unsafe.Pointer(info.metadata), info.metadata_size)
	if !ok {
		return OfflineRegionInfo{}, newBindingError(ErrNative, "offline region metadata buffer is invalid")
	}
	copied := OfflineRegionInfo{
		ID:                OfflineRegionID(info.id),
		RawDefinitionType: definitionType,
		Metadata:          metadata,
	}
	definition, err := offlineRegionDefinitionFromC(&info.definition)
	if err != nil {
		return OfflineRegionInfo{}, err
	}
	copied.Definition = definition
	return copied, nil
}

func offlineRegionDefinitionFromC(definition *C.mln_offline_region_definition) (OfflineRegionDefinition, error) {
	definitionType := uint32(C.mln_go_offline_region_definition_type(definition))
	switch definitionType {
	case uint32(C.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID):
		tile := C.mln_go_offline_region_definition_tile_pyramid(definition)
		return OfflineTilePyramidRegionDefinition{
			StyleURL:          C.GoString(tile.style_url),
			Bounds:            goLatLngBounds(tile.bounds),
			MinZoom:           float64(tile.min_zoom),
			MaxZoom:           float64(tile.max_zoom),
			PixelRatio:        float32(tile.pixel_ratio),
			IncludeIdeographs: bool(tile.include_ideographs),
		}, nil
	case uint32(C.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY):
		geometry := C.mln_go_offline_region_definition_geometry(definition)
		copiedGeometry, ok := goByteSlice(geometry.geometry.data, geometry.geometry.size)
		if !ok {
			return nil, newBindingError(ErrNative, "offline geometry buffer is invalid")
		}
		return OfflineGeometryRegionDefinition{
			StyleURL:          C.GoString(geometry.style_url),
			Geometry:          copiedGeometry,
			MinZoom:           float64(geometry.min_zoom),
			MaxZoom:           float64(geometry.max_zoom),
			PixelRatio:        float32(geometry.pixel_ratio),
			IncludeIdeographs: bool(geometry.include_ideographs),
		}, nil
	default:
		return nil, nil
	}
}

func rawOfflineRegionDownloadState(state OfflineRegionDownloadState) (uint32, error) {
	switch state {
	case OfflineRegionDownloadInactive, OfflineRegionDownloadActive:
		return uint32(state), nil
	default:
		return 0, newBindingError(ErrInvalidArgument, "unknown offline region download state cannot be set")
	}
}
