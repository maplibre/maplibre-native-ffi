package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

// OfflineRegionID identifies a native offline region.
type OfflineRegionID int64

// OfflineRegionDownloadState controls native offline region downloading.
type OfflineRegionDownloadState uint32

const (
	OfflineRegionDownloadInactive OfflineRegionDownloadState = OfflineRegionDownloadState(capi.OfflineRegionDownloadInactive)
	OfflineRegionDownloadActive   OfflineRegionDownloadState = OfflineRegionDownloadState(capi.OfflineRegionDownloadActive)
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

func (definition OfflineTilePyramidRegionDefinition) toCAPI() capi.OfflineTilePyramidRegionDefinition {
	return capi.OfflineTilePyramidRegionDefinition{
		StyleURL:          definition.StyleURL,
		Bounds:            definition.Bounds.toCAPI(),
		MinZoom:           definition.MinZoom,
		MaxZoom:           definition.MaxZoom,
		PixelRatio:        definition.PixelRatio,
		IncludeIdeographs: definition.IncludeIdeographs,
	}
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

// StartCreateOfflineRegion starts creating an offline region.
func (runtime *RuntimeHandle) StartCreateOfflineRegion(definition OfflineRegionDefinition, metadata []byte) (*OfflineOperationHandle[OfflineRegionInfo], error) {
	tile, ok := definition.(OfflineTilePyramidRegionDefinition)
	if !ok {
		return nil, newBindingError(ErrInvalidArgument, "unsupported offline region definition")
	}
	if err := tile.validate(); err != nil {
		return nil, err
	}
	return startOfflineOperation[OfflineRegionInfo](runtime, OfflineOperationRegionCreate, OfflineOperationResultRegion, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionCreateStart(ptr, tile.toCAPI(), metadata, out)
	})
}

// StartOfflineRegion starts getting an offline region snapshot by ID.
func (runtime *RuntimeHandle) StartOfflineRegion(id OfflineRegionID) (*OfflineOperationHandle[*OfflineRegionInfo], error) {
	return startOfflineOperation[*OfflineRegionInfo](runtime, OfflineOperationRegionGet, OfflineOperationResultOptionalRegion, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionGetStart(ptr, int64(id), out)
	})
}

// StartOfflineRegions starts listing offline regions.
func (runtime *RuntimeHandle) StartOfflineRegions() (*OfflineOperationHandle[[]OfflineRegionInfo], error) {
	return startOfflineOperation[[]OfflineRegionInfo](runtime, OfflineOperationRegionsList, OfflineOperationResultRegionList, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionsListStart(ptr, out)
	})
}

// StartMergeOfflineRegionsDatabase starts merging offline regions from another
// database path.
func (runtime *RuntimeHandle) StartMergeOfflineRegionsDatabase(path string) (*OfflineOperationHandle[[]OfflineRegionInfo], error) {
	if err := validateCStringArgument("offline side database path", path); err != nil {
		return nil, err
	}
	return startOfflineOperation[[]OfflineRegionInfo](runtime, OfflineOperationRegionsMergeDatabase, OfflineOperationResultRegionList, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionsMergeDatabaseStart(ptr, path, out)
	})
}

// StartUpdateOfflineRegionMetadata starts updating offline region metadata.
func (runtime *RuntimeHandle) StartUpdateOfflineRegionMetadata(id OfflineRegionID, metadata []byte) (*OfflineOperationHandle[OfflineRegionInfo], error) {
	return startOfflineOperation[OfflineRegionInfo](runtime, OfflineOperationRegionUpdateMetadata, OfflineOperationResultRegion, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionUpdateMetadataStart(ptr, int64(id), metadata, out)
	})
}

// StartOfflineRegionStatus starts getting offline region status.
func (runtime *RuntimeHandle) StartOfflineRegionStatus(id OfflineRegionID) (*OfflineOperationHandle[OfflineRegionStatus], error) {
	return startOfflineOperation[OfflineRegionStatus](runtime, OfflineOperationRegionGetStatus, OfflineOperationResultRegionStatus, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionGetStatusStart(ptr, int64(id), out)
	})
}

// StartSetOfflineRegionObserved starts setting offline event observation state.
func (runtime *RuntimeHandle) StartSetOfflineRegionObserved(id OfflineRegionID, observed bool) (*OfflineOperationHandle[struct{}], error) {
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionSetObserved, OfflineOperationResultNone, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionSetObservedStart(ptr, int64(id), observed, out)
	})
}

// StartSetOfflineRegionDownloadState starts setting offline region download
// state.
func (runtime *RuntimeHandle) StartSetOfflineRegionDownloadState(id OfflineRegionID, state OfflineRegionDownloadState) (*OfflineOperationHandle[struct{}], error) {
	raw, err := rawOfflineRegionDownloadState(state)
	if err != nil {
		return nil, err
	}
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionSetDownloadState, OfflineOperationResultNone, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionSetDownloadStateStart(ptr, int64(id), raw, out)
	})
}

// StartInvalidateOfflineRegion starts invalidating cached resources for a
// region.
func (runtime *RuntimeHandle) StartInvalidateOfflineRegion(id OfflineRegionID) (*OfflineOperationHandle[struct{}], error) {
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionInvalidate, OfflineOperationResultNone, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionInvalidateStart(ptr, int64(id), out)
	})
}

