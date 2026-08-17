package maplibre

import (
	"errors"
	"testing"
	"time"
)

func takeStyleOperationForTest[T any](operation *OperationHandle[T], err error) (T, error) {
	var zero T
	if err != nil {
		return zero, err
	}
	defer operation.Release()
	completed, waitErr := operation.Wait(-1)
	if waitErr != nil {
		return zero, waitErr
	}
	if !completed {
		return zero, newBindingError(ErrInvalidState, "style operation wait returned before completion")
	}
	return operation.Take()
}

func takeOptionalStyleOperationForTest[T any](operation *OperationHandle[StyleOptional[T]], err error) (T, bool, error) {
	result, err := takeStyleOperationForTest(operation, err)
	return result.Value, result.Found, err
}

func takeStyleStretchesForTest(operation *OperationHandle[StyleOptional[StyleImageStretchResult]], err error) ([]ImageStretch, []ImageStretch, bool, error) {
	result, found, err := takeOptionalStyleOperationForTest(operation, err)
	return result.X, result.Y, found, err
}

// awaitCommandFinishedForTest drains runtime events until commandID's terminal
// event arrives, returning its payload.
func awaitCommandFinishedForTest(t *testing.T, runtime *RuntimeHandle, commandID uint64, err error) RuntimeEventCommandFinishedPayload {
	t.Helper()
	if err != nil {
		t.Fatalf("command acceptance: %v", err)
	}
	if commandID == 0 {
		t.Fatal("command returned a zero command ID")
	}
	for range make([]struct{}, 5000) {
		batch, drainErr := runtime.DrainEvents()
		if drainErr != nil {
			t.Fatalf("DrainEvents(): %v", drainErr)
		}
		for _, event := range batch.Events {
			finished, ok := event.Payload.(RuntimeEventCommandFinishedPayload)
			if ok && finished.CommandID == commandID {
				return finished
			}
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("command %d did not report a terminal event", commandID)
	return RuntimeEventCommandFinishedPayload{}
}

func requireStyleCommandFailed(t *testing.T, runtime *RuntimeHandle, commandID uint64, err error) {
	t.Helper()
	finished := awaitCommandFinishedForTest(t, runtime, commandID, err)
	if finished.Disposition != CommandDispositionFailed {
		t.Fatalf("command %d disposition = %v, want failed", commandID, finished.Disposition)
	}
	if finished.Err == nil {
		t.Fatalf("command %d failed without a terminal error", commandID)
	}
}

// requireCommandCommitted waits for commandID's terminal event and returns the
// map snapshot generation the commit published.
func requireCommandCommitted(t *testing.T, runtime *RuntimeHandle, commandID uint64, err error) uint64 {
	t.Helper()
	finished := awaitCommandFinishedForTest(t, runtime, commandID, err)
	if finished.Disposition != CommandDispositionCommitted {
		t.Fatalf("command %d disposition = %v, want committed", commandID, finished.Disposition)
	}
	if finished.Err != nil {
		t.Fatalf("command %d committed with error %v, want nil", commandID, finished.Err)
	}
	if finished.Generation == 0 {
		t.Fatalf("command %d committed without publishing a generation", commandID)
	}
	return finished.Generation
}

// requireCommandFailedWith waits for commandID's terminal event and asserts it
// failed with the given binding error.
func requireCommandFailedWith(t *testing.T, runtime *RuntimeHandle, commandID uint64, err, want error) {
	t.Helper()
	finished := awaitCommandFinishedForTest(t, runtime, commandID, err)
	if finished.Disposition != CommandDispositionFailed {
		t.Fatalf("command %d disposition = %v, want failed", commandID, finished.Disposition)
	}
	if !errors.Is(finished.Err, want) {
		t.Fatalf("command %d terminal error = %v, want %v", commandID, finished.Err, want)
	}
}

func TestStyleSourceMetadataForMissingSources(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := takeStyleOperationForTest(m.StartStyleSourceIDs())
	if err != nil {
		t.Fatalf("StyleSourceIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleSourceIDs() unexpectedly contains missing source: %v", ids)
		}
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("missing"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if found || info.Type != StyleSourceTypeUnknown {
		t.Fatalf("StyleSourceInfo(missing) = (%#v, %v), want (unknown type, false)", info, found)
	}
	attribution, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceAttribution("missing"))
	if err != nil {
		t.Fatalf("StyleSourceAttribution(): %v", err)
	}
	if found || attribution != "" {
		t.Fatalf("StyleSourceAttribution(missing) = (%q, %v), want empty false", attribution, found)
	}
	commandID, err := m.RemoveStyleSource("missing")
	requireCommandFailedWith(t, runtime, commandID, err, ErrNotFound)
	if _, _, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("")); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("StyleSourceInfo(empty) error = %v, want ErrInvalidState", err)
	}
}

