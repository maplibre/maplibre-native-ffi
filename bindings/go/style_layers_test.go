package maplibre

import (
	"errors"
	"math"
	"testing"
)

func TestDedicatedStyleLayerHelpers(t *testing.T) {
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

const layerAccessorStyleJSON = `{"version":8,"sources":{"geo":{"type":"geojson",` +
	`"data":{"type":"FeatureCollection","features":[]}}},"layers":[` +
	`{"id":"bg","type":"background"},{"id":"fill","type":"fill","source":"geo"}]}`

func TestLayerBaseAccessorsRoundTrip(t *testing.T) {
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

	if err := m.SetStyleJSON(layerAccessorStyleJSON); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}

	if got, err := m.LayerSourceLayer("fill"); err != nil || got != "" {
		t.Fatalf("LayerSourceLayer(fill) = %q, %v; want \"\", nil", got, err)
	}
	if err := m.SetLayerSourceLayer("fill", "roads"); err != nil {
		t.Fatalf("SetLayerSourceLayer(): %v", err)
	}
	if got, err := m.LayerSourceLayer("fill"); err != nil || got != "roads" {
		t.Fatalf("LayerSourceLayer(fill) = %q, %v; want \"roads\", nil", got, err)
	}
	if got, err := m.LayerSourceID("fill"); err != nil || got != "geo" {
		t.Fatalf("LayerSourceID(fill) = %q, %v; want \"geo\", nil", got, err)
	}

	// A layer type that takes no source is rejected rather than silently ignored.
	err = m.SetLayerSourceLayer("bg", "roads")
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetLayerSourceLayer(bg) error = %v; want ErrInvalidArgument", err)
	}
	if got, err := m.LayerSourceID("bg"); err != nil || got != "" {
		t.Fatalf("LayerSourceID(bg) = %q, %v; want \"\", nil", got, err)
	}

	// An unset zoom range crosses the boundary as infinities.
	if got, err := m.LayerMinZoom("fill"); err != nil || !math.IsInf(got, -1) {
		t.Fatalf("LayerMinZoom(fill) = %v, %v; want -Inf, nil", got, err)
	}
	if got, err := m.LayerMaxZoom("fill"); err != nil || !math.IsInf(got, 1) {
		t.Fatalf("LayerMaxZoom(fill) = %v, %v; want +Inf, nil", got, err)
	}
	if err := m.SetLayerMinZoom("fill", 4); err != nil {
		t.Fatalf("SetLayerMinZoom(): %v", err)
	}
	if err := m.SetLayerMaxZoom("fill", 12.5); err != nil {
		t.Fatalf("SetLayerMaxZoom(): %v", err)
	}
	if got, err := m.LayerMinZoom("fill"); err != nil || got != 4 {
		t.Fatalf("LayerMinZoom(fill) = %v, %v; want 4, nil", got, err)
	}
	if got, err := m.LayerMaxZoom("fill"); err != nil || got != 12.5 {
		t.Fatalf("LayerMaxZoom(fill) = %v, %v; want 12.5, nil", got, err)
	}

	if got, err := m.LayerVisibility("fill"); err != nil || got != StyleLayerVisibilityVisible {
		t.Fatalf("LayerVisibility(fill) = %v, %v; want visible, nil", got, err)
	}
	if err := m.SetLayerVisibility("fill", StyleLayerVisibilityNone); err != nil {
		t.Fatalf("SetLayerVisibility(): %v", err)
	}
	if got, err := m.LayerVisibility("fill"); err != nil || got != StyleLayerVisibilityNone {
		t.Fatalf("LayerVisibility(fill) = %v, %v; want none, nil", got, err)
	}

	// An unknown raw visibility passes through to C, which rejects it.
	err = m.SetLayerVisibility("fill", StyleLayerVisibility(900))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetLayerVisibility(900) error = %v; want ErrInvalidArgument", err)
	}

	if _, err := m.LayerMinZoom("missing"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("LayerMinZoom(missing) error = %v; want ErrInvalidArgument", err)
	}
}

