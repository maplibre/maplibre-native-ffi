package maplibre

import (
	"errors"
	"slices"
	"testing"
)

func takeOptionalStyleOperationForTest[T any](future *Future[StyleOptional[T]], err error) (T, bool, error) {
	result, err := awaitForTest(future, err)
	return result.Value, result.Found, err
}

func awaitCommandCompletionForTest(t *testing.T, future *Future[CommandCompletion], err error) CommandCompletion {
	t.Helper()
	result, err := awaitForTest(future, err)
	if err != nil {
		t.Fatalf("command completion: %v", err)
	}
	return result
}

func requireStyleCommandFailed(t *testing.T, future *Future[CommandCompletion], err error) {
	t.Helper()
	completion, completionErr := awaitForTest(future, err)
	if completionErr != nil {
		t.Fatalf("command completion: %v", completionErr)
	}
	if completion.Disposition != CommandDispositionFailed {
		t.Fatalf("command disposition = %v, want failed", completion.Disposition)
	}
}

// requireCommandCommitted waits for completion's terminal event and returns the
// map snapshot generation the commit published.
func requireCommandCommitted(t *testing.T, future *Future[CommandCompletion], err error) uint64 {
	t.Helper()
	finished := awaitCommandCompletionForTest(t, future, err)
	if finished.Disposition != CommandDispositionCommitted {
		t.Fatalf("command disposition = %v, want committed", finished.Disposition)
	}
	if finished.Generation == 0 {
		t.Fatal("command committed without publishing a generation")
	}
	return finished.Generation
}

// requireCommandFailedWith waits for completion's terminal event and asserts it
// failed with the given binding error.
func requireCommandFailedWith(t *testing.T, future *Future[CommandCompletion], err, want error) {
	t.Helper()
	completion, completionErr := awaitForTest(future, err)
	if completionErr != nil {
		t.Fatalf("command completion: %v", completionErr)
	}
	if completion.Disposition != CommandDispositionFailed {
		t.Fatalf("command disposition = %v, want failed", completion.Disposition)
	}
	got := kindForStatus(completion.RawStatus)
	if !errors.Is(got, want) {
		t.Fatalf("command terminal status = %v, want %v", got, want)
	}
}

func TestStyleSourceMetadataForMissingSources(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := awaitForTest(m.StyleSourceIDs())
	if err != nil {
		t.Fatalf("StyleSourceIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleSourceIDs() unexpectedly contains missing source: %v", ids)
		}
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("missing"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if found || info.Type != StyleSourceTypeUnknown {
		t.Fatalf("StyleSourceInfo(missing) = (%#v, %v), want (unknown type, false)", info, found)
	}
	if info.Attribution != nil {
		t.Fatalf("StyleSourceInfo(missing) attribution = %v, want absent", info.Attribution)
	}
	completion, err := m.RemoveStyleSource("missing")
	requireCommandFailedWith(t, completion, err, ErrNotFound)
	if _, _, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("")); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StyleSourceInfo(empty) error = %v, want ErrInvalidArgument", err)
	}
	volatileCompletion, err := m.SetStyleSourceVolatile("missing", true)
	requireCommandFailedWith(t, volatileCompletion, err, ErrNotFound)
}

func TestStyleSourceVolatilityRoundTripsThroughPublicAPI(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if _, err := m.AddVectorSourceURL("volatile-source", "https://example.invalid/tiles.json", nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}

	info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("volatile-source"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(initial): %v", err)
	}
	if !found {
		t.Fatal("StyleSourceInfo(initial) found = false, want true")
	}
	if info.IsVolatile {
		t.Fatal("StyleSourceInfo(initial).IsVolatile = true, want false")
	}

	enabled, err := m.SetStyleSourceVolatile("volatile-source", true)
	requireCommandCommitted(t, enabled, err)
	info, found, err = takeOptionalStyleOperationForTest(m.StyleSourceInfo("volatile-source"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(true): %v", err)
	}
	if !found || !info.IsVolatile {
		t.Fatalf("StyleSourceInfo(true) = (%#v, %v), want found and volatile", info, found)
	}

	disabled, err := m.SetStyleSourceVolatile("volatile-source", false)
	requireCommandCommitted(t, disabled, err)
	info, found, err = takeOptionalStyleOperationForTest(m.StyleSourceInfo("volatile-source"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(false): %v", err)
	}
	if !found || info.IsVolatile {
		t.Fatalf("StyleSourceInfo(false) = (%#v, %v), want found and non-volatile", info, found)
	}
}

func TestStyleSourceURLAndTileBindings(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
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
		info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo(id))
		if err != nil {
			t.Fatalf("StyleSourceInfo(%s): %v", id, err)
		}
		if !found || info.Type != wantType {
			t.Fatalf("StyleSourceInfo(%s) type = (%v, %v), want %v true", id, info.Type, found, wantType)
		}
	}
	completion, err := m.AddVectorSourceTiles("bad-vector", nil, nil)
	requireStyleCommandFailed(t, completion, err)
	completion, err = m.AddGeoJSONSourceURL("", "asset://fixtures/points.geojson", nil)
	requireStyleCommandFailed(t, completion, err)
}