func TestStyleSourceURLAndTileBindings(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	geoJSONOptions := StyleGeoJSONSourceOptions{}.
		WithMinZoom(1).
		WithTolerance(0.5).
		WithBuffer(64)
	if _, err := m.AddGeoJSONSourceURL("geojson-url", "asset://fixtures/points.geojson", &geoJSONOptions); err != nil {
		t.Fatalf("AddGeoJSONSourceURL(): %v", err)
	}
	if _, err := m.SetGeoJSONSourceURL("geojson-url", "asset://fixtures/points-2.geojson"); err != nil {
		t.Fatalf("SetGeoJSONSourceURL(): %v", err)
	}
	tileOptions := StyleTileSourceOptions{}.
		WithTileSize(256).
		WithAttribution("unit attribution")
	if _, err := m.AddVectorSourceTiles("vector-tiles", []string{"https://example.com/vector/{z}/{x}/{y}.pbf"}, &tileOptions); err != nil {
		t.Fatalf("AddVectorSourceTiles(): %v", err)
	}
	if _, err := m.AddRasterSourceURL("raster-url", "https://example.com/raster.json", &tileOptions); err != nil {
		t.Fatalf("AddRasterSourceURL(): %v", err)
	}
	demOptions := StyleTileSourceOptions{}.
		WithTileSize(512).
		WithRasterEncoding(StyleRasterDEMEncodingTerrarium)
	if _, err := m.AddRasterDEMSourceTiles("dem-tiles", []string{"https://example.com/dem/{z}/{x}/{y}.png"}, &demOptions); err != nil {
		t.Fatalf("AddRasterDEMSourceTiles(): %v", err)
	}
	checks := map[string]StyleSourceType{
		"geojson-url":  StyleSourceTypeGeoJSON,
		"vector-tiles": StyleSourceTypeVector,
		"raster-url":   StyleSourceTypeRaster,
		"dem-tiles":    StyleSourceTypeRasterDEM,
	}
	for id, wantType := range checks {
		info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo(id))
		if err != nil {
			t.Fatalf("StyleSourceInfo(%s): %v", id, err)
		}
		if !found || info.Type != wantType {
			t.Fatalf("StyleSourceInfo(%s) type = (%v, %v), want %v true", id, info.Type, found, wantType)
		}
	}
	commandID, err := m.AddVectorSourceTiles("bad-vector", nil, nil)
	requireStyleCommandFailed(t, runtime, commandID, err)
	commandID, err = m.AddGeoJSONSourceURL("", "asset://fixtures/points.geojson", nil)
	requireStyleCommandFailed(t, runtime, commandID, err)
}

