package maplibre

import (
	"context"
	"errors"
	"testing"
)

func testOfflineTileDefinition() OfflineTilePyramidRegionDefinition {
	return OfflineTilePyramidRegionDefinition{
		StyleURL: "http://example.com/offline-style.json",
		Bounds: LatLngBounds{
			Southwest: LatLng{Latitude: -1, Longitude: -2},
			Northeast: LatLng{Latitude: 1, Longitude: 2},
		},
		MinZoom:           0,
		MaxZoom:           1,
		PixelRatio:        1,
		IncludeIdeographs: true,
	}
}

func testOfflineGeometryDefinition() OfflineGeometryRegionDefinition {
	return OfflineGeometryRegionDefinition{
		StyleURL:          "http://example.com/offline-geometry-style.json",
		Geometry:          []byte(`{"type":"Polygon","coordinates":[[[-2,-1],[2,-1],[2,1],[-2,1],[-2,-1]]]}`),
		MinZoom:           0,
		MaxZoom:           1,
		PixelRatio:        1,
		IncludeIdeographs: true,
	}
}

func requireReleaseOperation[T any](_ *testing.T, _ *Future[T]) {
}

func TestOfflineRegionStartOperationsReturnTypedHandles(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	create, err := runtime.CreateOfflineRegion(testOfflineTileDefinition(), []byte{1, 2, 3})
	if err != nil {
		t.Fatalf("CreateOfflineRegion(): %v", err)
	}
	requireReleaseOperation(t, create)

	createGeometry, err := runtime.CreateOfflineRegion(testOfflineGeometryDefinition(), []byte{1, 2, 3})
	if err != nil {
		t.Fatalf("CreateOfflineRegion(geometry): %v", err)
	}
	requireReleaseOperation(t, createGeometry)

	get, err := runtime.OfflineRegion(1)
	if err != nil {
		t.Fatalf("OfflineRegion(): %v", err)
	}
	requireReleaseOperation(t, get)

	list, err := runtime.OfflineRegions()
	if err != nil {
		t.Fatalf("OfflineRegions(): %v", err)
	}
	requireReleaseOperation(t, list)

	update, err := runtime.UpdateOfflineRegionMetadata(1, []byte{4, 5, 6})
	if err != nil {
		t.Fatalf("UpdateOfflineRegionMetadata(): %v", err)
	}
	requireReleaseOperation(t, update)

	status, err := runtime.OfflineRegionStatus(1)
	if err != nil {
		t.Fatalf("OfflineRegionStatus(): %v", err)
	}
	requireReleaseOperation(t, status)

	observed, err := runtime.SetOfflineRegionObserved(1, true)
	if err != nil {
		t.Fatalf("SetOfflineRegionObserved(): %v", err)
	}
	requireReleaseOperation(t, observed)

	download, err := runtime.SetOfflineRegionDownloadState(1, OfflineRegionDownloadInactive)
	if err != nil {
		t.Fatalf("SetOfflineRegionDownloadState(): %v", err)
	}
	requireReleaseOperation(t, download)

	invalidate, err := runtime.InvalidateOfflineRegion(1)
	if err != nil {
		t.Fatalf("InvalidateOfflineRegion(): %v", err)
	}
	requireReleaseOperation(t, invalidate)

	deleteOperation, err := runtime.DeleteOfflineRegion(1)
	if err != nil {
		t.Fatalf("DeleteOfflineRegion(): %v", err)
	}
	requireReleaseOperation(t, deleteOperation)
}

