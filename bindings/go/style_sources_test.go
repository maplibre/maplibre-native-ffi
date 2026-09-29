package maplibre

import (
	"errors"
	"slices"
	"testing"
)

func takeOptionalStyleOperationForTest[T any](future *Future[*T], err error) (T, bool, error) {
	result, err := awaitForTest(future, err)
	var zero T
	if result == nil {
		return zero, false, err
	}
	return *result, true, err
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

	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(empty style): %v", err)
	}
	ids, err := awaitForTest(m.ListStyleSourceIds())
	if err != nil {
		t.Fatalf("StyleSourceIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleSourceIDs() unexpectedly contains missing source: %v", ids)
		}
	}
	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("missing"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(): %v", err)
	}
	if found || info.Info.Type != StyleSourceTypeUnknown {
		t.Fatalf("GetStyleSourceInfo(missing) = (%#v, %v), want (unknown type, false)", info, found)
	}
	if info.Attribution != nil {
		t.Fatalf("GetStyleSourceInfo(missing) attribution = %v, want absent", info.Attribution)
	}
	completion, err := m.RemoveStyleSource("missing")
	requireCommandFailedWith(t, completion, err, ErrNotFound)
	if _, _, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("")); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GetStyleSourceInfo(empty) error = %v, want ErrInvalidArgument", err)
	}
	volatileCompletion, err := m.SetStyleSourceVolatile("missing", true)
	requireCommandFailedWith(t, volatileCompletion, err, ErrNotFound)
}

func TestStyleSourceVolatilityRoundTripsThroughPublicAPI(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(empty style): %v", err)
	}
	if _, err := m.AddVectorSourceUrl("volatile-source", "https://example.invalid/tiles.json", nil); err != nil {
		t.Fatalf("AddVectorSourceUrl(): %v", err)
	}

	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("volatile-source"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(initial): %v", err)
	}
	if !found {
		t.Fatal("GetStyleSourceInfo(initial) found = false, want true")
	}
	if info.Info.IsVolatile {
		t.Fatal("GetStyleSourceInfo(initial).IsVolatile = true, want false")
	}

	enabled, err := m.SetStyleSourceVolatile("volatile-source", true)
	requireCommandCommitted(t, enabled, err)
	info, found, err = takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("volatile-source"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(true): %v", err)
	}
	if !found || !info.Info.IsVolatile {
		t.Fatalf("GetStyleSourceInfo(true) = (%#v, %v), want found and volatile", info, found)
	}

	disabled, err := m.SetStyleSourceVolatile("volatile-source", false)
	requireCommandCommitted(t, disabled, err)
	info, found, err = takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("volatile-source"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(false): %v", err)
	}
	if !found || info.Info.IsVolatile {
		t.Fatalf("GetStyleSourceInfo(false) = (%#v, %v), want found and non-volatile", info, found)
	}
}