func TestStyleSourceInfoCopiesReconstructibleMetadata(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	minZoom := 0.0
	maxZoom := 14.0
	attribution := "copied attribution"
	scheme := StyleTileSchemeTMS
	bounds := LatLngBounds{
		Southwest: LatLng{Latitude: -5, Longitude: -10},
		Northeast: LatLng{Latitude: 15, Longitude: 20},
	}
	tileSize := uint32(512)
	vectorEncoding := StyleVectorTileEncodingMLT
	options := StyleTileSourceOptions{
		MinZoom:        &minZoom,
		MaxZoom:        &maxZoom,
		Attribution:    &attribution,
		Scheme:         &scheme,
		Bounds:         &bounds,
		TileSize:       &tileSize,
		VectorEncoding: &vectorEncoding,
	}
	tileURLs := []string{
		"https://example.com/first/{z}/{x}/{y}.mlt",
		"https://example.com/second/{z}/{x}/{y}.mlt",
	}
	if _, err := m.AddVectorSourceTiles("inline-vector", tileURLs, &options); err != nil {
		t.Fatalf("AddVectorSourceTiles(): %v", err)
	}

	info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("inline-vector"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(inline-vector): %v", err)
	}
	if !found {
		t.Fatal("StyleSourceInfo(inline-vector) found = false, want true")
	}
	copiedAttribution, attributionFound, err := takeOptionalStyleOperationForTest(m.StartStyleSourceAttribution("inline-vector"))
	if err != nil || !attributionFound {
		t.Fatalf("StyleSourceAttribution(inline-vector) = (%q, %v, %v)", copiedAttribution, attributionFound, err)
	}
	copiedTileURLs, tileURLsFound, err := takeOptionalStyleOperationForTest(m.StartStyleSourceTileURLs("inline-vector"))
	if err != nil || !tileURLsFound {
		t.Fatalf("StyleSourceTileURLs(inline-vector) = (%v, %v, %v)", copiedTileURLs, tileURLsFound, err)
	}
	info.Attribution = &copiedAttribution
	info.TileJSON.TileURLs = copiedTileURLs
	if info.Type != StyleSourceTypeVector || info.URL != nil {
		t.Fatalf("StyleSourceInfo(inline-vector) type/URL = (%v, %v), want vector and absent URL", info.Type, info.URL)
	}
	if info.Attribution == nil || *info.Attribution != attribution {
		t.Fatalf("StyleSourceInfo(inline-vector) attribution = %v, want %q", info.Attribution, attribution)
	}
	if info.TileJSON == nil {
		t.Fatal("StyleSourceInfo(inline-vector) TileJSON = nil, want inline TileJSON")
	}
	if len(info.TileJSON.TileURLs) != len(tileURLs) {
		t.Fatalf("StyleSourceInfo(inline-vector) tile URLs = %v, want %v", info.TileJSON.TileURLs, tileURLs)
	}
	for i := range tileURLs {
		if info.TileJSON.TileURLs[i] != tileURLs[i] {
			t.Fatalf("StyleSourceInfo(inline-vector) tile URL %d = %q, want %q", i, info.TileJSON.TileURLs[i], tileURLs[i])
		}
	}
	if info.TileJSON.MinZoom != minZoom || info.TileJSON.MaxZoom != maxZoom || info.TileJSON.Scheme != scheme {
		t.Fatalf("StyleSourceInfo(inline-vector) TileJSON = %#v, want zooms %v/%v and scheme %v", info.TileJSON, minZoom, maxZoom, scheme)
	}
	if info.TileJSON.Bounds == nil || *info.TileJSON.Bounds != bounds {
		t.Fatalf("StyleSourceInfo(inline-vector) bounds = %v, want %v", info.TileJSON.Bounds, bounds)
	}
	if info.TileSize == nil || *info.TileSize != tileSize {
		t.Fatalf("StyleSourceInfo(inline-vector) tile size = %v, want %d", info.TileSize, tileSize)
	}
	if info.VectorEncoding == nil || *info.VectorEncoding != vectorEncoding {
		t.Fatalf("StyleSourceInfo(inline-vector) vector encoding = %v, want %v", info.VectorEncoding, vectorEncoding)
	}
	if info.RasterEncoding != nil {
		t.Fatalf("StyleSourceInfo(inline-vector) raster encoding = %v, want absent", info.RasterEncoding)
	}

	layerJSON := []byte(`{"id":"inline-vector-layer","type":"line","source":"inline-vector","source-layer":"lines"}`)
	layerID, err := m.AddStyleLayerJSON(layerJSON, "")
	requireCommandCommitted(t, runtime, layerID, err)
	blockedID, err := m.RemoveStyleSource("inline-vector")
	requireCommandFailedWith(t, runtime, blockedID, err, ErrInvalidState)
	removeLayerID, err := m.RemoveStyleLayer("inline-vector-layer")
	requireCommandCommitted(t, runtime, removeLayerID, err)
	removeID, err := m.RemoveStyleSource("inline-vector")
	requireCommandCommitted(t, runtime, removeID, err)
	if _, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("inline-vector")); err != nil || found {
		t.Fatalf("StyleSourceInfo(inline-vector) after removal = (%v, %v), want (false, nil)", found, err)
	}
	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON([]byte(replacement)): %v", err)
	}
	if *info.Attribution != attribution || info.TileJSON.TileURLs[1] != tileURLs[1] || *info.TileJSON.Bounds != bounds {
		t.Fatalf("copied source info changed after removal and style replacement: %#v", info)
	}

	url := "https://example.invalid/vector-tilejson.json"
	if _, err := m.AddVectorSourceURL("url-vector", url, nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}
	urlInfo, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("url-vector"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(url-vector): %v", err)
	}
	copiedURL, urlFound, err := takeOptionalStyleOperationForTest(m.StartStyleSourceURL("url-vector"))
	if err != nil || !urlFound {
		t.Fatalf("StyleSourceURL(url-vector) = (%q, %v, %v)", copiedURL, urlFound, err)
	}
	urlInfo.URL = &copiedURL
	if !found || urlInfo.URL == nil || *urlInfo.URL != url {
		t.Fatalf("StyleSourceInfo(url-vector) URL = (%v, %v), want %q and true", urlInfo.URL, found, url)
	}
	if urlInfo.TileJSON != nil || urlInfo.Attribution != nil {
		t.Fatalf("StyleSourceInfo(url-vector) optional loaded fields = (%v, %v), want absent", urlInfo.TileJSON, urlInfo.Attribution)
	}

	data, err := NewGeoJSONSourceData([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(): %v", err)
	}
	defer func() {
		if err := data.Close(); err != nil {
			t.Errorf("GeoJSONSourceDataHandle Close(): %v", err)
		}
	}()
	if _, err := m.AddGeoJSONSourceData("inline-geojson", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	geoJSONInfo, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("inline-geojson"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(inline-geojson): %v", err)
	}
	if !found || geoJSONInfo.URL != nil || geoJSONInfo.TileJSON != nil {
		t.Fatalf("StyleSourceInfo(inline-geojson) = (%#v, %v), want absent URL and TileJSON", geoJSONInfo, found)
	}
}

func TestGeoJSONSourceDataPrepareAndInstall(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	document := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","geometry":{"type":"LineString","coordinates":[[2,1],[4,3]]},"properties":{"name":"before","rank":7}}]}`)
	options := StyleGeoJSONSourceOptions{}.
		WithMinZoom(1).
		WithMaxZoom(16).
		WithTolerance(0.5).
		WithBuffer(0).
		WithLineMetrics(true).
		WithTileSize(256)
	data, err := NewGeoJSONSourceData(document, &options)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(): %v", err)
	}
	// The document is copied at preparation, so mutating it afterward does not
	// reach the prepared index.
	document[0] = 'x'
	if _, err := m.AddGeoJSONSourceData("geojson-data", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("geojson-data"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(geojson-data): %v", err)
	}
	if !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(geojson-data) type = (%v, %v), want GeoJSON true", info.Type, found)
	}

	// One prepared handle installs on any number of sources.
	if _, err := m.AddGeoJSONSourceData("geojson-data-2", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(reused handle): %v", err)
	}

	// A set requires data prepared with the source's options.
	update, err := NewGeoJSONSourceData([]byte(`{"type":"Point","coordinates":[6,5]}`), &options)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(update): %v", err)
	}
	commandID, err := m.SetGeoJSONSourceData("geojson-data", update)
	requireCommandCommitted(t, runtime, commandID, err)
	commandID, err = m.SetGeoJSONSourceData("geojson-data-2", update)
	requireCommandCommitted(t, runtime, commandID, err)
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}

	// Data prepared under different options tiles inconsistently with the
	// source, so the install is rejected.
	mismatchedOptions := options.WithTolerance(0.25)
	mismatched, err := NewGeoJSONSourceData([]byte(`{"type":"Point","coordinates":[6,5]}`), &mismatchedOptions)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(mismatched): %v", err)
	}
	commandID, err = m.SetGeoJSONSourceData("geojson-data", mismatched)
	requireCommandFailedWith(t, runtime, commandID, err, ErrInvalidArgument)
	if err := mismatched.Close(); err != nil {
		t.Fatalf("mismatched Close(): %v", err)
	}

	// Closing the handle never invalidates a source it was installed on, and a
	// closed handle reports the binding's closed-handle error before crossing
	// into C.
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	if err := data.Close(); err != nil {
		t.Fatalf("second data Close(): %v", err)
	}
	if commandID, err := m.AddGeoJSONSourceData("closed-handle", data); commandID != 0 || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(closed handle) = (%d, %v), want 0 and ErrInvalidArgument", commandID, err)
	}
	if commandID, err := m.SetGeoJSONSourceData("geojson-data", data); commandID != 0 || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetGeoJSONSourceData(closed handle) = (%d, %v), want 0 and ErrInvalidArgument", commandID, err)
	}
	info, found, err = takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("geojson-data"))
	if err != nil || !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(geojson-data) after handle close = (%v, %v, %v), want GeoJSON true nil", info.Type, found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("closed-handle")); err != nil || found {
		t.Fatalf("StyleSourceInfo(closed-handle) = (%v, %v), want (false, nil)", found, err)
	}
}