func TestStyleSourceInfoCopiesReconstructibleMetadata(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
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

	info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("inline-vector"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(inline-vector): %v", err)
	}
	if !found {
		t.Fatal("StyleSourceInfo(inline-vector) found = false, want true")
	}
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

	// The narrow copies report the same values the aggregate carries.
	copiedAttribution, found, err := takeOptionalStyleOperationForTest(m.StyleSourceAttribution("inline-vector"))
	if err != nil || !found || copiedAttribution != attribution {
		t.Fatalf("StyleSourceAttribution(inline-vector) = (%q, %v, %v), want %q", copiedAttribution, found, err, attribution)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleSourceURL("inline-vector")); err != nil || found {
		t.Fatalf("StyleSourceURL(inline-vector) = (%v, %v), want (false, nil) for an inline source", found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleSourceAttribution("missing")); err != nil || found {
		t.Fatalf("StyleSourceAttribution(missing) = (%v, %v), want (false, nil)", found, err)
	}
	copiedTileURLs, err := awaitForTest(m.StyleSourceTileURLs("inline-vector"))
	if err != nil || !slices.Equal(copiedTileURLs, tileURLs) {
		t.Fatalf("StyleSourceTileURLs(inline-vector) = (%q, %v), want %q", copiedTileURLs, err, tileURLs)
	}
	if missingTileURLs, err := awaitForTest(m.StyleSourceTileURLs("missing")); err != nil || len(missingTileURLs) != 0 {
		t.Fatalf("StyleSourceTileURLs(missing) = (%q, %v), want an empty list", missingTileURLs, err)
	}

	layerJSON := []byte(`{"id":"inline-vector-layer","type":"line","source":"inline-vector","source-layer":"lines"}`)
	layerID, err := m.AddStyleLayerJSON(layerJSON, "")
	requireCommandCommitted(t, layerID, err)
	blockedID, err := m.RemoveStyleSource("inline-vector")
	requireCommandFailedWith(t, blockedID, err, ErrInvalidState)
	removeLayerID, err := m.RemoveStyleLayer("inline-vector-layer")
	requireCommandCommitted(t, removeLayerID, err)
	removeID, err := m.RemoveStyleSource("inline-vector")
	requireCommandCommitted(t, removeID, err)
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("inline-vector")); err != nil || found {
		t.Fatalf("StyleSourceInfo(inline-vector) after removal = (%v, %v), want (false, nil)", found, err)
	}
	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON([]byte(replacement)): %v", err)
	}
	if *info.Attribution != attribution || info.TileJSON.TileURLs[1] != tileURLs[1] || *info.TileJSON.Bounds != bounds {
		t.Fatalf("copied source info changed after removal and style replacement: %#v", info)
	}

	url := "https://example.invalid/vector-tilejson.json"
	if _, err := m.AddVectorSourceURL("url-vector", url, nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}
	urlInfo, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("url-vector"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(url-vector): %v", err)
	}
	if !found || urlInfo.URL == nil || *urlInfo.URL != url {
		t.Fatalf("StyleSourceInfo(url-vector) URL = (%v, %v), want %q and true", urlInfo.URL, found, url)
	}
	if urlInfo.TileJSON != nil || urlInfo.Attribution != nil {
		t.Fatalf("StyleSourceInfo(url-vector) optional loaded fields = (%v, %v), want absent", urlInfo.TileJSON, urlInfo.Attribution)
	}
	copiedURL, found, err := takeOptionalStyleOperationForTest(m.StyleSourceURL("url-vector"))
	if err != nil || !found || copiedURL != url {
		t.Fatalf("StyleSourceURL(url-vector) = (%q, %v, %v), want %q", copiedURL, found, err, url)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleSourceURL("missing")); err != nil || found {
		t.Fatalf("StyleSourceURL(missing) = (%v, %v), want (false, nil)", found, err)
	}
	if urlTileURLs, err := awaitForTest(m.StyleSourceTileURLs("url-vector")); err != nil || len(urlTileURLs) != 0 {
		t.Fatalf("StyleSourceTileURLs(url-vector) = (%q, %v), want an empty list for a URL-backed source", urlTileURLs, err)
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
	geoJSONInfo, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("inline-geojson"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(inline-geojson): %v", err)
	}
	if !found || geoJSONInfo.URL != nil || geoJSONInfo.TileJSON != nil {
		t.Fatalf("StyleSourceInfo(inline-geojson) = (%#v, %v), want absent URL and TileJSON", geoJSONInfo, found)
	}
}

