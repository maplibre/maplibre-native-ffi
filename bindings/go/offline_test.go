package maplibre

import (
	"bytes"
	"encoding/json"
	"errors"
	"reflect"
	"testing"
)

// decodeJSONForTest parses one JSON document so a comparison ignores the
// formatting MapLibre chose when it reserialized the value.
func decodeJSONForTest(t *testing.T, document []byte) any {
	t.Helper()
	var value any
	if err := json.Unmarshal(document, &value); err != nil {
		t.Fatalf("parsing %q: %v", document, err)
	}
	return value
}

// newOfflineRuntimeForTest creates a runtime whose offline database starts
// empty and lives only as long as the test.
func newOfflineRuntimeForTest(t *testing.T) *RuntimeHandle {
	t.Helper()
	runtime, err := RuntimeCreate(runtimeOptionsForTest("", ":memory:"))
	if err != nil {
		t.Fatalf("RuntimeCreate(): %v", err)
	}
	t.Cleanup(func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	})
	return runtime
}

func testOfflineTileDefinition() OfflineTilePyramidRegionDefinition {
	return OfflineTilePyramidRegionDefinition{
		StyleUrl: "http://example.com/offline-style.json",
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
		StyleUrl:          "http://example.com/offline-geometry-style.json",
		Geometry:          []byte(`{"type":"Polygon","coordinates":[[[-2,-1],[2,-1],[2,1],[-2,1],[-2,-1]]]}`),
		MinZoom:           0,
		MaxZoom:           1,
		PixelRatio:        1,
		IncludeIdeographs: true,
	}
}