func TestGeoJSONSourceDataRejectsInvalidDocumentsAtPreparation(t *testing.T) {
	badID := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":{},"geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}]}`)
	if _, err := NewGeoJSONSourceData(badID, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewGeoJSONSourceData(unsupported id) error = %v, want ErrInvalidArgument", err)
	}
	badGeometry := []byte(`{"type":"Unsupported","coordinates":[]}`)
	if _, err := NewGeoJSONSourceData(badGeometry, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewGeoJSONSourceData(unsupported geometry) error = %v, want ErrInvalidArgument", err)
	}
	badClusterProperties := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterProperties([]byte(`{"total":NaN}`))
	points := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"rank":1}}]}`)
	if _, err := NewGeoJSONSourceData(points, &badClusterProperties); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewGeoJSONSourceData(non-finite cluster property) error = %v, want ErrInvalidArgument", err)
	}
	// Clustering requires a feature collection of point features.
	clustered := StyleGeoJSONSourceOptions{}.WithCluster(true)
	if _, err := NewGeoJSONSourceData([]byte(`{"type":"Point","coordinates":[0,0]}`), &clustered); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewGeoJSONSourceData(clustered bare geometry) error = %v, want ErrInvalidArgument", err)
	}
	lines := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"LineString","coordinates":[[0,0],[1,1]]},"properties":{}}]}`)
	if _, err := NewGeoJSONSourceData(lines, &clustered); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewGeoJSONSourceData(clustered non-point feature) error = %v, want ErrInvalidArgument", err)
	}
}

func TestGeoJSONSourceClusterOptions(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	points := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"rank":1}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"rank":2}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"rank":3}}]}`)
	clusterProperties := []byte(`{"total":["+",["get","rank"]]}`)
	options := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterRadius(50).
		WithClusterMinPoints(2).
		WithClusterMaxZoom(14).
		WithClusterProperties(clusterProperties)
	data, err := NewGeoJSONSourceData(points, &options)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(clustered): %v", err)
	}
	clusterProperties[0] = 'x'
	if _, err := m.AddGeoJSONSourceData("cluster-source", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(clustered): %v", err)
	}
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("cluster-source"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(cluster-source): %v", err)
	}
	if !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(cluster-source) type = (%v, %v), want GeoJSON true", info.Type, found)
	}
	// Different cluster aggregations would change cluster feature properties
	// under the source's layers, so the options match rejects them.
	updatedProperties := options.WithClusterProperties([]byte(`{"top":["max",["get","rank"]]}`))
	update, err := NewGeoJSONSourceData(points, &updatedProperties)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(updated cluster properties): %v", err)
	}
	commandID, err := m.SetGeoJSONSourceData("cluster-source", update)
	requireCommandFailedWith(t, runtime, commandID, err, ErrInvalidArgument)
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}
	malformed := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterProperties([]byte(`{"total":["+"]}`))
	if _, err := NewGeoJSONSourceData(points, &malformed); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewGeoJSONSourceData(malformed cluster properties) error = %v, want ErrInvalidArgument", err)
	}
	emptyClusterProperties := []struct {
		name    string
		options StyleGeoJSONSourceOptions
	}{
		{
			name: "builder",
			options: StyleGeoJSONSourceOptions{}.
				WithCluster(true).
				WithClusterProperties([]byte{}),
		},
		{
			name: "clone",
			options: StyleGeoJSONSourceOptions{
				Cluster:           optionPtr(true),
				ClusterProperties: []byte{},
			}.Clone(),
		},
	}
	for _, test := range emptyClusterProperties {
		if _, err := NewGeoJSONSourceData(points, &test.options); !errors.Is(err, ErrInvalidArgument) {
			t.Errorf("NewGeoJSONSourceData(%s empty cluster properties) error = %v, want ErrInvalidArgument", test.name, err)
		}
	}
}