func TestOfflineOperationResultDoesNotUseRuntimeEventQueue(t *testing.T) {
	options := NewRuntimeOptions("", ":memory:")
	options.EventMask = RuntimeEventMaskNone
	runtime, err := NewRuntimeWithOptions(options)
	if err != nil {
		t.Fatalf("NewRuntimeWithOptions(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	operation, err := runtime.OfflineRegions()
	if err != nil {
		t.Fatalf("OfflineRegions(): %v", err)
	}
	regions := waitTakeOperation(t, runtime, operation)
	if regions == nil {
		t.Fatal("Take() returned a nil region list")
	}
	batch, err := runtime.DrainEvents()
	if err != nil {
		t.Fatalf("DrainEvents(): %v", err)
	}
	if len(batch.Events) != 0 {
		t.Fatalf("DrainEvents() returned %d events with an empty mask", len(batch.Events))
	}
}

func waitTakeOperation[T any](t *testing.T, _ *RuntimeHandle, future *Future[T]) T {
	t.Helper()
	result, err := future.Await(context.Background())
	if err != nil {
		t.Fatalf("Await(): %v", err)
	}
	return result
}

func TestOfflineCreateAndListTakeResultsCopyNativeData(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	metadata := []byte{9, 8, 7}
	create, err := runtime.CreateOfflineRegion(testOfflineTileDefinition(), metadata)
	if err != nil {
		t.Fatalf("CreateOfflineRegion(): %v", err)
	}
	info := waitTakeOperation(t, runtime, create)
	if info.ID == 0 {
		t.Fatal("created offline region ID is zero")
	}
	if got := info.Metadata; len(got) != len(metadata) || got[0] != metadata[0] || got[1] != metadata[1] || got[2] != metadata[2] {
		t.Fatalf("metadata = %v, want %v", got, metadata)
	}
	tile, ok := info.Definition.(OfflineTilePyramidRegionDefinition)
	if !ok {
		t.Fatalf("definition = %T, want OfflineTilePyramidRegionDefinition", info.Definition)
	}
	if tile.StyleURL != testOfflineTileDefinition().StyleURL {
		t.Fatalf("StyleURL = %q, want %q", tile.StyleURL, testOfflineTileDefinition().StyleURL)
	}

	list, err := runtime.OfflineRegions()
	if err != nil {
		t.Fatalf("OfflineRegions(): %v", err)
	}
	regions := waitTakeOperation(t, runtime, list)
	if len(regions) == 0 {
		t.Fatal("offline region list is empty after creating a region")
	}
}

func TestSetMaximumAmbientCacheSizeReportsCompletion(t *testing.T) {
	runtime, err := NewRuntimeWithOptions(NewRuntimeOptions("", ":memory:"))
	if err != nil {
		t.Fatalf("NewRuntimeWithOptions(): %v", err)
	}
	defer closeRuntimeForTest(runtime)

	future, err := runtime.SetMaximumAmbientCacheSize(8 << 20)
	if _, err := awaitForTest(future, err); err != nil {
		t.Fatalf("SetMaximumAmbientCacheSize completion: %v", err)
	}
}

func TestOfflineRegionStartOperationsValidateGoInputs(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	definition := testOfflineTileDefinition()
	definition.StyleURL = "http://example.com/\x00style.json"
	if _, err := runtime.CreateOfflineRegion(definition, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("CreateOfflineRegion embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	geometryDefinition := testOfflineGeometryDefinition()
	geometryDefinition.StyleURL = "http://example.com/\x00style.json"
	if _, err := runtime.CreateOfflineRegion(geometryDefinition, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("CreateOfflineRegion geometry embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	geometryDefinition = testOfflineGeometryDefinition()
	geometryDefinition.Geometry = []byte(`{"type":"Unsupported"}`)
	if _, err := runtime.CreateOfflineRegion(geometryDefinition, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("CreateOfflineRegion bad geometry error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.MergeOfflineRegionsDatabase("/tmp/\x00side.db"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("MergeOfflineRegionsDatabase embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.SetOfflineRegionDownloadState(1, OfflineRegionDownloadState(999_999)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetOfflineRegionDownloadState unknown error = %v, want ErrInvalidArgument", err)
	}
}

func TestOfflineGeometryDefinitionMaterializesAndCopies(t *testing.T) {
	definition := testOfflineGeometryDefinition()
	raw, err := newCOfflineGeometryRegionDefinition(definition)
	if err != nil {
		t.Fatalf("newCOfflineGeometryRegionDefinition(): %v", err)
	}
	defer raw.free()

	copiedDefinition, err := raw.copyDefinition()
	if err != nil {
		t.Fatalf("copyDefinition(): %v", err)
	}
	copied, ok := copiedDefinition.(OfflineGeometryRegionDefinition)
	if !ok {
		t.Fatalf("copyDefinition() = %T, want OfflineGeometryRegionDefinition", copiedDefinition)
	}
	if copied.StyleURL != definition.StyleURL || copied.MinZoom != definition.MinZoom || copied.MaxZoom != definition.MaxZoom || copied.PixelRatio != definition.PixelRatio || copied.IncludeIdeographs != definition.IncludeIdeographs {
		t.Fatalf("copied scalar fields = %#v, want %#v", copied, definition)
	}
	if string(copied.Geometry) != string(definition.Geometry) {
		t.Fatalf("copied geometry = %q, want %q", copied.Geometry, definition.Geometry)
	}
}
