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

// StartCreateOfflineRegion starts creating an offline region.
func (runtime *RuntimeHandle) StartCreateOfflineRegion(definition OfflineRegionDefinition, metadata []byte) (*OperationHandle[OfflineRegionInfo], error) {
	switch region := definition.(type) {
	case OfflineTilePyramidRegionDefinition:
		if err := region.validate(); err != nil {
			return nil, err
		}
		return startOperation[OfflineRegionInfo](runtime, operationRegionCreate, operationResultRegion, func(handle nativeRuntime, out *C.mln_operation) int32 {
			rawDefinition := newCOfflineTilePyramidRegionDefinition(region)
			defer rawDefinition.free()
			return int32(C.mln_runtime_offline_region_create_start(
				C.mln_runtime(handle),
				&rawDefinition.raw,
				metadataPointer(metadata),
				C.size_t(len(metadata)),
				out,
			))
		})
	case OfflineGeometryRegionDefinition:
		if err := region.validate(); err != nil {
			return nil, err
		}
		rawDefinition, err := newCOfflineGeometryRegionDefinition(region)
		if err != nil {
			return nil, err
		}
		defer rawDefinition.free()
		return startOperation[OfflineRegionInfo](runtime, operationRegionCreate, operationResultRegion, func(handle nativeRuntime, out *C.mln_operation) int32 {
			return int32(C.mln_runtime_offline_region_create_start(
				C.mln_runtime(handle),
				&rawDefinition.raw,
				metadataPointer(metadata),
				C.size_t(len(metadata)),
				out,
			))
		})
	default:
		return nil, newBindingError(ErrInvalidArgument, "unsupported offline region definition")
	}
}

// StartOfflineRegion starts getting an offline region snapshot by ID.
func (runtime *RuntimeHandle) StartOfflineRegion(id OfflineRegionID) (*OperationHandle[*OfflineRegionInfo], error) {
	return startOperation[*OfflineRegionInfo](runtime, operationRegionGet, operationResultOptionalRegion, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_region_get_start(C.mln_runtime(handle), C.mln_offline_region_id(id), out))
	})
}

// StartOfflineRegions starts listing offline regions.
func (runtime *RuntimeHandle) StartOfflineRegions() (*OperationHandle[[]OfflineRegionInfo], error) {
	return startOperation[[]OfflineRegionInfo](runtime, operationRegionsList, operationResultRegionList, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_regions_list_start(C.mln_runtime(handle), out))
	})
}

// StartMergeOfflineRegionsDatabase starts merging offline regions from another
// database path.
func (runtime *RuntimeHandle) StartMergeOfflineRegionsDatabase(path string) (*OperationHandle[[]OfflineRegionInfo], error) {
	if err := validateCStringArgument("offline side database path", path); err != nil {
		return nil, err
	}
	return startOperation[[]OfflineRegionInfo](runtime, operationRegionsMergeDatabase, operationResultRegionList, func(handle nativeRuntime, out *C.mln_operation) int32 {
		rawPath := C.CString(path)
		defer C.free(unsafe.Pointer(rawPath))
		return int32(C.mln_runtime_offline_regions_merge_database_start(C.mln_runtime(handle), rawPath, out))
	})
}

// StartUpdateOfflineRegionMetadata starts updating offline region metadata.
func (runtime *RuntimeHandle) StartUpdateOfflineRegionMetadata(id OfflineRegionID, metadata []byte) (*OperationHandle[OfflineRegionInfo], error) {
	return startOperation[OfflineRegionInfo](runtime, operationRegionUpdateMetadata, operationResultRegion, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_region_update_metadata_start(
			C.mln_runtime(handle),
			C.mln_offline_region_id(id),
			metadataPointer(metadata),
			C.size_t(len(metadata)),
			out,
		))
	})
}

// StartOfflineRegionStatus starts getting offline region status.
func (runtime *RuntimeHandle) StartOfflineRegionStatus(id OfflineRegionID) (*OperationHandle[OfflineRegionStatus], error) {
	return startOperation[OfflineRegionStatus](runtime, operationRegionGetStatus, operationResultRegionStatus, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_region_get_status_start(C.mln_runtime(handle), C.mln_offline_region_id(id), out))
	})
}

