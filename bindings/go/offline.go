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

func metadataPointer(metadata []byte) *C.uint8_t {
	if len(metadata) == 0 {
		return nil
	}
	return (*C.uint8_t)(unsafe.Pointer(&metadata[0]))
}

// StartCreateOfflineRegion starts creating an offline region.
func (runtime *RuntimeHandle) StartCreateOfflineRegion(definition OfflineRegionDefinition, metadata []byte) (*OfflineOperationHandle[OfflineRegionInfo], error) {
	tile, ok := definition.(OfflineTilePyramidRegionDefinition)
	if !ok {
		return nil, newBindingError(ErrInvalidArgument, "unsupported offline region definition")
	}
	if err := tile.validate(); err != nil {
		return nil, err
	}
	return startOfflineOperation[OfflineRegionInfo](runtime, OfflineOperationRegionCreate, OfflineOperationResultRegion, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		rawDefinition := newCOfflineTilePyramidRegionDefinition(tile)
		defer rawDefinition.free()
		return int32(C.mln_runtime_offline_region_create_start(
			(*C.mln_runtime)(unsafe.Pointer(ptr)),
			&rawDefinition.raw,
			metadataPointer(metadata),
			C.size_t(len(metadata)),
			out,
		))
	})
}

// StartOfflineRegion starts getting an offline region snapshot by ID.
func (runtime *RuntimeHandle) StartOfflineRegion(id OfflineRegionID) (*OfflineOperationHandle[*OfflineRegionInfo], error) {
	return startOfflineOperation[*OfflineRegionInfo](runtime, OfflineOperationRegionGet, OfflineOperationResultOptionalRegion, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_region_get_start((*C.mln_runtime)(unsafe.Pointer(ptr)), C.mln_offline_region_id(id), out))
	})
}

// StartOfflineRegions starts listing offline regions.
func (runtime *RuntimeHandle) StartOfflineRegions() (*OfflineOperationHandle[[]OfflineRegionInfo], error) {
	return startOfflineOperation[[]OfflineRegionInfo](runtime, OfflineOperationRegionsList, OfflineOperationResultRegionList, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_regions_list_start((*C.mln_runtime)(unsafe.Pointer(ptr)), out))
	})
}

// StartMergeOfflineRegionsDatabase starts merging offline regions from another
// database path.
func (runtime *RuntimeHandle) StartMergeOfflineRegionsDatabase(path string) (*OfflineOperationHandle[[]OfflineRegionInfo], error) {
	if err := validateCStringArgument("offline side database path", path); err != nil {
		return nil, err
	}
	return startOfflineOperation[[]OfflineRegionInfo](runtime, OfflineOperationRegionsMergeDatabase, OfflineOperationResultRegionList, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		rawPath := C.CString(path)
		defer C.free(unsafe.Pointer(rawPath))
		return int32(C.mln_runtime_offline_regions_merge_database_start((*C.mln_runtime)(unsafe.Pointer(ptr)), rawPath, out))
	})
}

// StartUpdateOfflineRegionMetadata starts updating offline region metadata.
func (runtime *RuntimeHandle) StartUpdateOfflineRegionMetadata(id OfflineRegionID, metadata []byte) (*OfflineOperationHandle[OfflineRegionInfo], error) {
	return startOfflineOperation[OfflineRegionInfo](runtime, OfflineOperationRegionUpdateMetadata, OfflineOperationResultRegion, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_region_update_metadata_start(
			(*C.mln_runtime)(unsafe.Pointer(ptr)),
			C.mln_offline_region_id(id),
			metadataPointer(metadata),
			C.size_t(len(metadata)),
			out,
		))
	})
}

// StartOfflineRegionStatus starts getting offline region status.
func (runtime *RuntimeHandle) StartOfflineRegionStatus(id OfflineRegionID) (*OfflineOperationHandle[OfflineRegionStatus], error) {
	return startOfflineOperation[OfflineRegionStatus](runtime, OfflineOperationRegionGetStatus, OfflineOperationResultRegionStatus, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_region_get_status_start((*C.mln_runtime)(unsafe.Pointer(ptr)), C.mln_offline_region_id(id), out))
	})
}

