package maplibre

import (
	"errors"
	"testing"
)

func TestDedicatedStyleLayerHelpers(t *testing.T) {
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
	demOptions := StyleTileSourceOptions{}.WithTileSize(512).WithRasterEncoding(StyleRasterDEMEncodingMapbox)
	if err := m.AddRasterDEMSourceTiles("dem", []string{"https://example.com/dem/{z}/{x}/{y}.png"}, &demOptions); err != nil {
		t.Fatalf("AddRasterDEMSourceTiles(): %v", err)
	}
	if err := m.AddHillshadeLayer("hillshade", "dem", ""); err != nil {
		t.Fatalf("AddHillshadeLayer(): %v", err)
	}
	if err := m.AddColorReliefLayer("relief", "dem", "hillshade"); err != nil {
		t.Fatalf("AddColorReliefLayer(): %v", err)
	}
	if err := m.AddLocationIndicatorLayer("location", ""); err != nil {
		t.Fatalf("AddLocationIndicatorLayer(): %v", err)
	}
	if err := m.SetLocationIndicatorLocation("location", LatLng{Latitude: 1, Longitude: 2}, 3); err != nil {
		t.Fatalf("SetLocationIndicatorLocation(): %v", err)
	}
	if err := m.SetLocationIndicatorBearing("location", 45); err != nil {
		t.Fatalf("SetLocationIndicatorBearing(): %v", err)
	}
	if err := m.SetLocationIndicatorAccuracyRadius("location", 12); err != nil {
		t.Fatalf("SetLocationIndicatorAccuracyRadius(): %v", err)
	}
	if err := m.SetLocationIndicatorImageName("location", LocationIndicatorImageKindTop, "marker"); err != nil {
		t.Fatalf("SetLocationIndicatorImageName(): %v", err)
	}
	checks := map[string]string{
		"hillshade": "hillshade",
		"relief":    "color-relief",
		"location":  "location-indicator",
	}
	for id, wantType := range checks {
		gotType, found, err := m.StyleLayerType(id)
		if err != nil {
			t.Fatalf("StyleLayerType(%s): %v", id, err)
		}
		if !found || gotType != wantType {
			t.Fatalf("StyleLayerType(%s) = (%q, %v), want %q true", id, gotType, found, wantType)
		}
	}
	ids, err := m.StyleLayerIDs()
	if err != nil {
		t.Fatalf("StyleLayerIDs(): %v", err)
	}
	positions := make(map[string]int, len(ids))
	for i, id := range ids {
		positions[id] = i
	}
	if positions["relief"] >= positions["hillshade"] || positions["location"] <= positions["hillshade"] {
		t.Fatalf("StyleLayerIDs() = %v, want relief before hillshade and location after hillshade", ids)
	}
	if err := m.SetLocationIndicatorImageName("location", LocationIndicatorImageKind(99), "bad"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetLocationIndicatorImageName(invalid kind) error = %v, want ErrInvalidArgument", err)
	}
	if err := m.AddHillshadeLayer("bad-hillshade", "missing", ""); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddHillshadeLayer(missing source) error = %v, want ErrInvalidArgument", err)
	}
}
func TestStyleLayerJSONAndPropertySnapshots(t *testing.T) {
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
	if err := m.AddStyleSourceJSON("points", map[string]any{
		"type": "geojson",
		"data": map[string]any{"type": "FeatureCollection", "features": []any{}},
	}); err != nil {
		t.Fatalf("AddStyleSourceJSON(points): %v", err)
	}
	layerJSON := map[string]any{
		"id":     "points-layer",
		"type":   "circle",
		"source": "points",
		"paint":  map[string]any{"circle-radius": float64(2)},
	}
	if err := m.AddStyleLayerJSON(layerJSON, ""); err != nil {
		t.Fatalf("AddStyleLayerJSON(): %v", err)
	}
	layerJSON["type"] = "mutated-after-call"
	layerType, found, err := m.StyleLayerType("points-layer")
	if err != nil {
		t.Fatalf("StyleLayerType(): %v", err)
	}
	if !found || layerType != "circle" {
		t.Fatalf("StyleLayerType(points-layer) = (%q, %v), want circle true", layerType, found)
	}
	copiedLayer, found, err := m.StyleLayerJSON("points-layer")
	if err != nil {
		t.Fatalf("StyleLayerJSON(): %v", err)
	}
	if !found {
		t.Fatalf("StyleLayerJSON(points-layer) found = false, want true")
	}
	copiedLayerObject, ok := copiedLayer.(map[string]any)
	if !ok || copiedLayerObject["type"] != "circle" {
		t.Fatalf("StyleLayerJSON(points-layer) = %#v, want copied circle object", copiedLayer)
	}
	if err := m.SetLayerProperty("points-layer", "circle-radius", float64(5)); err != nil {
		t.Fatalf("SetLayerProperty(circle-radius): %v", err)
	}
	property, err := m.LayerProperty("points-layer", "circle-radius")
	if err != nil {
		t.Fatalf("LayerProperty(circle-radius): %v", err)
	}
	if property != float64(5) {
		t.Fatalf("LayerProperty(circle-radius) = %#v, want 5", property)
	}
	filter := []any{"==", []any{"get", "kind"}, "unit"}
	if err := m.SetLayerFilter("points-layer", filter); err != nil {
		t.Fatalf("SetLayerFilter(): %v", err)
	}
	gotFilter, err := m.LayerFilter("points-layer")
	if err != nil {
		t.Fatalf("LayerFilter(): %v", err)
	}
	if gotFilter == nil {
		t.Fatalf("LayerFilter() = nil, want copied filter")
	}
	if err := m.SetLayerFilter("points-layer", nil); err != nil {
		t.Fatalf("SetLayerFilter(nil): %v", err)
	}
	if err := m.SetLayerProperty("points-layer", "circle-radius", make(chan int)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetLayerProperty(unsupported value) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.LayerProperty("missing", "circle-radius"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("LayerProperty(missing layer) error = %v, want ErrInvalidArgument", err)
	}
}
func TestStyleLightPropertyJSONSnapshots(t *testing.T) {
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
	if err := m.SetStyleLightJSON(map[string]any{"anchor": "viewport", "color": "#ffffff", "intensity": float64(0.5)}); err != nil {
		t.Fatalf("SetStyleLightJSON(): %v", err)
	}
	if err := m.SetStyleLightProperty("intensity", float64(0.75)); err != nil {
		t.Fatalf("SetStyleLightProperty(intensity): %v", err)
	}
	intensity, err := m.StyleLightProperty("intensity")
	if err != nil {
		t.Fatalf("StyleLightProperty(intensity): %v", err)
	}
	if intensity != float64(0.75) {
		t.Fatalf("StyleLightProperty(intensity) = %#v, want 0.75", intensity)
	}
	if err := m.SetStyleLightProperty("intensity", make(chan int)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleLightProperty(unsupported value) error = %v, want ErrInvalidArgument", err)
	}
}
func TestStyleLayerMetadataForMissingLayers(t *testing.T) {
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
	ids, err := m.StyleLayerIDs()
	if err != nil {
		t.Fatalf("StyleLayerIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleLayerIDs() unexpectedly contains missing layer: %v", ids)
		}
	}
	exists, err := m.StyleLayerExists("missing")
	if err != nil {
		t.Fatalf("StyleLayerExists(): %v", err)
	}
	if exists {
		t.Fatalf("StyleLayerExists(missing) = true, want false")
	}
	layerType, found, err := m.StyleLayerType("missing")
	if err != nil {
		t.Fatalf("StyleLayerType(): %v", err)
	}
	if found || layerType != "" {
		t.Fatalf("StyleLayerType(missing) = (%q, %v), want empty false", layerType, found)
	}
	removed, err := m.RemoveStyleLayer("missing")
	if err != nil {
		t.Fatalf("RemoveStyleLayer(): %v", err)
	}
	if removed {
		t.Fatalf("RemoveStyleLayer(missing) = true, want false")
	}
	if err := m.MoveStyleLayer("missing", ""); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("MoveStyleLayer(missing) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.StyleLayerExists(""); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StyleLayerExists(empty) error = %v, want ErrInvalidArgument", err)
	}
}