// StartDeleteOfflineRegion starts deleting an offline region.
func (runtime *RuntimeHandle) StartDeleteOfflineRegion(id OfflineRegionID) (*OfflineOperationHandle[struct{}], error) {
	return startOfflineOperation[struct{}](runtime, OfflineOperationRegionDelete, OfflineOperationResultNone, func(ptr *capi.Runtime, out *uint64) capi.Status {
		return capi.RuntimeOfflineRegionDeleteStart(ptr, int64(id), out)
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

	result, err := takeOfflineOperationResult[T](ptr, id, kind, resultKind)
	if err != nil {
		return zero, err
	}
	operation.mu.Lock()
	operation.live = false
	operation.mu.Unlock()
	return result, nil
}

func takeOfflineOperationResult[T any](runtime *capi.Runtime, id uint64, kind OfflineOperationKind, resultKind OfflineOperationResultKind) (T, error) {
	var zero T
	switch resultKind {
	case OfflineOperationResultNone:
		if err := checkNative(func() capi.Status { return capi.RuntimeOfflineOperationDiscard(runtime, id) }); err != nil {
			return zero, err
		}
		if result, ok := any(struct{}{}).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultRegion:
		var raw capi.OfflineRegionInfo
		status := capi.StatusInvalidState
		switch kind {
		case OfflineOperationRegionCreate:
			status = capi.RuntimeOfflineRegionCreateTakeResult(runtime, id, &raw)
		case OfflineOperationRegionUpdateMetadata:
			status = capi.RuntimeOfflineRegionUpdateMetadataTakeResult(runtime, id, &raw)
		}
		if err := checkNative(func() capi.Status { return status }); err != nil {
			return zero, err
		}
		if result, ok := any(offlineRegionInfoFromCAPI(raw)).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultOptionalRegion:
		var raw capi.OfflineRegionInfo
		var found bool
		if err := checkNative(func() capi.Status { return capi.RuntimeOfflineRegionGetTakeResult(runtime, id, &raw, &found) }); err != nil {
			return zero, err
		}
		var result *OfflineRegionInfo
		if found {
			info := offlineRegionInfoFromCAPI(raw)
			result = &info
		}
		if typed, ok := any(result).(T); ok {
			return typed, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultRegionList:
		var raw []capi.OfflineRegionInfo
		status := capi.StatusInvalidState
		switch kind {
		case OfflineOperationRegionsList:
			status = capi.RuntimeOfflineRegionsListTakeResult(runtime, id, &raw)
		case OfflineOperationRegionsMergeDatabase:
			status = capi.RuntimeOfflineRegionsMergeDatabaseTakeResult(runtime, id, &raw)
		}
		if err := checkNative(func() capi.Status { return status }); err != nil {
			return zero, err
		}
		regions := make([]OfflineRegionInfo, len(raw))
		for index, region := range raw {
			regions[index] = offlineRegionInfoFromCAPI(region)
		}
		if result, ok := any(regions).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	case OfflineOperationResultRegionStatus:
		var raw capi.OfflineRegionStatus
		if err := checkNative(func() capi.Status { return capi.RuntimeOfflineRegionGetStatusTakeResult(runtime, id, &raw) }); err != nil {
			return zero, err
		}
		if result, ok := any(offlineRegionStatusFromCAPI(raw)).(T); ok {
			return result, nil
		}
		return zero, newBindingError(ErrInvalidState, "offline operation result type mismatch")
	default:
		return zero, newBindingError(ErrInvalidState, "unknown offline operation result kind")
	}
}

func offlineRegionInfoFromCAPI(info capi.OfflineRegionInfo) OfflineRegionInfo {
	copied := OfflineRegionInfo{
		ID:                OfflineRegionID(info.ID),
		RawDefinitionType: info.DefinitionType,
		Metadata:          append([]byte(nil), info.Metadata...),
	}
	if info.DefinitionType == capi.OfflineRegionDefinitionTilePyramid {
		copied.Definition = OfflineTilePyramidRegionDefinition{
			StyleURL:          info.TilePyramid.StyleURL,
			Bounds:            latLngBoundsFromCAPI(info.TilePyramid.Bounds),
			MinZoom:           info.TilePyramid.MinZoom,
			MaxZoom:           info.TilePyramid.MaxZoom,
			PixelRatio:        info.TilePyramid.PixelRatio,
			IncludeIdeographs: info.TilePyramid.IncludeIdeographs,
		}
	}
	return copied
}

func offlineRegionStatusFromCAPI(status capi.OfflineRegionStatus) OfflineRegionStatus {
	return OfflineRegionStatus{
		DownloadState:                  OfflineRegionDownloadState(status.DownloadState),
		RawDownloadState:               status.DownloadState,
		CompletedResourceCount:         status.CompletedResourceCount,
		CompletedResourceSize:          status.CompletedResourceSize,
		CompletedTileCount:             status.CompletedTileCount,
		RequiredTileCount:              status.RequiredTileCount,
		CompletedTileSize:              status.CompletedTileSize,
		RequiredResourceCount:          status.RequiredResourceCount,
		RequiredResourceCountIsPrecise: status.RequiredResourceCountIsPrecise,
		Complete:                       status.Complete,
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

func startOfflineOperation[T any](runtime *RuntimeHandle, kind OfflineOperationKind, resultKind OfflineOperationResultKind, start func(*capi.Runtime, *uint64) capi.Status) (*OfflineOperationHandle[T], error) {
	ptr, err := runtime.ptr()
	if err != nil {
		return nil, err
	}
	defer runtime.state.KeepAlive()

	var id uint64
	if err := checkNative(func() capi.Status { return start(ptr, &id) }); err != nil {
		return nil, err
	}
	return newOfflineOperationHandle[T](runtime, id, kind, resultKind), nil
}