func TestStyleLayerJSONAndPropertySnapshots(t *testing.T) {
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
	if err := m.AddStyleSourceJSON("points", JSONObject(
		JSONMember{Name: "type", Value: JSONString("geojson")},
		JSONMember{Name: "data", Value: JSONObject(
			JSONMember{Name: "type", Value: JSONString("FeatureCollection")},
			JSONMember{Name: "features", Value: JSONArray()},
		)},
	)); err != nil {
		t.Fatalf("AddStyleSourceJSON(points): %v", err)
	}
	layerJSON := JSONObject(
		JSONMember{Name: "id", Value: JSONString("points-layer")},
		JSONMember{Name: "type", Value: JSONString("circle")},
		JSONMember{Name: "source", Value: JSONString("points")},
		JSONMember{Name: "paint", Value: JSONObject(
			JSONMember{Name: "circle-radius", Value: JSONDouble(2)},
		)},
	)
	if err := m.AddStyleLayerJSON(layerJSON, ""); err != nil {
		t.Fatalf("AddStyleLayerJSON(): %v", err)
	}
	layerJSON.Object[1].Value = JSONString("mutated-after-call")
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
	var copiedLayerType JSONValue
	for _, member := range copiedLayer.Object {
		if member.Name == "type" {
			copiedLayerType = member.Value
			break
		}
	}
	if copiedLayer.Type != JSONValueTypeObject || copiedLayerType.Type != JSONValueTypeString || copiedLayerType.String != "circle" {
		t.Fatalf("StyleLayerJSON(points-layer) = %#v, want copied circle object", copiedLayer)
	}
	if err := m.SetLayerProperty("points-layer", "circle-radius", JSONDouble(5)); err != nil {
		t.Fatalf("SetLayerProperty(circle-radius): %v", err)
	}
	property, err := m.LayerProperty("points-layer", "circle-radius")
	if err != nil {
		t.Fatalf("LayerProperty(circle-radius): %v", err)
	}
	if property.Type != JSONValueTypeDouble || property.Double != 5 {
		t.Fatalf("LayerProperty(circle-radius) = %#v, want 5", property)
	}
	filter := JSONArray(JSONString("=="), JSONArray(JSONString("get"), JSONString("kind")), JSONString("unit"))
	if err := m.SetLayerFilter("points-layer", &filter); err != nil {
		t.Fatalf("SetLayerFilter(): %v", err)
	}
	gotFilter, err := m.LayerFilter("points-layer")
	if err != nil {
		t.Fatalf("LayerFilter(): %v", err)
	}
	if gotFilter.Type == JSONValueTypeNull {
		t.Fatalf("LayerFilter() = nil, want copied filter")
	}
	if err := m.SetLayerFilter("points-layer", nil); err != nil {
		t.Fatalf("SetLayerFilter(nil): %v", err)
	}
	if err := m.SetLayerProperty("points-layer", "circle-radius", JSONDouble(math.NaN())); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetLayerProperty(non-finite JSON double) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.LayerProperty("missing", "circle-radius"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("LayerProperty(missing layer) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleLightPropertyJSONSnapshots(t *testing.T) {
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
	if err := m.SetStyleLightJSON(JSONObject(
		JSONMember{Name: "anchor", Value: JSONString("viewport")},
		JSONMember{Name: "color", Value: JSONString("#ffffff")},
		JSONMember{Name: "intensity", Value: JSONDouble(0.5)},
	)); err != nil {
		t.Fatalf("SetStyleLightJSON(): %v", err)
	}
	undefined, err := m.StyleLightProperty("does-not-exist")
	if err != nil {
		t.Fatalf("StyleLightProperty(does-not-exist): %v", err)
	}
	if undefined.Type != JSONValueTypeNull {
		t.Fatalf("StyleLightProperty(does-not-exist) = %#v, want JSON null", undefined)
	}
	if err := m.SetStyleLightProperty("intensity", JSONDouble(0.75)); err != nil {
		t.Fatalf("SetStyleLightProperty(intensity): %v", err)
	}
	intensity, err := m.StyleLightProperty("intensity")
	if err != nil {
		t.Fatalf("StyleLightProperty(intensity): %v", err)
	}
	if intensity.Type != JSONValueTypeDouble || intensity.Double != 0.75 {
		t.Fatalf("StyleLightProperty(intensity) = %#v, want 0.75", intensity)
	}
	if err := m.SetStyleLightProperty("intensity", JSONDouble(math.Inf(-1))); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleLightProperty(non-finite JSON double) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleLayerMetadataForMissingLayers(t *testing.T) {
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

func TestStyleTransitionOptionsRoundTrip(t *testing.T) {
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

	// A map with no style yet reports nothing set.
	defaults, err := m.StyleTransitionOptions()
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if !defaults.Equal(NewStyleTransitionOptions()) {
		t.Fatalf("StyleTransitionOptions() = %#v, want the C API defaults", defaults)
	}

	// The style parser fills in its own 300ms duration for a style that declares no transition.
	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	parsed, err := m.StyleTransitionOptions()
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if parsed.DurationMS == nil || *parsed.DurationMS != 300 {
		t.Fatalf("parsed DurationMS = %v, want 300", parsed.DurationMS)
	}
	if parsed.DelayMS != nil {
		t.Fatalf("parsed DelayMS = %v, want absent", *parsed.DelayMS)
	}

	const transitionStyle = `{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}`
	if err := m.SetStyleJSON(transitionStyle); err != nil {
		t.Fatalf("SetStyleJSON(transition style): %v", err)
	}
	declared, err := m.StyleTransitionOptions()
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if declared.DurationMS == nil || *declared.DurationMS != 750 {
		t.Fatalf("declared DurationMS = %v, want 750", declared.DurationMS)
	}
	if declared.DelayMS == nil || *declared.DelayMS != 100 {
		t.Fatalf("declared DelayMS = %v, want 100", declared.DelayMS)
	}
	if !declared.EnablePlacementTransitions {
		t.Fatal("declared EnablePlacementTransitions = false, want true")
	}

	// A present zero stays distinguishable from an absent field, and an absent field clears
	// what the style declared rather than merging into it.
	zero := 0.0
	options := StyleTransitionOptions{DurationMS: &zero}
	if err := m.SetStyleTransitionOptions(options); err != nil {
		t.Fatalf("SetStyleTransitionOptions(): %v", err)
	}
	applied, err := m.StyleTransitionOptions()
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if !applied.Equal(options) {
		t.Fatalf("StyleTransitionOptions() = %#v, want %#v", applied, options)
	}

	// Loading a style replaces the override with what that style declares.
	if err := m.SetStyleJSON(transitionStyle); err != nil {
		t.Fatalf("SetStyleJSON(transition style): %v", err)
	}
	reloaded, err := m.StyleTransitionOptions()
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if !reloaded.Equal(declared) {
		t.Fatalf("StyleTransitionOptions() = %#v, want %#v", reloaded, declared)
	}

	negative := -1.0
	err = m.SetStyleTransitionOptions(StyleTransitionOptions{DelayMS: &negative})
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleTransitionOptions(negative delay) = %v, want ErrInvalidArgument", err)
	}
}