func TestStyleSourceURLAndTileBindings(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(empty style): %v", err)
	}
	geoJSONOptions := GeojsonSourceOptions{MinZoom: pointerTo(float64(1)), Tolerance: pointerTo(0.5), Buffer: pointerTo(uint32(64))}
	if _, err := m.AddGeojsonSourceUrl("geojson-url", "asset://fixtures/points.geojson", &geoJSONOptions); err != nil {
		t.Fatalf("AddGeojsonSourceUrl(): %v", err)
	}
	if _, err := m.SetGeojsonSourceUrl("geojson-url", "asset://fixtures/points-2.geojson"); err != nil {
		t.Fatalf("SetGeojsonSourceUrl(): %v", err)
	}
	tileOptions := StyleTileSourceOptions{TileSize: pointerTo(uint32(256)), Attribution: pointerTo("unit attribution")}
	if _, err := m.AddVectorSourceTiles("vector-tiles", []string{"https://example.com/vector/{z}/{x}/{y}.pbf"}, &tileOptions); err != nil {
		t.Fatalf("AddVectorSourceTiles(): %v", err)
	}
	if _, err := m.AddRasterSourceUrl("raster-url", "https://example.com/raster.json", &tileOptions); err != nil {
		t.Fatalf("AddRasterSourceUrl(): %v", err)
	}
	demOptions := StyleTileSourceOptions{TileSize: pointerTo(uint32(512)), RasterEncoding: pointerTo(StyleRasterDemEncodingTerrarium)}
	if _, err := m.AddRasterDemSourceTiles("dem-tiles", []string{"https://example.com/dem/{z}/{x}/{y}.png"}, &demOptions); err != nil {
		t.Fatalf("AddRasterDemSourceTiles(): %v", err)
	}
	checks := map[string]StyleSourceType{
		"geojson-url":  StyleSourceTypeGeojson,
		"vector-tiles": StyleSourceTypeVector,
		"raster-url":   StyleSourceTypeRaster,
		"dem-tiles":    StyleSourceTypeRasterDem,
	}
	for id, wantType := range checks {
		info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo(id))
		if err != nil {
			t.Fatalf("GetStyleSourceInfo(%s): %v", id, err)
		}
		if !found || info.Info.Type != wantType {
			t.Fatalf("GetStyleSourceInfo(%s) type = (%v, %v), want %v true", id, info.Info.Type, found, wantType)
		}
	}
	completion, err := m.AddVectorSourceTiles("bad-vector", nil, nil)
	requireStyleCommandFailed(t, completion, err)
	completion, err = m.AddGeojsonSourceUrl("", "asset://fixtures/points.geojson", nil)
	requireStyleCommandFailed(t, completion, err)
}

