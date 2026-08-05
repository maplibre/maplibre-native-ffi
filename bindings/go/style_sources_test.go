package maplibre

import (
	"errors"
	"math"
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
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
	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(replacement): %v", err)
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

	data := GeoJSONFeatureCollection(nil)
	if err := m.AddGeoJSONSourceData("inline-geojson", data, nil); err != nil {
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

func TestGeoJSONSourceDataDescriptors(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	points := []LatLng{{Latitude: 1, Longitude: 2}, {Latitude: 3, Longitude: 4}}
	properties := JSONMembers{
		{Name: "name", Value: JSONString("before")},
		{Name: "rank", Value: JSONInt(7)},
	}
	data := GeoJSONFeatureCollection([]Feature{{
		Geometry:   LineStringGeometry(points),
		Properties: properties,
		Identifier: "feature-1",
	}})
	options := StyleGeoJSONSourceOptions{}.
		WithMinZoom(1).
		WithMaxZoom(16).
		WithTolerance(0.5).
		WithBuffer(0).
		WithLineMetrics(true).
		WithTileSize(256)
	if err := m.AddGeoJSONSourceData("geojson-data", data, &options); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	points[0] = LatLng{Latitude: 90, Longitude: 90}
	properties[0].Value = JSONString("after")
	if err := m.SetGeoJSONSourceData("geojson-data", GeoJSON{Type: GeoJSONTypeGeometry, Geometry: PointGeometry(LatLng{Latitude: 5, Longitude: 6})}); err != nil {
		t.Fatalf("SetGeoJSONSourceData(): %v", err)
	}
	sourceType, found, err := m.StyleSourceType("geojson-data")
	if err != nil {
		t.Fatalf("StyleSourceType(geojson-data): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(geojson-data) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	badID := GeoJSONFeatureCollection([]Feature{{Geometry: PointGeometry(LatLng{}), Identifier: struct{}{}}})
	if err := m.AddGeoJSONSourceData("bad-geojson-data", badID, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(unsupported id) error = %v, want ErrInvalidArgument", err)
	}
	badGeometry := GeoJSON{Type: GeoJSONTypeGeometry, Geometry: Geometry{Type: GeometryType(999)}}
	if err := m.AddGeoJSONSourceData("bad-geometry", badGeometry, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(unsupported geometry) error = %v, want ErrInvalidArgument", err)
	}
	badClusterProperties := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterProperties(JSONObject(JSONMember{Name: "total", Value: JSONDouble(math.Inf(1))}))
	if err := m.AddGeoJSONSourceData("bad-cluster-properties", data, &badClusterProperties); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(non-finite cluster property) error = %v, want ErrInvalidArgument", err)
	}
	exists, err := m.StyleSourceExists("bad-cluster-properties")
	if err != nil {
		t.Fatalf("StyleSourceExists(bad-cluster-properties): %v", err)
	}
	if exists {
		t.Fatalf("StyleSourceExists(bad-cluster-properties) = true, want false")
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	points := GeoJSONFeatureCollection([]Feature{
		{Geometry: PointGeometry(LatLng{Latitude: 0, Longitude: 0}), Properties: JSONMembers{{Name: "rank", Value: JSONInt(1)}}},
		{Geometry: PointGeometry(LatLng{Latitude: 0.001, Longitude: 0.001}), Properties: JSONMembers{{Name: "rank", Value: JSONInt(2)}}},
		{Geometry: PointGeometry(LatLng{Latitude: 0.002, Longitude: 0.002}), Properties: JSONMembers{{Name: "rank", Value: JSONInt(3)}}},
	})
	// MapLibre Native parses the aggregation expressions while the descriptor is borrowed, so a
	// source that adds successfully proves the nested cluster property tree crossed the boundary.
	clusterProperties := JSONMembers{
		{Name: "total", Value: JSONArray(JSONString("+"), JSONArray(JSONString("get"), JSONString("rank")))},
	}
	options := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterRadius(50).
		WithClusterMinPoints(2).
		WithClusterMaxZoom(14).
		WithClusterProperties(JSONValue{Type: JSONValueTypeObject, Object: clusterProperties})
	if err := m.AddGeoJSONSourceData("cluster-source", points, &options); err != nil {
		t.Fatalf("AddGeoJSONSourceData(clustered): %v", err)
	}
	clusterProperties[0].Value = JSONString("mutated-after-call")
	sourceType, found, err := m.StyleSourceType("cluster-source")
	if err != nil {
		t.Fatalf("StyleSourceType(cluster-source): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(cluster-source) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	// Options are fixed at creation, so updating the data keeps the clustered source usable.
	if err := m.SetGeoJSONSourceData("cluster-source", points); err != nil {
		t.Fatalf("SetGeoJSONSourceData(clustered): %v", err)
	}
	malformed := StyleGeoJSONSourceOptions{}.
		WithCluster(true).
		WithClusterProperties(JSONObject(JSONMember{Name: "total", Value: JSONArray(JSONString("+"))}))
	if err := m.AddGeoJSONSourceData("malformed-cluster-source", points, &malformed); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(malformed cluster properties) error = %v, want ErrInvalidArgument", err)
	}
}

func TestAddStyleSourceJSONCopiesGoJSONDescriptor(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	source := JSONObject(
		JSONMember{Name: "type", Value: JSONString("geojson")},
		JSONMember{Name: "data", Value: JSONObject(
			JSONMember{Name: "type", Value: JSONString("FeatureCollection")},
			JSONMember{Name: "features", Value: JSONArray()},
		)},
		JSONMember{Name: "attribution", Value: JSONString("unit-test")},
	)
	if err := m.AddStyleSourceJSON("go-json-source", source); err != nil {
		t.Fatalf("AddStyleSourceJSON(): %v", err)
	}
	source.Object[0].Value = JSONString("mutated-after-call")
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
	if err := m.AddStyleSourceJSON("bad-json-source", JSONDouble(math.Inf(1))); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddStyleSourceJSON(non-finite JSON double) error = %v, want ErrInvalidArgument", err)
	}
}