// The whole region lifecycle runs through the public API, and every operation
// reaches a terminal outcome the test observes.
func TestOfflineRegionOperationsRunAWholeRegionLifecycle(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

	metadata := []byte{9, 8, 7}
	tile, err := awaitForTest(runtime.OfflineRegionCreate(offlineDefinitionForTest(testOfflineTileDefinition()), metadata))
	if err != nil {
		t.Fatalf("OfflineRegionCreate(tile pyramid): %v", err)
	}
	if tile.Id == 0 {
		t.Fatal("created offline region ID is zero")
	}
	if !bytes.Equal(tile.Metadata, metadata) {
		t.Fatalf("metadata = %v, want %v", tile.Metadata, metadata)
	}
	tileVariant, ok := tile.Definition.Data.(OfflineRegionDefinitionDataTilePyramidVariant)
	tileDefinition := tileVariant.Value
	if !ok {
		t.Fatalf("definition = %T, want OfflineTilePyramidRegionDefinition", tile.Definition)
	}
	if tileDefinition.StyleUrl != testOfflineTileDefinition().StyleUrl {
		t.Fatalf("StyleURL = %q, want %q", tileDefinition.StyleUrl, testOfflineTileDefinition().StyleUrl)
	}

	geometry, err := awaitForTest(runtime.OfflineRegionCreate(offlineDefinitionForTest(testOfflineGeometryDefinition()), nil))
	if err != nil {
		t.Fatalf("OfflineRegionCreate(geometry): %v", err)
	}
	geometryVariant, ok := geometry.Definition.Data.(OfflineRegionDefinitionDataGeometryVariant)
	geometryDefinition := geometryVariant.Value
	if !ok {
		t.Fatalf("definition = %T, want OfflineGeometryRegionDefinition", geometry.Definition)
	}
	// MapLibre stores the parsed geometry and reserializes it, so the copy is
	// compared as JSON rather than byte for byte.
	if got, want := decodeJSONForTest(t, geometryDefinition.Geometry), decodeJSONForTest(t, testOfflineGeometryDefinition().Geometry); !reflect.DeepEqual(got, want) {
		t.Fatalf("geometry = %v, want %v", got, want)
	}

	regions, err := awaitForTest(runtime.OfflineRegionsList())
	if err != nil {
		t.Fatalf("OfflineRegionsList(): %v", err)
	}
	if len(regions) != 2 {
		t.Fatalf("OfflineRegionsList() = %d regions, want 2", len(regions))
	}

	stored, err := awaitForTest(runtime.OfflineRegionGet(tile.Id))
	if err != nil {
		t.Fatalf("OfflineRegion(): %v", err)
	}
	if stored == nil || stored.Id != tile.Id {
		t.Fatalf("OfflineRegion(%d) = %#v, want the stored region", tile.Id, stored)
	}

	replacement := []byte{1, 2}
	updated, err := awaitForTest(runtime.OfflineRegionUpdateMetadata(tile.Id, replacement))
	if err != nil {
		t.Fatalf("UpdateOfflineRegionMetadata(): %v", err)
	}
	if !bytes.Equal(updated.Metadata, replacement) {
		t.Fatalf("updated metadata = %v, want %v", updated.Metadata, replacement)
	}

	status, err := awaitForTest(runtime.OfflineRegionGetStatus(tile.Id))
	if err != nil {
		t.Fatalf("OfflineRegionStatus(): %v", err)
	}
	if status.DownloadState != OfflineRegionDownloadStateInactive {
		t.Fatalf("download state = %v, want inactive", status.DownloadState)
	}

	if _, err := awaitForTest(runtime.OfflineRegionSetObserved(tile.Id, true)); err != nil {
		t.Fatalf("SetOfflineRegionObserved(): %v", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionSetDownloadState(tile.Id, OfflineRegionDownloadStateInactive)); err != nil {
		t.Fatalf("SetOfflineRegionDownloadState(): %v", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionInvalidate(tile.Id)); err != nil {
		t.Fatalf("InvalidateOfflineRegion(): %v", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionDelete(tile.Id)); err != nil {
		t.Fatalf("DeleteOfflineRegion(): %v", err)
	}

	// A deleted region is missing, and a get reports that with no record rather
	// than with an error.
	deleted, err := awaitForTest(runtime.OfflineRegionGet(tile.Id))
	if err != nil {
		t.Fatalf("OfflineRegion() after delete: %v", err)
	}
	if deleted != nil {
		t.Fatalf("OfflineRegion(%d) after delete = %#v, want no record", tile.Id, deleted)
	}
}

// Every region mutation reports ErrNotFound for an ID no region carries, while
// the get reports the missing region with no record instead.
func TestOfflineRegionOperationsReportNotFoundForAMissingRegion(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

	const missing int64 = 4242
	stored, err := awaitForTest(runtime.OfflineRegionGet(missing))
	if err != nil {
		t.Fatalf("OfflineRegion(): %v", err)
	}
	if stored != nil {
		t.Fatalf("OfflineRegion(%d) = %#v, want no record", missing, stored)
	}

	if _, err := awaitForTest(runtime.OfflineRegionUpdateMetadata(missing, []byte{1})); !errors.Is(err, ErrNotFound) {
		t.Fatalf("UpdateOfflineRegionMetadata() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionGetStatus(missing)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("OfflineRegionStatus() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionSetObserved(missing, true)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("SetOfflineRegionObserved() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionSetDownloadState(missing, OfflineRegionDownloadStateInactive)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("SetOfflineRegionDownloadState() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionInvalidate(missing)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("InvalidateOfflineRegion() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionDelete(missing)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("DeleteOfflineRegion() error = %v, want ErrNotFound", err)
	}
}

func TestOfflineOperationResultDoesNotUseRuntimeEventQueue(t *testing.T) {
	options := runtimeOptionsForTest("", ":memory:")
	options.EventMask = RuntimeEventMaskNone
	runtime, err := RuntimeCreate(options)
	if err != nil {
		t.Fatalf("RuntimeCreate(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	regions, err := awaitForTest(runtime.OfflineRegionsList())
	if err != nil {
		t.Fatalf("OfflineRegionsList(): %v", err)
	}
	if regions == nil {
		t.Fatal("OfflineRegionsList() returned a nil region list")
	}
	drained, err := drainEventsForTest(runtime)
	if err != nil {
		t.Fatalf("Close(): %v", err)
	}
	if len(drained) != 0 {
		t.Fatalf("Close() returned %d events with an empty mask", len(drained))
	}
}

func TestAmbientCacheOperationsKeepStoredOfflineRegions(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

	if _, err := awaitForTest(runtime.SetMaximumAmbientCacheSize(8 << 20)); err != nil {
		t.Fatalf("SetMaximumAmbientCacheSize(): %v", err)
	}
	if _, err := awaitForTest(runtime.RunAmbientCacheOperation(AmbientCacheOperationInvalidate)); err != nil {
		t.Fatalf("AmbientCacheOperation(invalidate): %v", err)
	}

	region, err := awaitForTest(runtime.OfflineRegionCreate(offlineDefinitionForTest(testOfflineTileDefinition()), nil))
	if err != nil {
		t.Fatalf("OfflineRegionCreate(): %v", err)
	}
	if _, err := awaitForTest(runtime.RunAmbientCacheOperation(AmbientCacheOperationClear)); err != nil {
		t.Fatalf("AmbientCacheOperation(clear): %v", err)
	}

	stored, err := awaitForTest(runtime.OfflineRegionGet(region.Id))
	if err != nil {
		t.Fatalf("OfflineRegion(): %v", err)
	}
	if stored == nil || stored.Id != region.Id {
		t.Fatalf("OfflineRegion(%d) after an ambient cache clear = %#v, want the stored region", region.Id, stored)
	}
}

func TestOfflineRegionStartOperationsValidateGoInputs(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

	definition := testOfflineTileDefinition()
	definition.StyleUrl = "http://example.com/\x00style.json"
	if _, err := runtime.OfflineRegionCreate(offlineDefinitionForTest(definition), nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("OfflineRegionCreate embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	geometryDefinition := testOfflineGeometryDefinition()
	geometryDefinition.StyleUrl = "http://example.com/\x00style.json"
	if _, err := runtime.OfflineRegionCreate(offlineDefinitionForTest(geometryDefinition), nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("OfflineRegionCreate geometry embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	geometryDefinition = testOfflineGeometryDefinition()
	geometryDefinition.Geometry = []byte(`{"type":"Unsupported"}`)
	if _, err := runtime.OfflineRegionCreate(offlineDefinitionForTest(geometryDefinition), nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("OfflineRegionCreate bad geometry error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.OfflineRegionsMergeDatabase("/tmp/\x00side.db"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("OfflineRegionsMergeDatabase embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.OfflineRegionSetDownloadState(1, OfflineRegionDownloadState(999_999)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetOfflineRegionDownloadState unknown error = %v, want ErrInvalidArgument", err)
	}
}