func TestStyleSourceInfoCopiesReconstructibleMetadata(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(empty style): %v", err)
	}
	minZoom := 0.0
	maxZoom := 14.0
	attribution := "copied attribution"
	scheme := StyleTileSchemeTms
	bounds := LatLngBounds{
		Southwest: LatLng{Latitude: -5, Longitude: -10},
		Northeast: LatLng{Latitude: 15, Longitude: 20},
	}
	tileSize := uint32(512)
	vectorEncoding := StyleVectorTileEncodingMlt
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

	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("inline-vector"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(inline-vector): %v", err)
	}
	if !found {
		t.Fatal("GetStyleSourceInfo(inline-vector) found = false, want true")
	}
	if info.Info.Type != StyleSourceTypeVector || info.Url != nil {
		t.Fatalf("GetStyleSourceInfo(inline-vector) type/URL = (%v, %v), want vector and absent URL", info.Info.Type, info.Url)
	}
	if info.Attribution == nil || *info.Attribution != attribution {
		t.Fatalf("GetStyleSourceInfo(inline-vector) attribution = %v, want %q", info.Attribution, attribution)
	}
	if info.Info.Tilejson == nil {
		t.Fatal("GetStyleSourceInfo(inline-vector) TileJSON = nil, want inline TileJSON")
	}
	if len(info.TileUrls) != len(tileURLs) {
		t.Fatalf("GetStyleSourceInfo(inline-vector) tile URLs = %v, want %v", info.TileUrls, tileURLs)
	}
	for i := range tileURLs {
		if info.TileUrls[i] != tileURLs[i] {
			t.Fatalf("GetStyleSourceInfo(inline-vector) tile URL %d = %q, want %q", i, info.TileUrls[i], tileURLs[i])
		}
	}
	if info.Info.Tilejson.MinZoom != minZoom || info.Info.Tilejson.MaxZoom != maxZoom || info.Info.Tilejson.Scheme != scheme {
		t.Fatalf("GetStyleSourceInfo(inline-vector) TileJSON = %#v, want zooms %v/%v and scheme %v", info.Info.Tilejson, minZoom, maxZoom, scheme)
	}
	if info.Info.Bounds == nil || *info.Info.Bounds != bounds {
		t.Fatalf("GetStyleSourceInfo(inline-vector) bounds = %v, want %v", info.Info.Bounds, bounds)
	}
	if info.Info.TileSize == nil || *info.Info.TileSize != tileSize {
		t.Fatalf("GetStyleSourceInfo(inline-vector) tile size = %v, want %d", info.Info.TileSize, tileSize)
	}
	if info.Info.VectorEncoding == nil || *info.Info.VectorEncoding != vectorEncoding {
		t.Fatalf("GetStyleSourceInfo(inline-vector) vector encoding = %v, want %v", info.Info.VectorEncoding, vectorEncoding)
	}
	if info.Info.RasterEncoding != nil {
		t.Fatalf("GetStyleSourceInfo(inline-vector) raster encoding = %v, want absent", info.Info.RasterEncoding)
	}

	// The narrow copies report the same values the aggregate carries.
	copiedAttribution, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceAttribution("inline-vector"))
	if err != nil || !found || copiedAttribution != attribution {
		t.Fatalf("StyleSourceAttribution(inline-vector) = (%q, %v, %v), want %q", copiedAttribution, found, err, attribution)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceUrl("inline-vector")); err != nil || found {
		t.Fatalf("StyleSourceURL(inline-vector) = (%v, %v), want (false, nil) for an inline source", found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceAttribution("missing")); err != nil || found {
		t.Fatalf("StyleSourceAttribution(missing) = (%v, %v), want (false, nil)", found, err)
	}
	copiedTileURLs, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceTileUrls("inline-vector"))
	if err != nil || !found || !slices.Equal(copiedTileURLs.TileUrls, tileURLs) {
		t.Fatalf("GetStyleSourceTileUrls(inline-vector) = (%q, %v, %v), want %q and true", copiedTileURLs, found, err, tileURLs)
	}
	if missingTileURLs, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceTileUrls("missing")); err != nil || found {
		t.Fatalf("GetStyleSourceTileUrls(missing) = (%q, %v, %v), want (false, nil)", missingTileURLs, found, err)
	}

	layerJSON := []byte(`{"id":"inline-vector-layer","type":"line","source":"inline-vector","source-layer":"lines"}`)
	layerID, err := m.AddStyleLayerJson(layerJSON, nil)
	requireCommandCommitted(t, layerID, err)
	blockedID, err := m.RemoveStyleSource("inline-vector")
	requireCommandFailedWith(t, blockedID, err, ErrInvalidState)
	removeLayerID, err := m.RemoveStyleLayer("inline-vector-layer")
	requireCommandCommitted(t, removeLayerID, err)
	removeID, err := m.RemoveStyleSource("inline-vector")
	requireCommandCommitted(t, removeID, err)
	if _, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("inline-vector")); err != nil || found {
		t.Fatalf("GetStyleSourceInfo(inline-vector) after removal = (%v, %v), want (false, nil)", found, err)
	}
	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson([]byte(replacement)): %v", err)
	}
	if *info.Attribution != attribution || info.TileUrls[1] != tileURLs[1] || *info.Info.Bounds != bounds {
		t.Fatalf("copied source info changed after removal and style replacement: %#v", info)
	}

	url := "https://example.invalid/vector-tilejson.json"
	if _, err := m.AddVectorSourceUrl("url-vector", url, nil); err != nil {
		t.Fatalf("AddVectorSourceUrl(): %v", err)
	}
	urlInfo, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("url-vector"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(url-vector): %v", err)
	}
	if !found || urlInfo.Url == nil || *urlInfo.Url != url {
		t.Fatalf("GetStyleSourceInfo(url-vector) URL = (%v, %v), want %q and true", urlInfo.Url, found, url)
	}
	if urlInfo.Info.Tilejson != nil || urlInfo.Attribution != nil {
		t.Fatalf("GetStyleSourceInfo(url-vector) optional loaded fields = (%v, %v), want absent", urlInfo.Info.Tilejson, urlInfo.Attribution)
	}
	copiedURL, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceUrl("url-vector"))
	if err != nil || !found || copiedURL != url {
		t.Fatalf("StyleSourceURL(url-vector) = (%q, %v, %v), want %q", copiedURL, found, err, url)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceUrl("missing")); err != nil || found {
		t.Fatalf("StyleSourceURL(missing) = (%v, %v), want (false, nil)", found, err)
	}
	if urlTileURLs, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceTileUrls("url-vector")); err != nil || !found || len(urlTileURLs.TileUrls) != 0 {
		t.Fatalf("GetStyleSourceTileUrls(url-vector) = (%q, %v, %v), want an empty list and true for a URL-backed source", urlTileURLs, found, err)
	}

	data, err := GeojsonSourceDataCreate([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatalf("GeojsonSourceDataCreate(): %v", err)
	}
	defer func() {
		if err := data.Close(); err != nil {
			t.Errorf("GeoJSONSourceDataHandle Close(): %v", err)
		}
	}()
	if _, err := m.AddGeojsonSourceData("inline-geojson", data); err != nil {
		t.Fatalf("AddGeojsonSourceData(): %v", err)
	}
	geoJSONInfo, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("inline-geojson"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(inline-geojson): %v", err)
	}
	if !found || geoJSONInfo.Url != nil || geoJSONInfo.Info.Tilejson != nil {
		t.Fatalf("GetStyleSourceInfo(inline-geojson) = (%#v, %v), want absent URL and TileJSON", geoJSONInfo, found)
	}
}

