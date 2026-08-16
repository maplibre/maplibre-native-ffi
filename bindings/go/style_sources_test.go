package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
)

func TestStyleSourceMetadataForMissingSources(t *testing.T) {
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := m.StyleSourceIDs()
	if err != nil {
		t.Fatalf("StyleSourceIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleSourceIDs() unexpectedly contains missing source: %v", ids)
		}
	}
	exists, err := m.StyleSourceExists("missing")
	if err != nil {
		t.Fatalf("StyleSourceExists(): %v", err)
	}
	if exists {
		t.Fatalf("StyleSourceExists(missing) = true, want false")
	}
	sourceType, found, err := m.StyleSourceType("missing")
	if err != nil {
		t.Fatalf("StyleSourceType(): %v", err)
	}
	if found || sourceType != StyleSourceTypeUnknown {
		t.Fatalf("StyleSourceType(missing) = (%v, %v), want (unknown, false)", sourceType, found)
	}
	_, found, err = m.StyleSourceInfo("missing")
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if found {
		t.Fatalf("StyleSourceInfo(missing) found = true, want false")
	}
	attribution, found, err := m.StyleSourceAttribution("missing")
	if err != nil {
		t.Fatalf("StyleSourceAttribution(): %v", err)
	}
	if found || attribution != "" {
		t.Fatalf("StyleSourceAttribution(missing) = (%q, %v), want empty false", attribution, found)
	}
	removed, err := m.RemoveStyleSource("missing")
	if err != nil {
		t.Fatalf("RemoveStyleSource(): %v", err)
	}
	if removed {
		t.Fatalf("RemoveStyleSource(missing) = true, want false")
	}
	if _, err := m.StyleSourceExists(""); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StyleSourceExists(empty) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleSourceURLAndTileBindings(t *testing.T) {
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	geoJSONOptions := StyleGeoJSONSourceOptions{}.
		WithMinZoom(1).
		WithTolerance(0.5).
		WithBuffer(64)
	if err := m.AddGeoJSONSourceURL("geojson-url", "asset://fixtures/points.geojson", &geoJSONOptions); err != nil {
		t.Fatalf("AddGeoJSONSourceURL(): %v", err)
	}
	if err := m.SetGeoJSONSourceURL("geojson-url", "asset://fixtures/points-2.geojson"); err != nil {
		t.Fatalf("SetGeoJSONSourceURL(): %v", err)
	}
	tileOptions := StyleTileSourceOptions{}.
		WithTileSize(256).
		WithAttribution("unit attribution")
	if err := m.AddVectorSourceTiles("vector-tiles", []string{"https://example.com/vector/{z}/{x}/{y}.pbf"}, &tileOptions); err != nil {
		t.Fatalf("AddVectorSourceTiles(): %v", err)
	}
	if err := m.AddRasterSourceURL("raster-url", "https://example.com/raster.json", &tileOptions); err != nil {
		t.Fatalf("AddRasterSourceURL(): %v", err)
	}
	demOptions := StyleTileSourceOptions{}.
		WithTileSize(512).
		WithRasterEncoding(StyleRasterDEMEncodingTerrarium)
	if err := m.AddRasterDEMSourceTiles("dem-tiles", []string{"https://example.com/dem/{z}/{x}/{y}.png"}, &demOptions); err != nil {
		t.Fatalf("AddRasterDEMSourceTiles(): %v", err)
	}
	checks := map[string]StyleSourceType{
		"geojson-url":  StyleSourceTypeGeoJSON,
		"vector-tiles": StyleSourceTypeVector,
		"raster-url":   StyleSourceTypeRaster,
		"dem-tiles":    StyleSourceTypeRasterDEM,
	}
	for id, wantType := range checks {
		gotType, found, err := m.StyleSourceType(id)
		if err != nil {
			t.Fatalf("StyleSourceType(%s): %v", id, err)
		}
		if !found || gotType != wantType {
			t.Fatalf("StyleSourceType(%s) = (%v, %v), want %v true", id, gotType, found, wantType)
		}
	}
	if err := m.AddVectorSourceTiles("bad-vector", nil, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddVectorSourceTiles(nil) error = %v, want ErrInvalidArgument", err)
	}
	if err := m.AddGeoJSONSourceURL("", "asset://fixtures/points.geojson", nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceURL(empty id) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleSourceInfoCopiesReconstructibleMetadata(t *testing.T) {
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
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
	if err := m.AddVectorSourceTiles("inline-vector", tileURLs, &options); err != nil {
		t.Fatalf("AddVectorSourceTiles(): %v", err)
	}

	info, found, err := m.StyleSourceInfo("inline-vector")
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

	removed, err := m.RemoveStyleSource("inline-vector")
	if err != nil || !removed {
		t.Fatalf("RemoveStyleSource(inline-vector) = (%v, %v), want true and nil", removed, err)
	}
	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON([]byte(replacement)): %v", err)
	}
	if *info.Attribution != attribution || info.TileJSON.TileURLs[1] != tileURLs[1] || *info.TileJSON.Bounds != bounds {
		t.Fatalf("copied source info changed after removal and style replacement: %#v", info)
	}

	url := "https://example.invalid/vector-tilejson.json"
	if err := m.AddVectorSourceURL("url-vector", url, nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}
	urlInfo, found, err := m.StyleSourceInfo("url-vector")
	if err != nil {
		t.Fatalf("StyleSourceInfo(url-vector): %v", err)
	}
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
	if err := m.AddGeoJSONSourceData("inline-geojson", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	geoJSONInfo, found, err := m.StyleSourceInfo("inline-geojson")
	if err != nil {
		t.Fatalf("StyleSourceInfo(inline-geojson): %v", err)
	}
	if !found || geoJSONInfo.URL != nil || geoJSONInfo.TileJSON != nil {
		t.Fatalf("StyleSourceInfo(inline-geojson) = (%#v, %v), want absent URL and TileJSON", geoJSONInfo, found)
	}
}

func TestGeoJSONSourceDataPrepareAndInstall(t *testing.T) {
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
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
	if err := m.AddGeoJSONSourceData("geojson-data", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	sourceType, found, err := m.StyleSourceType("geojson-data")
	if err != nil {
		t.Fatalf("StyleSourceType(geojson-data): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(geojson-data) = (%v, %v), want GeoJSON true", sourceType, found)
	}

	// One prepared handle installs on any number of sources.
	if err := m.AddGeoJSONSourceData("geojson-data-2", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(reused handle): %v", err)
	}

	// A set requires data prepared with the source's options.
	update, err := NewGeoJSONSourceData([]byte(`{"type":"Point","coordinates":[6,5]}`), &options)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(update): %v", err)
	}
	if err := m.SetGeoJSONSourceData("geojson-data", update); err != nil {
		t.Fatalf("SetGeoJSONSourceData(): %v", err)
	}
	if err := m.SetGeoJSONSourceData("geojson-data-2", update); err != nil {
		t.Fatalf("SetGeoJSONSourceData(reused handle): %v", err)
	}
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
	if err := m.SetGeoJSONSourceData("geojson-data", mismatched); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetGeoJSONSourceData(mismatched options) error = %v, want ErrInvalidArgument", err)
	}
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
	if err := m.AddGeoJSONSourceData("closed-handle", data); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(closed handle) error = %v, want ErrInvalidArgument", err)
	}
	if err := m.SetGeoJSONSourceData("geojson-data", data); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetGeoJSONSourceData(closed handle) error = %v, want ErrInvalidArgument", err)
	}
	sourceType, found, err = m.StyleSourceType("geojson-data")
	if err != nil || !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(geojson-data) after handle close = (%v, %v, %v), want GeoJSON true nil", sourceType, found, err)
	}
	exists, err := m.StyleSourceExists("closed-handle")
	if err != nil {
		t.Fatalf("StyleSourceExists(closed-handle): %v", err)
	}
	if exists {
		t.Fatalf("StyleSourceExists(closed-handle) = true, want false")
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
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
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
	if err := m.AddGeoJSONSourceData("cluster-source", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(clustered): %v", err)
	}
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	sourceType, found, err := m.StyleSourceType("cluster-source")
	if err != nil {
		t.Fatalf("StyleSourceType(cluster-source): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(cluster-source) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	// Different cluster aggregations would change cluster feature properties
	// under the source's layers, so the options match rejects them.
	updatedProperties := options.WithClusterProperties([]byte(`{"top":["max",["get","rank"]]}`))
	update, err := NewGeoJSONSourceData(points, &updatedProperties)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(updated cluster properties): %v", err)
	}
	if err := m.SetGeoJSONSourceData("cluster-source", update); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetGeoJSONSourceData(updated cluster properties) error = %v, want ErrInvalidArgument", err)
	}
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
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
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
	if err := m.AddGeoJSONSourceData("worker-prepared", result.data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(worker prepared): %v", err)
	}
	closed := make(chan error)
	go func() {
		closed <- result.data.Close()
	}()
	if err := <-closed; err != nil {
		t.Fatalf("Close() on a goroutine: %v", err)
	}
	sourceType, found, err := m.StyleSourceType("worker-prepared")
	if err != nil || !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(worker-prepared) = (%v, %v, %v), want GeoJSON true nil", sourceType, found, err)
	}
}