// StartSetOfflineRegionObserved starts setting offline event observation state.
func (runtime *RuntimeHandle) StartSetOfflineRegionObserved(id OfflineRegionID, observed bool) (*OfflineOperationHandle[struct{}], error) {
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionSetObserved, OfflineOperationResultNone, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_region_set_observed_start((*C.mln_runtime)(unsafe.Pointer(ptr)), C.mln_offline_region_id(id), C.bool(observed), out))
	})
}

// StartSetOfflineRegionDownloadState starts setting offline region download
// state.
func (runtime *RuntimeHandle) StartSetOfflineRegionDownloadState(id OfflineRegionID, state OfflineRegionDownloadState) (*OfflineOperationHandle[struct{}], error) {
	raw, err := rawOfflineRegionDownloadState(state)
	if err != nil {
		return nil, err
	}
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionSetDownloadState, OfflineOperationResultNone, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_region_set_download_state_start((*C.mln_runtime)(unsafe.Pointer(ptr)), C.mln_offline_region_id(id), C.uint32_t(raw), out))
	})
}

// StartInvalidateOfflineRegion starts invalidating cached resources for a
// region.
func (runtime *RuntimeHandle) StartInvalidateOfflineRegion(id OfflineRegionID) (*OfflineOperationHandle[struct{}], error) {
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionInvalidate, OfflineOperationResultNone, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_region_invalidate_start((*C.mln_runtime)(unsafe.Pointer(ptr)), C.mln_offline_region_id(id), out))
	})
}

// StartDeleteOfflineRegion starts deleting an offline region.
func (runtime *RuntimeHandle) StartDeleteOfflineRegion(id OfflineRegionID) (*OfflineOperationHandle[struct{}], error) {
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionDelete, OfflineOperationResultNone, func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
		return int32(C.mln_runtime_offline_region_delete_start((*C.mln_runtime)(unsafe.Pointer(ptr)), C.mln_offline_region_id(id), out))
	})
}

// Take consumes a completed offline operation result and copies it into Go
// values. If native reports that the result is not ready, the handle remains
// live so callers can retry later or discard it.
func (operation *OfflineOperationHandle[T]) Take() (T, error) {
	var zero T
	if operation == nil || operation.runtime == nil {
		return zero, newBindingError(ErrInvalidArgument, "OfflineOperationHandle is nil")
	}
	operation.mu.Lock()
	if !operation.live {
		operation.mu.Unlock()
		return zero, newBindingError(ErrInvalidArgument, "OfflineOperationHandle is closed")
	}
	id := operation.id
	kind := operation.kind
	resultKind := operation.resultKind
	operation.mu.Unlock()

	ptr, err := operation.runtime.ptr()
	if err != nil {
		return zero, err
	}
	defer operation.runtime.state.KeepAlive()

	result, err := takeOfflineOperationResult[T](ptr, C.mln_offline_operation_id(id), kind, resultKind)
	if err != nil {
		return zero, err
	}
	operation.mu.Lock()
	operation.live = false
	operation.mu.Unlock()
	return result, nil
}