// StartSetOfflineRegionObserved starts setting offline event observation state.
func (runtime *RuntimeHandle) StartSetOfflineRegionObserved(id OfflineRegionID, observed bool) (*OperationHandle[struct{}], error) {
	operation, err := startOperation[struct{}](runtime, operationRegionSetObserved, operationResultNone, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_region_set_observed_start(C.mln_runtime(handle), C.mln_offline_region_id(id), C.bool(observed), out))
	})
	return operation, err
}

// StartSetOfflineRegionDownloadState starts setting offline region download
// state.
func (runtime *RuntimeHandle) StartSetOfflineRegionDownloadState(id OfflineRegionID, state OfflineRegionDownloadState) (*OperationHandle[struct{}], error) {
	raw, err := rawOfflineRegionDownloadState(state)
	if err != nil {
		return nil, err
	}
	return startOperation[struct{}](runtime, operationRegionSetDownloadState, operationResultNone, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_region_set_download_state_start(C.mln_runtime(handle), C.mln_offline_region_id(id), C.uint32_t(raw), out))
	})
}

// StartInvalidateOfflineRegion starts invalidating cached resources for a
// region.
func (runtime *RuntimeHandle) StartInvalidateOfflineRegion(id OfflineRegionID) (*OperationHandle[struct{}], error) {
	return startOperation[struct{}](runtime, operationRegionInvalidate, operationResultNone, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_region_invalidate_start(C.mln_runtime(handle), C.mln_offline_region_id(id), out))
	})
}

// StartDeleteOfflineRegion starts deleting an offline region.
func (runtime *RuntimeHandle) StartDeleteOfflineRegion(id OfflineRegionID) (*OperationHandle[struct{}], error) {
	operation, err := startOperation[struct{}](runtime, operationRegionDelete, operationResultNone, func(handle nativeRuntime, out *C.mln_operation) int32 {
		return int32(C.mln_runtime_offline_region_delete_start(C.mln_runtime(handle), C.mln_offline_region_id(id), out))
	})
	return operation, err
}

// Take consumes a completed operation result and copies it into Go values. If
// native reports that the result is not ready, the operation remains available
// for retry. A successful take consumes the operation.
func (operation *OperationHandle[T]) Take() (T, error) {
	var zero T
	id, kind, resultKind, err := operation.beginResultUse()
	if err != nil {
		return zero, err
	}
	var result T
	var transferred bool
	if operation.takeResult != nil {
		result, transferred, err = operation.takeResult(id)
	} else {
		result, transferred, err = takeOfflineOperationResult[T](C.mln_operation(id), kind, resultKind)
	}
	operation.endResultUse(transferred)
	if err != nil {
		return zero, err
	}
	return result, nil
}