func TestGeoJSONSourceDataPrepareAndInstall(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)
	for _, invalid := range []*GeojsonSourceDataHandle{nil, {}} {
		if future, err := m.AddGeojsonSourceData("invalid", invalid); future != nil || !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("nil or zero input owner: future=%v error=%v", future, err)
		}
	}

	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(empty style): %v", err)
	}
	document := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","geometry":{"type":"LineString","coordinates":[[2,1],[4,3]]},"properties":{"name":"before","rank":7}}]}`)
	options := GeojsonSourceOptions{MinZoom: pointerTo(float64(1)), MaxZoom: pointerTo(float64(16)), Tolerance: pointerTo(0.5), Buffer: pointerTo(uint32(0)), LineMetrics: pointerTo(true), TileSize: pointerTo(uint32(256))}
	data, err := GeojsonSourceDataCreate(document, &options)
	if err != nil {
		t.Fatalf("GeojsonSourceDataCreate(): %v", err)
	}
	// The document is copied at preparation, so mutating it afterward does not
	// reach the prepared index.
	document[0] = 'x'
	if _, err := m.AddGeojsonSourceData("geojson-data", data); err != nil {
		t.Fatalf("AddGeojsonSourceData(): %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("geojson-data"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(geojson-data): %v", err)
	}
	if !found || info.Info.Type != StyleSourceTypeGeojson {
		t.Fatalf("GetStyleSourceInfo(geojson-data) type = (%v, %v), want GeoJSON true", info.Info.Type, found)
	}

	// One prepared handle installs on any number of sources.
	if _, err := m.AddGeojsonSourceData("geojson-data-2", data); err != nil {
		t.Fatalf("AddGeojsonSourceData(reused handle): %v", err)
	}

	// A set requires data prepared with the source's options.
	update, err := GeojsonSourceDataCreate([]byte(`{"type":"Point","coordinates":[6,5]}`), &options)
	if err != nil {
		t.Fatalf("GeojsonSourceDataCreate(update): %v", err)
	}
	completion, err := m.SetGeojsonSourceData("geojson-data", update)
	requireCommandCommitted(t, completion, err)
	completion, err = m.SetGeojsonSourceData("geojson-data-2", update)
	requireCommandCommitted(t, completion, err)
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}

	// Data prepared under different options tiles inconsistently with the
	// source, so the install is rejected.
	mismatchedOptions := options
	mismatchedOptions.Tolerance = pointerTo(0.25)
	mismatched, err := GeojsonSourceDataCreate([]byte(`{"type":"Point","coordinates":[6,5]}`), &mismatchedOptions)
	if err != nil {
		t.Fatalf("GeojsonSourceDataCreate(mismatched): %v", err)
	}
	completion, err = m.SetGeojsonSourceData("geojson-data", mismatched)
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
	if future, err := m.AddGeojsonSourceData("closed-handle", data); future != nil || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("AddGeojsonSourceData(closed handle) = (%v, %v), want nil and ErrInvalidState", future, err)
	}
	if future, err := m.SetGeojsonSourceData("geojson-data", data); future != nil || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetGeojsonSourceData(closed handle) = (%v, %v), want nil and ErrInvalidState", future, err)
	}
	info, found, err = takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("geojson-data"))
	if err != nil || !found || info.Info.Type != StyleSourceTypeGeojson {
		t.Fatalf("GetStyleSourceInfo(geojson-data) after handle close = (%v, %v, %v), want GeoJSON true nil", info.Info.Type, found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("closed-handle")); err != nil || found {
		t.Fatalf("GetStyleSourceInfo(closed-handle) = (%v, %v), want (false, nil)", found, err)
	}
}

