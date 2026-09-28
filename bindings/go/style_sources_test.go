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

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := awaitForTest(m.ListStyleSourceIDs())
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

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if _, err := m.AddVectorSourceURL("volatile-source", "https://example.invalid/tiles.json", nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
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

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	geoJSONOptions := GeoJSONSourceOptions{MinZoom: pointerTo(float64(1)), Tolerance: pointerTo(0.5), Buffer: pointerTo(uint32(64))}
	if _, err := m.AddGeoJSONSourceURL("geojson-url", "asset://fixtures/points.geojson", &geoJSONOptions); err != nil {
		t.Fatalf("AddGeoJSONSourceURL(): %v", err)
	}
	if _, err := m.SetGeoJSONSourceURL("geojson-url", "asset://fixtures/points-2.geojson"); err != nil {
		t.Fatalf("SetGeoJSONSourceURL(): %v", err)
	}
	tileOptions := StyleTileSourceOptions{TileSize: pointerTo(uint32(256)), Attribution: pointerTo("unit attribution")}
	if _, err := m.AddVectorSourceTiles("vector-tiles", []string{"https://example.com/vector/{z}/{x}/{y}.pbf"}, &tileOptions); err != nil {
		t.Fatalf("AddVectorSourceTiles(): %v", err)
	}
	if _, err := m.AddRasterSourceURL("raster-url", "https://example.com/raster.json", &tileOptions); err != nil {
		t.Fatalf("AddRasterSourceURL(): %v", err)
	}
	demOptions := StyleTileSourceOptions{TileSize: pointerTo(uint32(512)), RasterEncoding: pointerTo(StyleRasterDEMEncodingTerrarium)}
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

	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("inline-vector"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(inline-vector): %v", err)
	}
	if !found {
		t.Fatal("GetStyleSourceInfo(inline-vector) found = false, want true")
	}
	if info.Info.Type != StyleSourceTypeVector || info.URL != nil {
		t.Fatalf("GetStyleSourceInfo(inline-vector) type/URL = (%v, %v), want vector and absent URL", info.Info.Type, info.URL)
	}
	if info.Attribution == nil || *info.Attribution != attribution {
		t.Fatalf("GetStyleSourceInfo(inline-vector) attribution = %v, want %q", info.Attribution, attribution)
	}
	if info.Info.TileJSON == nil {
		t.Fatal("GetStyleSourceInfo(inline-vector) TileJSON = nil, want inline TileJSON")
	}
	if len(info.TileURLs) != len(tileURLs) {
		t.Fatalf("GetStyleSourceInfo(inline-vector) tile URLs = %v, want %v", info.TileURLs, tileURLs)
	}
	for i := range tileURLs {
		if info.TileURLs[i] != tileURLs[i] {
			t.Fatalf("GetStyleSourceInfo(inline-vector) tile URL %d = %q, want %q", i, info.TileURLs[i], tileURLs[i])
		}
	}
	if info.Info.TileJSON.MinZoom != minZoom || info.Info.TileJSON.MaxZoom != maxZoom || info.Info.TileJSON.Scheme != scheme {
		t.Fatalf("GetStyleSourceInfo(inline-vector) TileJSON = %#v, want zooms %v/%v and scheme %v", info.Info.TileJSON, minZoom, maxZoom, scheme)
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
	if _, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceURL("inline-vector")); err != nil || found {
		t.Fatalf("StyleSourceURL(inline-vector) = (%v, %v), want (false, nil) for an inline source", found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceAttribution("missing")); err != nil || found {
		t.Fatalf("StyleSourceAttribution(missing) = (%v, %v), want (false, nil)", found, err)
	}
	copiedTileURLs, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceTileURLs("inline-vector"))
	if err != nil || !found || !slices.Equal(copiedTileURLs.TileURLs, tileURLs) {
		t.Fatalf("GetStyleSourceTileURLs(inline-vector) = (%q, %v, %v), want %q and true", copiedTileURLs, found, err, tileURLs)
	}
	if missingTileURLs, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceTileURLs("missing")); err != nil || found {
		t.Fatalf("GetStyleSourceTileURLs(missing) = (%q, %v, %v), want (false, nil)", missingTileURLs, found, err)
	}

	layerJSON := []byte(`{"id":"inline-vector-layer","type":"line","source":"inline-vector","source-layer":"lines"}`)
	layerID, err := m.AddStyleLayerJSON(layerJSON, nil)
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
	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON([]byte(replacement)): %v", err)
	}
	if *info.Attribution != attribution || info.TileURLs[1] != tileURLs[1] || *info.Info.Bounds != bounds {
		t.Fatalf("copied source info changed after removal and style replacement: %#v", info)
	}

	url := "https://example.invalid/vector-tilejson.json"
	if _, err := m.AddVectorSourceURL("url-vector", url, nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}
	urlInfo, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("url-vector"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(url-vector): %v", err)
	}
	if !found || urlInfo.URL == nil || *urlInfo.URL != url {
		t.Fatalf("GetStyleSourceInfo(url-vector) URL = (%v, %v), want %q and true", urlInfo.URL, found, url)
	}
	if urlInfo.Info.TileJSON != nil || urlInfo.Attribution != nil {
		t.Fatalf("GetStyleSourceInfo(url-vector) optional loaded fields = (%v, %v), want absent", urlInfo.Info.TileJSON, urlInfo.Attribution)
	}
	copiedURL, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceURL("url-vector"))
	if err != nil || !found || copiedURL != url {
		t.Fatalf("StyleSourceURL(url-vector) = (%q, %v, %v), want %q", copiedURL, found, err, url)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.CopyStyleSourceURL("missing")); err != nil || found {
		t.Fatalf("StyleSourceURL(missing) = (%v, %v), want (false, nil)", found, err)
	}
	if urlTileURLs, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceTileURLs("url-vector")); err != nil || !found || len(urlTileURLs.TileURLs) != 0 {
		t.Fatalf("GetStyleSourceTileURLs(url-vector) = (%q, %v, %v), want an empty list and true for a URL-backed source", urlTileURLs, found, err)
	}

	data, err := GeoJSONSourceDataCreate([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatalf("GeoJSONSourceDataCreate(): %v", err)
	}
	defer func() {
		if err := data.Close(); err != nil {
			t.Errorf("GeoJSONSourceDataHandle Close(): %v", err)
		}
	}()
	if _, err := m.AddGeoJSONSourceData("inline-geojson", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	geoJSONInfo, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("inline-geojson"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(inline-geojson): %v", err)
	}
	if !found || geoJSONInfo.URL != nil || geoJSONInfo.Info.TileJSON != nil {
		t.Fatalf("GetStyleSourceInfo(inline-geojson) = (%#v, %v), want absent URL and TileJSON", geoJSONInfo, found)
	}
}

func TestGeoJSONSourceDataPrepareAndInstall(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)
	for _, invalid := range []*GeoJSONSourceDataHandle{nil, {}} {
		if future, err := m.AddGeoJSONSourceData("invalid", invalid); future != nil || !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("nil or zero input owner: future=%v error=%v", future, err)
		}
	}

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	document := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","geometry":{"type":"LineString","coordinates":[[2,1],[4,3]]},"properties":{"name":"before","rank":7}}]}`)
	options := GeoJSONSourceOptions{MinZoom: pointerTo(float64(1)), MaxZoom: pointerTo(float64(16)), Tolerance: pointerTo(0.5), Buffer: pointerTo(uint32(0)), LineMetrics: pointerTo(true), TileSize: pointerTo(uint32(256))}
	data, err := GeoJSONSourceDataCreate(document, &options)
	if err != nil {
		t.Fatalf("GeoJSONSourceDataCreate(): %v", err)
	}
	// The document is copied at preparation, so mutating it afterward does not
	// reach the prepared index.
	document[0] = 'x'
	if _, err := m.AddGeoJSONSourceData("geojson-data", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("geojson-data"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(geojson-data): %v", err)
	}
	if !found || info.Info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("GetStyleSourceInfo(geojson-data) type = (%v, %v), want GeoJSON true", info.Info.Type, found)
	}

	// One prepared handle installs on any number of sources.
	if _, err := m.AddGeoJSONSourceData("geojson-data-2", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(reused handle): %v", err)
	}

	// A set requires data prepared with the source's options.
	update, err := GeoJSONSourceDataCreate([]byte(`{"type":"Point","coordinates":[6,5]}`), &options)
	if err != nil {
		t.Fatalf("GeoJSONSourceDataCreate(update): %v", err)
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
	mismatchedOptions := options
	mismatchedOptions.Tolerance = pointerTo(0.25)
	mismatched, err := GeoJSONSourceDataCreate([]byte(`{"type":"Point","coordinates":[6,5]}`), &mismatchedOptions)
	if err != nil {
		t.Fatalf("GeoJSONSourceDataCreate(mismatched): %v", err)
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
	if future, err := m.AddGeoJSONSourceData("closed-handle", data); future != nil || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("AddGeoJSONSourceData(closed handle) = (%v, %v), want nil and ErrInvalidState", future, err)
	}
	if future, err := m.SetGeoJSONSourceData("geojson-data", data); future != nil || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetGeoJSONSourceData(closed handle) = (%v, %v), want nil and ErrInvalidState", future, err)
	}
	info, found, err = takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("geojson-data"))
	if err != nil || !found || info.Info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("GetStyleSourceInfo(geojson-data) after handle close = (%v, %v, %v), want GeoJSON true nil", info.Info.Type, found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("closed-handle")); err != nil || found {
		t.Fatalf("GetStyleSourceInfo(closed-handle) = (%v, %v), want (false, nil)", found, err)
	}
}

