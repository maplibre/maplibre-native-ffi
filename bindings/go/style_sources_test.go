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

func requireStyleCommandFailed(t *testing.T, runtime *RuntimeHandle, commandID uint64, err error) {
	t.Helper()
	if err != nil {
		t.Fatalf("style command acceptance: %v", err)
	}
	if commandID == 0 {
		t.Fatal("style command returned a zero command ID")
	}
	for range make([]struct{}, 5000) {
		batch, drainErr := runtime.DrainEvents(0)
		if drainErr != nil {
			t.Fatalf("DrainEvents(): %v", drainErr)
		}
		for _, event := range batch.Events {
			finished, ok := event.Payload.(RuntimeEventCommandFinishedPayload)
			if ok && finished.CommandID == commandID {
				if finished.Disposition != CommandDispositionFailed {
					t.Fatalf("command %d disposition = %v, want failed", commandID, finished.Disposition)
				}
				return
			}
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("command %d did not report a terminal event", commandID)
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
	exists, err := takeStyleOperationForTest(m.StartStyleSourceExists("missing"))
	if err != nil {
		t.Fatalf("StyleSourceExists(): %v", err)
	}
	if exists {
		t.Fatalf("StyleSourceExists(missing) = true, want false")
	}
	sourceType, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceType("missing"))
	if err != nil {
		t.Fatalf("StyleSourceType(): %v", err)
	}
	if found || sourceType != StyleSourceTypeUnknown {
		t.Fatalf("StyleSourceType(missing) = (%v, %v), want (unknown, false)", sourceType, found)
	}
	_, found, err = takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("missing"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if found {
		t.Fatalf("StyleSourceInfo(missing) found = true, want false")
	}
	attribution, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceAttribution("missing"))
	if err != nil {
		t.Fatalf("StyleSourceAttribution(): %v", err)
	}
	if found || attribution != "" {
		t.Fatalf("StyleSourceAttribution(missing) = (%q, %v), want empty false", attribution, found)
	}
	removed, err := takeStyleOperationForTest(m.StartRemoveStyleSource("missing"))
	if err != nil {
		t.Fatalf("RemoveStyleSource(): %v", err)
	}
	if removed {
		t.Fatalf("RemoveStyleSource(missing) = true, want false")
	}
	if _, err := takeStyleOperationForTest(m.StartStyleSourceExists("")); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StyleSourceExists(empty) error = %v, want ErrInvalidArgument", err)
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
		gotType, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceType(id))
		if err != nil {
			t.Fatalf("StyleSourceType(%s): %v", id, err)
		}
		if !found || gotType != wantType {
			t.Fatalf("StyleSourceType(%s) = (%v, %v), want %v true", id, gotType, found, wantType)
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

	removed, err := takeStyleOperationForTest(m.StartRemoveStyleSource("inline-vector"))
	if err != nil || !removed {
		t.Fatalf("RemoveStyleSource(inline-vector) = (%v, %v), want true and nil", removed, err)
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

	data := []byte(`{"type":"FeatureCollection","features":[]}`)
	if _, err := m.AddGeoJSONSourceData("inline-geojson", data, nil); err != nil {
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

func TestGeoJSONSourceDataBuffers(t *testing.T) {
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
	data := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","geometry":{"type":"LineString","coordinates":[[2,1],[4,3]]},"properties":{"name":"before","rank":7}}]}`)
	options := StyleGeoJSONSourceOptions{}.
		WithMinZoom(1).
		WithMaxZoom(16).
		WithTolerance(0.5).
		WithBuffer(0).
		WithLineMetrics(true).
		WithTileSize(256)
	if _, err := m.AddGeoJSONSourceData("geojson-data", data, &options); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	data[0] = 'x'
	if _, err := m.SetGeoJSONSourceData("geojson-data", []byte(`{"type":"Point","coordinates":[6,5]}`)); err != nil {
		t.Fatalf("SetGeoJSONSourceData(): %v", err)
	}
	sourceType, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceType("geojson-data"))
	if err != nil {
		t.Fatalf("StyleSourceType(geojson-data): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(geojson-data) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	badID := []byte(`{"type":"FeatureCollection","features":[{"type":"Feature","id":{},"geometry":{"type":"Point","coordinates":[0,0]},"properties":{}}]}`)
	commandID, err := m.AddGeoJSONSourceData("bad-geojson-data", badID, nil)
	requireStyleCommandFailed(t, runtime, commandID, err)
	badGeometry := []byte(`{"type":"Unsupported","coordinates":[]}`)
	commandID, err = m.AddGeoJSONSourceData("bad-geometry", badGeometry, nil)
	requireStyleCommandFailed(t, runtime, commandID, err)
	badClusterProperties := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterProperties([]byte(`{"total":NaN}`))
	commandID, err = m.AddGeoJSONSourceData("bad-cluster-properties", data, &badClusterProperties)
	requireStyleCommandFailed(t, runtime, commandID, err)
	exists, err := takeStyleOperationForTest(m.StartStyleSourceExists("bad-cluster-properties"))
	if err != nil {
		t.Fatalf("StyleSourceExists(bad-cluster-properties): %v", err)
	}
	if exists {
		t.Fatalf("StyleSourceExists(bad-cluster-properties) = true, want false")
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
	if _, err := m.AddGeoJSONSourceData("cluster-source", points, &options); err != nil {
		t.Fatalf("AddGeoJSONSourceData(clustered): %v", err)
	}
	clusterProperties[0] = 'x'
	sourceType, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceType("cluster-source"))
	if err != nil {
		t.Fatalf("StyleSourceType(cluster-source): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(cluster-source) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	// Options are fixed at creation, so updating the data keeps the clustered source usable.
	if _, err := m.SetGeoJSONSourceData("cluster-source", points); err != nil {
		t.Fatalf("SetGeoJSONSourceData(clustered): %v", err)
	}
	malformed := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterProperties([]byte(`{"total":["+"]}`))
	commandID, err := m.AddGeoJSONSourceData("malformed-cluster-source", points, &malformed)
	requireStyleCommandFailed(t, runtime, commandID, err)
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
		commandID, err := m.AddGeoJSONSourceData("empty-cluster-properties-"+test.name, points, &test.options)
		if commandID != 0 || !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf(
				"AddGeoJSONSourceData(empty cluster properties) = (%d, %v), want 0 and ErrInvalidArgument",
				commandID,
				err,
			)
		}
	}
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
	exists, err := takeStyleOperationForTest(m.StartStyleSourceExists("go-json-source"))
	if err != nil {
		t.Fatalf("StyleSourceExists(): %v", err)
	}
	if !exists {
		t.Fatalf("StyleSourceExists(go-json-source) = false, want true")
	}
	sourceType, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceType("go-json-source"))
	if err != nil {
		t.Fatalf("StyleSourceType(): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(go-json-source) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StartStyleSourceInfo("go-json-source"))
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if !found || info.IDSize != uint64(len("go-json-source")) {
		t.Fatalf("StyleSourceInfo(go-json-source) = (%#v, %v), want copied ID size", info, found)
	}
	commandID, err := m.AddStyleSourceJSON("bad-json-source", []byte(`NaN`))
	requireStyleCommandFailed(t, runtime, commandID, err)
}