func takeOfflineOperationResult[T any](id C.mln_operation, kind operationKind, resultKind operationResultKind) (T, bool, error) {
	var zero T
	switch resultKind {
	case operationResultNone:
		return zero, false, newBindingError(ErrInvalidState, "offline operation does not produce a take result; finish it after completion")
	case operationResultRegion:
		var snapshot C.mln_offline_region_snapshot
		var err error
		switch kind {
		case operationRegionCreate:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_region_create_take_result(id, &snapshot))
			})
		case operationRegionUpdateMetadata:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_region_update_metadata_take_result(id, &snapshot))
			})
		default:
			return zero, false, newBindingError(ErrInvalidState, "offline operation result kind mismatch")
		}
		if err != nil {
			return zero, false, err
		}
		info, err := offlineRegionSnapshotInfo(snapshot)
		if err != nil {
			return zero, true, err
		}
		if result, ok := any(info).(T); ok {
			return result, true, nil
		}
		return zero, true, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case operationResultOptionalRegion:
		var snapshot C.mln_offline_region_snapshot
		var found C.bool
		if err := checkNative(func() int32 {
			return int32(C.mln_runtime_offline_region_get_take_result(id, &snapshot, &found))
		}); err != nil {
			return zero, false, err
		}
		var result *OfflineRegionInfo
		if bool(found) {
			info, err := offlineRegionSnapshotInfo(snapshot)
			if err != nil {
				return zero, true, err
			}
			result = &info
		}
		if typed, ok := any(result).(T); ok {
			return typed, true, nil
		}
		return zero, true, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case operationResultRegionList:
		var list C.mln_offline_region_list
		var err error
		switch kind {
		case operationRegionsList:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_regions_list_take_result(id, &list))
			})
		case operationRegionsMergeDatabase:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_regions_merge_database_take_result(id, &list))
			})
		default:
			return zero, false, newBindingError(ErrInvalidState, "offline operation result kind mismatch")
		}
		if err != nil {
			return zero, false, err
		}
		regions, err := offlineRegionListInfos(list)
		if err != nil {
			return zero, true, err
		}
		if result, ok := any(regions).(T); ok {
			return result, true, nil
		}
		return zero, true, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case operationResultRegionStatus:
		raw := C.mln_offline_region_status{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_status{}))}
		if err := checkNative(func() int32 { return int32(C.mln_runtime_offline_region_get_status_take_result(id, &raw)) }); err != nil {
			return zero, false, err
		}
		if result, ok := any(offlineRegionStatusFromC(raw)).(T); ok {
			return result, true, nil
		}
		return zero, true, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case operationResultCamera:
		if kind != operationCameraQuery {
			return zero, false, newBindingError(ErrInvalidState, "camera operation result kind mismatch")
		}
		raw := C.mln_camera_query_result{size: C.uint32_t(unsafe.Sizeof(C.mln_camera_query_result{}))}
		if err := checkNative(func() int32 {
			return int32(C.mln_map_camera_query_take_result(id, &raw))
		}); err != nil {
			return zero, false, err
		}
		snapshot := CameraSnapshot{
			Generation: uint64(raw.generation),
			Camera:     goCameraOptions(raw.camera),
		}
		if result, ok := any(snapshot).(T); ok {
			return result, true, nil
		}
		return zero, true, newBindingError(ErrInvalidState, "camera operation result type mismatch")
	default:
		return zero, false, newBindingError(ErrInvalidState, "unknown offline operation result kind")
	}
}

func offlineRegionSnapshotInfo(snapshot C.mln_offline_region_snapshot) (OfflineRegionInfo, error) {
	defer C.mln_offline_region_snapshot_destroy(snapshot)
	raw := C.mln_offline_region_info{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_info{}))}
	if err := checkNative(func() int32 { return int32(C.mln_offline_region_snapshot_get(snapshot, &raw)) }); err != nil {
		return OfflineRegionInfo{}, err
	}
	return offlineRegionInfoFromC(raw)
}

func offlineRegionListInfos(list C.mln_offline_region_list) ([]OfflineRegionInfo, error) {
	defer C.mln_offline_region_list_destroy(list)
	var count C.size_t
	if err := checkNative(func() int32 { return int32(C.mln_offline_region_list_count(list, &count)) }); err != nil {
		return nil, err
	}
	regions := make([]OfflineRegionInfo, int(count))
	for index := range regions {
		raw := C.mln_offline_region_info{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_info{}))}
		if err := checkNative(func() int32 { return int32(C.mln_offline_region_list_get(list, C.size_t(index), &raw)) }); err != nil {
			return nil, err
		}
		info, err := offlineRegionInfoFromC(raw)
		if err != nil {
			return nil, err
		}
		regions[index] = info
	}
	return regions, nil
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

func startOperation[T any](runtime *RuntimeHandle, kind operationKind, resultKind operationResultKind, start func(nativeRuntime, *C.mln_operation) int32) (*OperationHandle[T], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}

	defer runtime.state.KeepAlive()

	var id C.mln_operation
	if err := checkNative(func() int32 { return start(ptr, &id) }); err != nil {
		return nil, err
	}
	if id == 0 {
		return nil, newBindingError(ErrInvalidState, "operation did not return a handle")
	}
	return newOperationHandle[T](runtime, uint64(id), kind, resultKind), nil
}
