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
	runtime, err := NewRuntimeWithOptions(NewRuntimeOptions("", ":memory:"))
	if err != nil {
		t.Fatalf("NewRuntimeWithOptions(): %v", err)
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

// The whole region lifecycle runs through the public API, and every operation
// reaches a terminal outcome the test observes.
func TestOfflineRegionOperationsRunAWholeRegionLifecycle(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

	metadata := []byte{9, 8, 7}
	tile, err := awaitForTest(runtime.CreateOfflineRegion(testOfflineTileDefinition(), metadata))
	if err != nil {
		t.Fatalf("CreateOfflineRegion(tile pyramid): %v", err)
	}
	if tile.ID == 0 {
		t.Fatal("created offline region ID is zero")
	}
	if !bytes.Equal(tile.Metadata, metadata) {
		t.Fatalf("metadata = %v, want %v", tile.Metadata, metadata)
	}
	tileDefinition, ok := tile.Definition.(OfflineTilePyramidRegionDefinition)
	if !ok {
		t.Fatalf("definition = %T, want OfflineTilePyramidRegionDefinition", tile.Definition)
	}
	if tileDefinition.StyleURL != testOfflineTileDefinition().StyleURL {
		t.Fatalf("StyleURL = %q, want %q", tileDefinition.StyleURL, testOfflineTileDefinition().StyleURL)
	}

	geometry, err := awaitForTest(runtime.CreateOfflineRegion(testOfflineGeometryDefinition(), nil))
	if err != nil {
		t.Fatalf("CreateOfflineRegion(geometry): %v", err)
	}
	geometryDefinition, ok := geometry.Definition.(OfflineGeometryRegionDefinition)
	if !ok {
		t.Fatalf("definition = %T, want OfflineGeometryRegionDefinition", geometry.Definition)
	}
	// MapLibre stores the parsed geometry and reserializes it, so the copy is
	// compared as JSON rather than byte for byte.
	if got, want := decodeJSONForTest(t, geometryDefinition.Geometry), decodeJSONForTest(t, testOfflineGeometryDefinition().Geometry); !reflect.DeepEqual(got, want) {
		t.Fatalf("geometry = %v, want %v", got, want)
	}

	regions, err := awaitForTest(runtime.OfflineRegions())
	if err != nil {
		t.Fatalf("OfflineRegions(): %v", err)
	}
	if len(regions) != 2 {
		t.Fatalf("OfflineRegions() = %d regions, want 2", len(regions))
	}

	stored, err := awaitForTest(runtime.OfflineRegion(tile.ID))
	if err != nil {
		t.Fatalf("OfflineRegion(): %v", err)
	}
	if stored == nil || stored.ID != tile.ID {
		t.Fatalf("OfflineRegion(%d) = %#v, want the stored region", tile.ID, stored)
	}

	replacement := []byte{1, 2}
	updated, err := awaitForTest(runtime.UpdateOfflineRegionMetadata(tile.ID, replacement))
	if err != nil {
		t.Fatalf("UpdateOfflineRegionMetadata(): %v", err)
	}
	if !bytes.Equal(updated.Metadata, replacement) {
		t.Fatalf("updated metadata = %v, want %v", updated.Metadata, replacement)
	}

	status, err := awaitForTest(runtime.OfflineRegionStatus(tile.ID))
	if err != nil {
		t.Fatalf("OfflineRegionStatus(): %v", err)
	}
	if status.DownloadState != OfflineRegionDownloadInactive {
		t.Fatalf("download state = %v, want inactive", status.DownloadState)
	}

	if _, err := awaitForTest(runtime.SetOfflineRegionObserved(tile.ID, true)); err != nil {
		t.Fatalf("SetOfflineRegionObserved(): %v", err)
	}
	if _, err := awaitForTest(runtime.SetOfflineRegionDownloadState(tile.ID, OfflineRegionDownloadInactive)); err != nil {
		t.Fatalf("SetOfflineRegionDownloadState(): %v", err)
	}
	if _, err := awaitForTest(runtime.InvalidateOfflineRegion(tile.ID)); err != nil {
		t.Fatalf("InvalidateOfflineRegion(): %v", err)
	}
	if _, err := awaitForTest(runtime.DeleteOfflineRegion(tile.ID)); err != nil {
		t.Fatalf("DeleteOfflineRegion(): %v", err)
	}

	// A deleted region is missing, and a get reports that with no record rather
	// than with an error.
	deleted, err := awaitForTest(runtime.OfflineRegion(tile.ID))
	if err != nil {
		t.Fatalf("OfflineRegion() after delete: %v", err)
	}
	if deleted != nil {
		t.Fatalf("OfflineRegion(%d) after delete = %#v, want no record", tile.ID, deleted)
	}
}

// Every region mutation reports ErrNotFound for an ID no region carries, while
// the get reports the missing region with no record instead.
func TestOfflineRegionOperationsReportNotFoundForAMissingRegion(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

	const missing OfflineRegionID = 4242
	stored, err := awaitForTest(runtime.OfflineRegion(missing))
	if err != nil {
		t.Fatalf("OfflineRegion(): %v", err)
	}
	if stored != nil {
		t.Fatalf("OfflineRegion(%d) = %#v, want no record", missing, stored)
	}

	if _, err := awaitForTest(runtime.UpdateOfflineRegionMetadata(missing, []byte{1})); !errors.Is(err, ErrNotFound) {
		t.Fatalf("UpdateOfflineRegionMetadata() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.OfflineRegionStatus(missing)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("OfflineRegionStatus() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.SetOfflineRegionObserved(missing, true)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("SetOfflineRegionObserved() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.SetOfflineRegionDownloadState(missing, OfflineRegionDownloadInactive)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("SetOfflineRegionDownloadState() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.InvalidateOfflineRegion(missing)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("InvalidateOfflineRegion() error = %v, want ErrNotFound", err)
	}
	if _, err := awaitForTest(runtime.DeleteOfflineRegion(missing)); !errors.Is(err, ErrNotFound) {
		t.Fatalf("DeleteOfflineRegion() error = %v, want ErrNotFound", err)
	}
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

	regions, err := awaitForTest(runtime.OfflineRegions())
	if err != nil {
		t.Fatalf("OfflineRegions(): %v", err)
	}
	if regions == nil {
		t.Fatal("OfflineRegions() returned a nil region list")
	}
	drained, err := runtime.DrainEvents()
	if err != nil {
		t.Fatalf("DrainEvents(): %v", err)
	}
	if len(drained) != 0 {
		t.Fatalf("DrainEvents() returned %d events with an empty mask", len(drained))
	}
}

func TestAmbientCacheOperationsKeepStoredOfflineRegions(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

	if _, err := awaitForTest(runtime.SetMaximumAmbientCacheSize(8 << 20)); err != nil {
		t.Fatalf("SetMaximumAmbientCacheSize(): %v", err)
	}
	if _, err := awaitForTest(runtime.AmbientCacheOperation(AmbientCacheOperationInvalidate)); err != nil {
		t.Fatalf("AmbientCacheOperation(invalidate): %v", err)
	}

	region, err := awaitForTest(runtime.CreateOfflineRegion(testOfflineTileDefinition(), nil))
	if err != nil {
		t.Fatalf("CreateOfflineRegion(): %v", err)
	}
	if _, err := awaitForTest(runtime.AmbientCacheOperation(AmbientCacheOperationClear)); err != nil {
		t.Fatalf("AmbientCacheOperation(clear): %v", err)
	}

	stored, err := awaitForTest(runtime.OfflineRegion(region.ID))
	if err != nil {
		t.Fatalf("OfflineRegion(): %v", err)
	}
	if stored == nil || stored.ID != region.ID {
		t.Fatalf("OfflineRegion(%d) after an ambient cache clear = %#v, want the stored region", region.ID, stored)
	}
}

func TestOfflineRegionStartOperationsValidateGoInputs(t *testing.T) {
	runtime := newOfflineRuntimeForTest(t)

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
	raw := newCOfflineGeometryRegionDefinition(definition)
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
	if !bytes.Equal(copied.Geometry, definition.Geometry) {
		t.Fatalf("copied geometry = %q, want %q", copied.Geometry, definition.Geometry)
	}
}