// Preparation touches no runtime or map, so a plain goroutine prepares data
// that installs on the map owner thread, and another goroutine releases it.
func TestGeoJSONSourceDataPreparesOnAnotherGoroutine(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}

	type prepared struct {
		data *GeoJSONSourceDataHandle
		err  error
	}
	results := make(chan prepared)
	go func() {
		data, err := NewGeoJSONSourceData([]byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[1,2]},"properties":{}}]}`), nil)
		results <- prepared{data: data, err: err}
	}()
	result := <-results
	if result.err != nil {
		t.Fatalf("NewGeoJSONSourceData() on a goroutine: %v", result.err)
	}
	commandID, err := m.AddGeoJSONSourceData("worker-prepared", result.data)
	requireCommandCommitted(t, runtime, commandID, err)
	closed := make(chan error)
	go func() {
		closed <- result.data.Close()
	}()
	if err := <-closed; err != nil {
		t.Fatalf("Close() on a goroutine: %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("worker-prepared"))
	if err != nil || !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(worker-prepared) = (%v, %v, %v), want GeoJSON true nil", info.Type, found, err)
	}
}

func TestGeoJSONSourceSynchronousTilingOverride(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	data, err := NewGeoJSONSourceData([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(): %v", err)
	}
	if _, err := m.AddGeoJSONSourceData("tracked", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	commandID, err := m.SetGeoJSONSourceSynchronousTiling("tracked", true)
	requireCommandCommitted(t, runtime, commandID, err)
	update, err := NewGeoJSONSourceData([]byte(`{"type":"Point","coordinates":[3,4]}`), nil)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(update): %v", err)
	}
	commandID, err = m.SetGeoJSONSourceData("tracked", update)
	requireCommandCommitted(t, runtime, commandID, err)
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}
	commandID, err = m.SetGeoJSONSourceSynchronousTiling("tracked", false)
	requireCommandCommitted(t, runtime, commandID, err)
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	commandID, err = m.SetGeoJSONSourceSynchronousTiling("missing", true)
	requireCommandFailedWith(t, runtime, commandID, err, ErrInvalidArgument)
	if _, err := m.AddVectorSourceURL("vector", "https://example.invalid/tiles.json", nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}
	commandID, err = m.SetGeoJSONSourceSynchronousTiling("vector", true)
	requireCommandFailedWith(t, runtime, commandID, err, ErrInvalidArgument)
}

func TestAddStyleSourceJSONCopiesGoBuffer(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	source := []byte(`{"type":"geojson","data":{"type":"FeatureCollection","features":[]},"attribution":"unit-test"}`)
	if _, err := m.AddStyleSourceJSON("go-json-source", source); err != nil {
		t.Fatalf("AddStyleSourceJSON(): %v", err)
	}
	source[0] = 'x'
	info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("go-json-source"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(go-json-source) type = (%v, %v), want GeoJSON true", info.Type, found)
	}
	if info.IDSize != uint64(len("go-json-source")) {
		t.Fatalf("StyleSourceInfo(go-json-source) = %#v, want copied ID size", info)
	}
	commandID, err := m.AddStyleSourceJSON("bad-json-source", []byte(`NaN`))
	requireStyleCommandFailed(t, runtime, commandID, err)
}
