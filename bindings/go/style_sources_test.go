package maplibre

import (
	"errors"
	"testing"
)

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
	if err := m.AddGeoJSONSourceURL("geojson-url", "asset://fixtures/points.geojson"); err != nil {
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
	if err := m.AddGeoJSONSourceURL("", "asset://fixtures/points.geojson"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceURL(empty id) error = %v, want ErrInvalidArgument", err)
	}
}
func TestGeoJSONSourceDataDescriptors(t *testing.T) {
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
	properties := map[string]any{"name": "before", "rank": int64(7)}
	data := GeoJSONFeatureCollection([]Feature{{
		Geometry:   LineStringGeometry(points),
		Properties: properties,
		Identifier: "feature-1",
	}})
	if err := m.AddGeoJSONSourceData("geojson-data", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	points[0] = LatLng{Latitude: 90, Longitude: 90}
	properties["name"] = "after"
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
	if err := m.AddGeoJSONSourceData("bad-geojson-data", badID); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(unsupported id) error = %v, want ErrInvalidArgument", err)
	}
	badGeometry := GeoJSON{Type: GeoJSONTypeGeometry, Geometry: Geometry{Type: GeometryType(999)}}
	if err := m.AddGeoJSONSourceData("bad-geometry", badGeometry); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(unsupported geometry) error = %v, want ErrInvalidArgument", err)
	}
}
func TestAddStyleSourceJSONCopiesGoJSONDescriptor(t *testing.T) {
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
	source := map[string]any{
		"type": "geojson",
		"data": map[string]any{
			"type":     "FeatureCollection",
			"features": []any{},
		},
		"attribution": "unit-test",
	}
	if err := m.AddStyleSourceJSON("go-json-source", source); err != nil {
		t.Fatalf("AddStyleSourceJSON(): %v", err)
	}
	source["type"] = "mutated-after-call"
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
	if err := m.AddStyleSourceJSON("bad-json-source", map[string]any{"type": make(chan int)}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddStyleSourceJSON(unsupported Go value) error = %v, want ErrInvalidArgument", err)
	}
}