func TestGeoJSONSourceDataPrepareAndInstall(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
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
	info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("geojson-data"))
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
	completion, err := m.SetGeoJSONSourceData("geojson-data", update)
	requireCommandCommitted(t, completion, err)
	completion, err = m.SetGeoJSONSourceData("geojson-data-2", update)
	requireCommandCommitted(t, completion, err)
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
	completion, err = m.SetGeoJSONSourceData("geojson-data", mismatched)
	requireCommandFailedWith(t, completion, err, ErrInvalidArgument)
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
	if future, err := m.AddGeoJSONSourceData("closed-handle", data); future != nil || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(closed handle) = (%v, %v), want nil and ErrInvalidArgument", future, err)
	}
	if future, err := m.SetGeoJSONSourceData("geojson-data", data); future != nil || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetGeoJSONSourceData(closed handle) = (%v, %v), want nil and ErrInvalidArgument", future, err)
	}
	info, found, err = takeOptionalStyleOperationForTest(m.StyleSourceInfo("geojson-data"))
	if err != nil || !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(geojson-data) after handle close = (%v, %v, %v), want GeoJSON true nil", info.Type, found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("closed-handle")); err != nil || found {
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
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
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
	info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("cluster-source"))
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
	completion, err := m.SetGeoJSONSourceData("cluster-source", update)
	requireCommandFailedWith(t, completion, err, ErrInvalidArgument)
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

// Preparation touches no runtime or map, so one goroutine prepares data,
// another installs it on the map, and a third releases it.
func TestGeoJSONSourceDataPreparesOnAnotherGoroutine(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
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
	completion, err := m.AddGeoJSONSourceData("worker-prepared", result.data)
	requireCommandCommitted(t, completion, err)
	closed := make(chan error)
	go func() {
		closed <- result.data.Close()
	}()
	if err := <-closed; err != nil {
		t.Fatalf("Close() on a goroutine: %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("worker-prepared"))
	if err != nil || !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(worker-prepared) = (%v, %v, %v), want GeoJSON true nil", info.Type, found, err)
	}
}

func TestGeoJSONSourceSynchronousTilingOverride(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	data, err := NewGeoJSONSourceData([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(): %v", err)
	}
	if _, err := m.AddGeoJSONSourceData("tracked", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	completion, err := m.SetGeoJSONSourceSynchronousTiling("tracked", true)
	requireCommandCommitted(t, completion, err)
	update, err := NewGeoJSONSourceData([]byte(`{"type":"Point","coordinates":[3,4]}`), nil)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(update): %v", err)
	}
	completion, err = m.SetGeoJSONSourceData("tracked", update)
	requireCommandCommitted(t, completion, err)
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}
	completion, err = m.SetGeoJSONSourceSynchronousTiling("tracked", false)
	requireCommandCommitted(t, completion, err)
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	// A missing source is not found, while a source of another type is a type
	// mismatch the command reports as an invalid argument.
	completion, err = m.SetGeoJSONSourceSynchronousTiling("missing", true)
	requireCommandFailedWith(t, completion, err, ErrNotFound)
	if _, err := m.AddVectorSourceURL("vector", "https://example.invalid/tiles.json", nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}
	completion, err = m.SetGeoJSONSourceSynchronousTiling("vector", true)
	requireCommandFailedWith(t, completion, err, ErrInvalidArgument)
}

func TestAddStyleSourceJSONCopiesGoBuffer(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	source := []byte(`{"type":"geojson","data":{"type":"FeatureCollection","features":[]},"attribution":"unit-test"}`)
	if _, err := m.AddStyleSourceJSON("go-json-source", source); err != nil {
		t.Fatalf("AddStyleSourceJSON(): %v", err)
	}
	source[0] = 'x'
	info, found, err := takeOptionalStyleOperationForTest(m.StyleSourceInfo("go-json-source"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if !found || info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceInfo(go-json-source) type = (%v, %v), want GeoJSON true", info.Type, found)
	}
	completion, err := m.AddStyleSourceJSON("bad-json-source", []byte(`NaN`))
	requireStyleCommandFailed(t, completion, err)
}