func TestGeoJSONSourceSynchronousTilingOverride(t *testing.T) {
	lockOSThreadForTest(t)

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	data, err := NewGeoJSONSourceData([]byte(`{"type":"FeatureCollection","features":[]}`), nil)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(): %v", err)
	}
	if err := m.AddGeoJSONSourceData("tracked", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	if err := m.SetGeoJSONSourceSynchronousTiling("tracked", true); err != nil {
		t.Fatalf("SetGeoJSONSourceSynchronousTiling(true): %v", err)
	}
	update, err := NewGeoJSONSourceData([]byte(`{"type":"Point","coordinates":[3,4]}`), nil)
	if err != nil {
		t.Fatalf("NewGeoJSONSourceData(update): %v", err)
	}
	if err := m.SetGeoJSONSourceData("tracked", update); err != nil {
		t.Fatalf("SetGeoJSONSourceData() under the override: %v", err)
	}
	if err := update.Close(); err != nil {
		t.Fatalf("update Close(): %v", err)
	}
	if err := m.SetGeoJSONSourceSynchronousTiling("tracked", false); err != nil {
		t.Fatalf("SetGeoJSONSourceSynchronousTiling(false): %v", err)
	}
	if err := data.Close(); err != nil {
		t.Fatalf("data Close(): %v", err)
	}
	if err := m.SetGeoJSONSourceSynchronousTiling("missing", true); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetGeoJSONSourceSynchronousTiling(missing source) error = %v, want ErrInvalidArgument", err)
	}
	if err := m.AddVectorSourceURL("vector", "https://example.invalid/tiles.json", nil); err != nil {
		t.Fatalf("AddVectorSourceURL(): %v", err)
	}
	if err := m.SetGeoJSONSourceSynchronousTiling("vector", true); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetGeoJSONSourceSynchronousTiling(non-GeoJSON source) error = %v, want ErrInvalidArgument", err)
	}
}

func TestAddStyleSourceJSONCopiesGoBuffer(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

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

	if err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	source := []byte(`{"type":"geojson","data":{"type":"FeatureCollection","features":[]},"attribution":"unit-test"}`)
	if err := m.AddStyleSourceJSON("go-json-source", source); err != nil {
		t.Fatalf("AddStyleSourceJSON(): %v", err)
	}
	source[0] = 'x'
	exists, err := m.StyleSourceExists("go-json-source")
	if err != nil {
		t.Fatalf("StyleSourceExists(): %v", err)
	}
	if !exists {
		t.Fatalf("StyleSourceExists(go-json-source) = false, want true")
	}
	sourceType, found, err := m.StyleSourceType("go-json-source")
	if err != nil {
		t.Fatalf("StyleSourceType(): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(go-json-source) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	info, found, err := m.StyleSourceInfo("go-json-source")
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if !found || info.IDSize != uint64(len("go-json-source")) {
		t.Fatalf("StyleSourceInfo(go-json-source) = (%#v, %v), want copied ID size", info, found)
	}
	if err := m.AddStyleSourceJSON("bad-json-source", []byte(`NaN`)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddStyleSourceJSON(non-finite JSON double) error = %v, want ErrInvalidArgument", err)
	}
}
