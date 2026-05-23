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
	ID         OfflineRegionID
	Definition OfflineRegionDefinition
	Metadata   []byte
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