func TestGeoJSONSourceDataRejectsInvalidDocumentsAtPreparation(t *testing.T) {
	badID := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":{},"geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}]}`)
	if _, err := GeoJSONSourceDataCreate(badID, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeoJSONSourceDataCreate(unsupported id) error = %v, want ErrInvalidArgument", err)
	}
	badGeometry := []byte(`{"type":"Unsupported","coordinates":[]}`)
	if _, err := GeoJSONSourceDataCreate(badGeometry, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeoJSONSourceDataCreate(unsupported geometry) error = %v, want ErrInvalidArgument", err)
	}
	badClusterProperties := GeoJSONSourceOptions{Cluster: pointerTo(true), ClusterProperties: pointerTo([]byte(`{"total":NaN}`))}
	points := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"rank":1}}]}`)
	if _, err := GeoJSONSourceDataCreate(points, &badClusterProperties); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeoJSONSourceDataCreate(non-finite cluster property) error = %v, want ErrInvalidArgument", err)
	}
	// Clustering requires a feature collection of point features.
	clustered := GeoJSONSourceOptions{Cluster: pointerTo(true)}
	if _, err := GeoJSONSourceDataCreate([]byte(`{"type":"Point","coordinates":[0,0]}`), &clustered); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeoJSONSourceDataCreate(clustered bare geometry) error = %v, want ErrInvalidArgument", err)
	}
	lines := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"LineString","coordinates":[[0,0],[1,1]]},"properties":{}}]}`)
	if _, err := GeoJSONSourceDataCreate(lines, &clustered); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeoJSONSourceDataCreate(clustered non-point feature) error = %v, want ErrInvalidArgument", err)
	}
}

func TestGeoJSONSourceClusterOptions(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	points := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":{"rank":1}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"rank":2}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"rank":3}}]}`)
	clusterProperties := []byte(`{"total":["+",["get","rank"]]}`)
	options := GeoJSONSourceOptions{Cluster: pointerTo(true), ClusterRadius: pointerTo(uint32(50)), ClusterMinPoints: pointerTo(uint32(2)), ClusterMaxZoom: pointerTo(float64(14)), ClusterProperties: pointerTo(clusterProperties)}
	data, err := GeoJSONSourceDataCreate(points, &options)
	if err != nil {
		t.Fatalf("GeoJSONSourceDataCreate(clustered): %v", err)
	}
	clusterProperties[0] = 'x'
	if _, err := m.AddGeoJSONSourceData("cluster-source", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(clustered): %v", err)
	}
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("cluster-source"))
	if err != nil {
		t.Fatalf("GetStyleSourceInfo(cluster-source): %v", err)
	}
	if !found || info.Info.Type != StyleSourceTypeGeoJSON {
		t.Fatalf("GetStyleSourceInfo(cluster-source) type = (%v, %v), want GeoJSON true", info.Info.Type, found)
	}
	// Different cluster aggregations would change cluster feature properties
	// under the source's layers, so the options match rejects them.
	updatedProperties := options
	updatedProperties.ClusterProperties = pointerTo([]byte(`{"top":["max",["get","rank"]]}`))
	update, err := GeoJSONSourceDataCreate(points, &updatedProperties)
	if err != nil {
		t.Fatalf("GeoJSONSourceDataCreate(updated cluster properties): %v", err)
	}
	completion, err := m.SetGeoJSONSourceData("cluster-source", update)
	requireCommandFailedWith(t, completion, err, ErrInvalidArgument)
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}
	malformed := GeoJSONSourceOptions{Cluster: pointerTo(true), ClusterProperties: pointerTo([]byte(`{"total":["+"]}`))}
	if _, err := GeoJSONSourceDataCreate(points, &malformed); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GeoJSONSourceDataCreate(malformed cluster properties) error = %v, want ErrInvalidArgument", err)
	}
	empty := GeoJSONSourceOptions{Cluster: pointerTo(true), ClusterProperties: pointerTo([]byte{})}
	if _, err := GeoJSONSourceDataCreate(points, &empty); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("empty cluster properties: %v", err)
	}
}