func TestGeoJSONSourceDataRejectsInvalidDocumentsAtPreparation(t *testing.T) {
	badID := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":{},"geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}]}`)
	if _, err := GeojsonSourceDataCreate(badID, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeojsonSourceDataCreate(unsupported id) error = %v, want ErrInvalidArgument", err)
	}
	badGeometry := []byte(`{"type":"Unsupported","coordinates":[]}`)
	if _, err := GeojsonSourceDataCreate(badGeometry, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeojsonSourceDataCreate(unsupported geometry) error = %v, want ErrInvalidArgument", err)
	}
	badClusterProperties := GeojsonSourceOptions{Cluster: pointerTo(true), ClusterProperties: pointerTo([]byte(`{"total":NaN}`))}
	points := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"rank":1}}]}`)
	if _, err := GeojsonSourceDataCreate(points, &badClusterProperties); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeojsonSourceDataCreate(non-finite cluster property) error = %v, want ErrInvalidArgument", err)
	}
	// Clustering requires a feature collection of point features.
	clustered := GeojsonSourceOptions{Cluster: pointerTo(true)}
	if _, err := GeojsonSourceDataCreate([]byte(`{"type":"Point","coordinates":[0,0]}`), &clustered); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeojsonSourceDataCreate(clustered bare geometry) error = %v, want ErrInvalidArgument", err)
	}
	lines := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"LineString","coordinates":[[0,0],[1,1]]},"properties":{}}]}`)
	if _, err := GeojsonSourceDataCreate(lines, &clustered); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeojsonSourceDataCreate(clustered non-point feature) error = %v, want ErrInvalidArgument", err)
	}
}

func TestGeoJSONSourceClusterOptions(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJson([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJson(empty style): %v", err)
	}
	points := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"rank":1}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"rank":2}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"rank":3}}]}`)
	clusterProperties := []byte(`{"total":["+",["get","rank"]]}`)
	options := GeojsonSourceOptions{Cluster: pointerTo(true), ClusterRadius: pointerTo(uint32(50)), ClusterMinPoints: pointerTo(uint32(2)), ClusterMaxZoom: pointerTo(float64(14)), ClusterProperties: pointerTo(clusterProperties)}
	data, err := GeojsonSourceDataCreate(points, &options)
	if err != nil {
		t.Fatalf("GeojsonSourceDataCreate(clustered): %v", err)
	}
	clusterProperties[0] = 'x'
	if _, err := m.AddGeojsonSourceData("cluster-source", data); err != nil {
		t.Fatalf("AddGeojsonSourceData(clustered): %v", err)
	}
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("cluster-source"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(cluster-source): %v", err)
	}
	if !found || info.Info.Type != StyleSourceTypeGeojson {
		t.Fatalf("GetStyleSourceInfo(cluster-source) type = (%v, %v), want GeoJSON true", info.Info.Type, found)
	}
	// Different cluster aggregations would change cluster feature properties
	// under the source's layers, so the options match rejects them.
	updatedProperties := options
	updatedProperties.ClusterProperties = pointerTo([]byte(`{"top":["max",["get","rank"]]}`))
	update, err := GeojsonSourceDataCreate(points, &updatedProperties)
	if err != nil {
		t.Fatalf("GeojsonSourceDataCreate(updated cluster properties): %v", err)
	}
	completion, err := m.SetGeojsonSourceData("cluster-source", update)
	requireCommandFailedWith(t, completion, err, ErrInvalidArgument)
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}
	malformed := GeojsonSourceOptions{Cluster: pointerTo(true), ClusterProperties: pointerTo([]byte(`{"total":["+"]}`))}
	if _, err := GeojsonSourceDataCreate(points, &malformed); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeojsonSourceDataCreate(malformed cluster properties) error = %v, want ErrInvalidArgument", err)
	}
	empty := GeojsonSourceOptions{Cluster: pointerTo(true), ClusterProperties: pointerTo([]byte{})}
	if _, err := GeojsonSourceDataCreate(points, &empty); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("empty cluster properties: %v", err)
	}
}