func takeOfflineOperationResult[T any](runtime *nativeRuntime, id C.mln_offline_operation_id, kind OfflineOperationKind, resultKind OfflineOperationResultKind) (T, error) {
	var zero T
	rawRuntime := (*C.mln_runtime)(unsafe.Pointer(runtime))
	switch resultKind {
	case OfflineOperationResultNone:
		if err := checkNative(func() int32 { return int32(C.mln_runtime_offline_operation_discard(rawRuntime, id)) }); err != nil {
			return zero, err
		}
		if result, ok := any(struct{}{}).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultRegion:
		var snapshot *C.mln_offline_region_snapshot
		var err error
		switch kind {
		case OfflineOperationRegionCreate:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_region_create_take_result(rawRuntime, id, &snapshot))
			})
		case OfflineOperationRegionUpdateMetadata:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_region_update_metadata_take_result(rawRuntime, id, &snapshot))
			})
		default:
			return zero, newBindingError(ErrInvalidState, "offline operation result kind mismatch")
		}
		if err != nil {
			return zero, err
		}
		info, err := offlineRegionSnapshotInfo(snapshot)
		if err != nil {
			return zero, err
		}
		if result, ok := any(info).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultOptionalRegion:
		var snapshot *C.mln_offline_region_snapshot
		var found C.bool
		if err := checkNative(func() int32 {
			return int32(C.mln_runtime_offline_region_get_take_result(rawRuntime, id, &snapshot, &found))
		}); err != nil {
			return zero, err
		}
		var result *OfflineRegionInfo
		if bool(found) {
			info, err := offlineRegionSnapshotInfo(snapshot)
			if err != nil {
				return zero, err
			}
			result = &info
		}
		if typed, ok := any(result).(T); ok {
			return typed, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultRegionList:
		var list *C.mln_offline_region_list
		var err error
		switch kind {
		case OfflineOperationRegionsList:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_regions_list_take_result(rawRuntime, id, &list))
			})
		case OfflineOperationRegionsMergeDatabase:
			err = checkNative(func() int32 {
				return int32(C.mln_runtime_offline_regions_merge_database_take_result(rawRuntime, id, &list))
			})
		default:
			return zero, newBindingError(ErrInvalidState, "offline operation result kind mismatch")
		}
		if err != nil {
			return zero, err
		}
		regions, err := offlineRegionListInfos(list)
		if err != nil {
			return zero, err
		}
		if result, ok := any(regions).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultRegionStatus:
		raw := C.mln_offline_region_status{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_status{}))}
		if err := checkNative(func() int32 { return int32(C.mln_runtime_offline_region_get_status_take_result(rawRuntime, id, &raw)) }); err != nil {
			return zero, err
		}
		if result, ok := any(offlineRegionStatusFromC(raw)).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	default:
		return zero, newBindingError(ErrInvalidState, "unknown offline operation result kind")
	}
}

func offlineRegionSnapshotInfo(snapshot *C.mln_offline_region_snapshot) (OfflineRegionInfo, error) {
	defer C.mln_offline_region_snapshot_destroy(snapshot)
	raw := C.mln_offline_region_info{size: C.uint32_t(unsafe.Sizeof(C.mln_offline_region_info{}))}
	if err := checkNative(func() int32 { return int32(C.mln_offline_region_snapshot_get(snapshot, &raw)) }); err != nil {
		return OfflineRegionInfo{}, err
	}
	return offlineRegionInfoFromC(raw), nil
}

func offlineRegionListInfos(list *C.mln_offline_region_list) ([]OfflineRegionInfo, error) {
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
		regions[index] = offlineRegionInfoFromC(raw)
	}
	return regions, nil
}

func offlineRegionInfoFromC(info C.mln_offline_region_info) OfflineRegionInfo {
	definitionType := uint32(C.mln_go_offline_region_info_definition_type(&info))
	copied := OfflineRegionInfo{
		ID:                OfflineRegionID(info.id),
		RawDefinitionType: definitionType,
		Metadata:          append([]byte(nil), unsafe.Slice((*byte)(unsafe.Pointer(info.metadata)), int(info.metadata_size))...),
	}
	if definitionType == uint32(C.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID) {
		tile := C.mln_go_offline_region_info_tile_pyramid(&info)
		copied.Definition = OfflineTilePyramidRegionDefinition{
			StyleURL:          C.GoString(tile.style_url),
			Bounds:            goLatLngBounds(tile.bounds),
			MinZoom:           float64(tile.min_zoom),
			MaxZoom:           float64(tile.max_zoom),
			PixelRatio:        float32(tile.pixel_ratio),
			IncludeIdeographs: bool(tile.include_ideographs),
		}
	}
	return copied
}

func rawOfflineRegionDownloadState(state OfflineRegionDownloadState) (uint32, error) {
	switch state {
	case OfflineRegionDownloadInactive, OfflineRegionDownloadActive:
		return uint32(state), nil
	default:
		return 0, newBindingError(ErrInvalidArgument, "unknown offline region download state cannot be set")
	}
}

func startOfflineOperation[T any](runtime *RuntimeHandle, kind OfflineOperationKind, resultKind OfflineOperationResultKind, start func(*nativeRuntime, *C.mln_offline_operation_id) int32) (*OfflineOperationHandle[T], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer runtime.state.KeepAlive()

	var id C.mln_offline_operation_id
	if err := checkNative(func() int32 { return start(ptr, &id) }); err != nil {
		return nil, err
	}
	return newOfflineOperationHandle[T](runtime, uint64(id), kind, resultKind), nil
}
