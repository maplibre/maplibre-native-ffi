package maplibre

import (
	"bytes"
	"errors"
	"math"
	"reflect"
	"testing"
)

func TestDedicatedStyleLayerHelpers(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	demOptions := StyleTileSourceOptions{TileSize: pointerTo(uint32(512)), RasterEncoding: pointerTo(StyleRasterDEMEncodingMapbox)}
	if _, err := m.AddRasterDEMSourceTiles("dem", []string{"https://example.com/dem/{z}/{x}/{y}.png"}, &demOptions); err != nil {
		t.Fatalf("AddRasterDEMSourceTiles(): %v", err)
	}
	if _, err := m.AddHillshadeLayer("hillshade", "dem", nil); err != nil {
		t.Fatalf("AddHillshadeLayer(): %v", err)
	}
	if _, err := m.AddColorReliefLayer("relief", "dem", pointerTo("hillshade")); err != nil {
		t.Fatalf("AddColorReliefLayer(): %v", err)
	}
	if _, err := m.AddLocationIndicatorLayer("location", nil); err != nil {
		t.Fatalf("AddLocationIndicatorLayer(): %v", err)
	}
	if _, err := m.SetLocationIndicatorLocation("location", LatLng{Latitude: 1, Longitude: 2}, 3); err != nil {
		t.Fatalf("SetLocationIndicatorLocation(): %v", err)
	}
	if _, err := m.SetLocationIndicatorBearing("location", 45); err != nil {
		t.Fatalf("SetLocationIndicatorBearing(): %v", err)
	}
	if _, err := m.SetLocationIndicatorAccuracyRadius("location", 12); err != nil {
		t.Fatalf("SetLocationIndicatorAccuracyRadius(): %v", err)
	}
	if _, err := m.SetLocationIndicatorImageName("location", LocationIndicatorImageKindTop, "marker"); err != nil {
		t.Fatalf("SetLocationIndicatorImageName(): %v", err)
	}
	checks := map[string]string{
		"hillshade": "hillshade",
		"relief":    "color-relief",
		"location":  "location-indicator",
	}
	for id, wantType := range checks {
		info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo(id))
		if err != nil {
			t.Fatalf("GetStyleLayerInfo(%s): %v", id, err)
		}
		if !found || info.Info.Type != wantType {
			t.Fatalf("GetStyleLayerInfo(%s) type = (%q, %v), want %q true", id, info.Info.Type, found, wantType)
		}
	}
	ids, err := awaitForTest(m.ListStyleLayerIDs())
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
	completion, err := m.SetLocationIndicatorImageName("location", LocationIndicatorImageKind(99), "bad")
	requireStyleCommandFailed(t, completion, err)
	completion, err = m.AddHillshadeLayer("bad-hillshade", "missing", nil)
	requireStyleCommandFailed(t, completion, err)
}

const layerListStyleJSON = `{"version":8,"sources":{"tiles":{"type":"vector",` +
	`"tiles":["https://example.com/{z}/{x}/{y}.pbf"]}},"layers":[` +
	`{"id":"roads","type":"line","source":"tiles","source-layer":"transportation"},` +
	`{"id":"bg","type":"background"}]}`

// style layer listing copies the whole layer stack in style order,
// with absent source fields as nil rather than empty strings.
func TestStyleLayersListsLayerStackInStyleOrder(t *testing.T) {
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	m, err := awaitForTest(runtime.MapCreate(DefaultMapOptions()))
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := closeMapForTest(m); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := awaitForTest(m.SetStyleJSON([]byte(layerListStyleJSON))); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	layers, err := awaitForTest(m.ListStyleLayers())
	if err != nil {
		t.Fatalf("StyleLayers(): %v", err)
	}
	// Replace the native backing records before reading the copied result.
	if _, err := awaitForTest(m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`))); err != nil {
		t.Fatalf("replace style: %v", err)
	}
	want := []StyleLayerEntry{
		{ID: "roads", Type: "line", SourceID: pointerTo("tiles"), SourceLayer: pointerTo("transportation")},
		{ID: "bg", Type: "background"},
	}
	if len(layers) != len(want) {
		t.Fatalf("StyleLayers() = %+v, want %d layers", layers, len(want))
	}
	for i := range want {
		if !reflect.DeepEqual(layers[i], want[i]) {
			t.Errorf("StyleLayers()[%d] = %+v, want %+v", i, layers[i], want[i])
		}
	}
	if layers[1].SourceID != nil || layers[1].SourceLayer != nil {
		t.Errorf("background layer source fields = (%v, %v), want both absent", layers[1].SourceID, layers[1].SourceLayer)
	}
}

const layerAccessorStyleJSON = `{"version":8,"sources":{"geo":{"type":"geojson",` +
	`"data":{"type":"FeatureCollection","features":[]}}},"layers":[` +
	`{"id":"bg","type":"background"},{"id":"fill","type":"fill","source":"geo"}]}`

func TestLayerBaseAccessorsRoundTrip(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(layerAccessorStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}

	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("fill"))
	if err != nil || !found || info.SourceLayer != nil && *info.SourceLayer != "" {
		t.Fatalf("GetStyleLayerInfo(fill) source layer = %v, %v; want empty", info.SourceLayer, err)
	}
	if _, err := m.SetLayerSourceLayer("fill", pointerTo("roads")); err != nil {
		t.Fatalf("SetLayerSourceLayer(): %v", err)
	}
	info, found, err = takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("fill"))
	if err != nil || !found || (info.SourceLayer == nil || *info.SourceLayer != "roads") || (info.SourceID == nil || *info.SourceID != "geo") {
		t.Fatalf("GetStyleLayerInfo(fill) sources = (%v, %v, %v)", info.SourceID, info.SourceLayer, err)
	}

	// The narrow copies report the same values the aggregate carries.
	sourceLayer, err := awaitForTest(m.CopyLayerSourceLayer("fill"))
	if err != nil || (sourceLayer == nil || *sourceLayer != "roads") {
		t.Fatalf("LayerSourceLayer(fill) = (%q, %v), want roads", *sourceLayer, err)
	}
	sourceID, err := awaitForTest(m.CopyLayerSourceID("fill"))
	if err != nil || (sourceID == nil || *sourceID != "geo") {
		t.Fatalf("LayerSourceID(fill) = (%q, %v), want geo", *sourceID, err)
	}
	if _, err := awaitForTest(m.CopyLayerSourceID("missing")); !errors.Is(err, ErrNotFound) {
		t.Fatalf("LayerSourceID(missing) error = %v, want ErrNotFound", err)
	}

	// A layer type that takes no source is rejected rather than silently ignored.
	completion, err := m.SetLayerSourceLayer("bg", pointerTo("roads"))
	requireStyleCommandFailed(t, completion, err)
	background, found, queryErr := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("bg"))
	if queryErr != nil || !found || background.SourceID != nil {
		t.Fatalf("GetStyleLayerInfo(bg) source = %v, %v; want empty", background.SourceID, queryErr)
	}

	// An unset zoom range crosses the boundary as infinities.
	info, found, err = takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("fill"))
	if err != nil || !found {
		t.Fatalf("GetStyleLayerInfo(fill) = (%#v, %v, %v), want found", info, found, err)
	}
	if info.Info.Type != "fill" {
		t.Fatalf("GetStyleLayerInfo(fill) type = %q, want fill", info.Info.Type)
	}
	if !math.IsInf(info.Info.MinZoom, -1) || !math.IsInf(info.Info.MaxZoom, 1) {
		t.Fatalf("GetStyleLayerInfo(fill) zoom range = (%v, %v), want infinities", info.Info.MinZoom, info.Info.MaxZoom)
	}
	if _, err := m.SetLayerMinZoom("fill", 4); err != nil {
		t.Fatalf("SetLayerMinZoom(): %v", err)
	}
	if _, err := m.SetLayerMaxZoom("fill", 12.5); err != nil {
		t.Fatalf("SetLayerMaxZoom(): %v", err)
	}
	if info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("fill")); err != nil || !found || info.Info.MinZoom != 4 || info.Info.MaxZoom != 12.5 {
		t.Fatalf("GetStyleLayerInfo(fill) zoom range = (%v, %v, %v, %v); want 4 and 12.5", info.Info.MinZoom, info.Info.MaxZoom, found, err)
	}

	if info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("fill")); err != nil || !found || info.Info.Visibility != StyleLayerVisibilityVisible {
		t.Fatalf("GetStyleLayerInfo(fill) visibility = (%v, %v, %v); want visible", info.Info.Visibility, found, err)
	}
	if _, err := m.SetLayerVisibility("fill", StyleLayerVisibilityNone); err != nil {
		t.Fatalf("SetLayerVisibility(): %v", err)
	}
	if info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("fill")); err != nil || !found || info.Info.Visibility != StyleLayerVisibilityNone {
		t.Fatalf("GetStyleLayerInfo(fill) visibility = (%v, %v, %v); want none", info.Info.Visibility, found, err)
	}

	// An unknown raw visibility passes through to C, which rejects it.
	completion, err = m.SetLayerVisibility("fill", StyleLayerVisibility(900))
	requireStyleCommandFailed(t, completion, err)

	// A background layer carries neither a source ID nor a source layer.
	if info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("bg")); err != nil || !found ||
		info.SourceID != nil || info.SourceLayer != nil && *info.SourceLayer != "" {
		t.Fatalf("GetStyleLayerInfo(bg) = (%#v, %v, %v), want found without source fields", info, found, err)
	}

	// Removing an existing layer commits, and the info getter stops finding it.
	completion, err = m.RemoveStyleLayer("fill")
	requireCommandCommitted(t, completion, err)
	if _, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("fill")); err != nil || found {
		t.Fatalf("GetStyleLayerInfo(fill) after removal = (%v, %v), want (false, nil)", found, err)
	}
}

func TestStyleLayerJSONAndPropertySnapshots(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if _, err := m.AddStyleSourceJSON("points", []byte(`{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}`)); err != nil {
		t.Fatalf("AddStyleSourceJSON(points): %v", err)
	}
	layerJSON := []byte(`{"id":"points-layer","type":"circle","source":"points","paint":{"circle-radius":2}}`)
	if _, err := m.AddStyleLayerJSON(layerJSON, nil); err != nil {
		t.Fatalf("AddStyleLayerJSON(): %v", err)
	}
	layerJSON[0] = 'x'
	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("points-layer"))
	if err != nil {
		t.Fatalf("GetStyleLayerInfo(): %v", err)
	}
	if !found || info.Info.Type != "circle" {
		t.Fatalf("GetStyleLayerInfo(points-layer) type = (%q, %v), want circle true", info.Info.Type, found)
	}
	copiedLayer, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerJSON("points-layer"))
	if err != nil {
		t.Fatalf("StyleLayerJSON(): %v", err)
	}
	if !found {
		t.Fatalf("StyleLayerJSON(points-layer) found = false, want true")
	}
	if !bytes.Contains(copiedLayer, []byte(`"type":"circle"`)) {
		t.Fatalf("StyleLayerJSON(points-layer) = %s, want copied circle object", copiedLayer)
	}
	if _, err := m.SetLayerProperty("points-layer", "circle-radius", []byte("5")); err != nil {
		t.Fatalf("SetLayerProperty(circle-radius): %v", err)
	}
	property, err := awaitForTest(m.GetLayerProperty("points-layer", "circle-radius"))
	if err != nil {
		t.Fatalf("LayerProperty(circle-radius): %v", err)
	}
	if property == nil || string(*property) != "5.0" {
		t.Fatalf("LayerProperty(circle-radius) = %s, want 5", *property)
	}
	filter := []byte(`["==",["get","kind"],"unit"]`)
	if _, err := m.SetLayerFilter("points-layer", &filter); err != nil {
		t.Fatalf("SetLayerFilter(): %v", err)
	}
	gotFilter, found, err := takeOptionalStyleOperationForTest(m.GetLayerFilter("points-layer"))
	if err != nil {
		t.Fatalf("LayerFilter(): %v", err)
	}
	if !found || len(gotFilter) == 0 {
		t.Fatalf("LayerFilter() = nil, want copied filter")
	}
	if _, err := m.SetLayerFilter("points-layer", nil); err != nil {
		t.Fatalf("SetLayerFilter(nil): %v", err)
	}
	completion, err := m.SetLayerProperty("points-layer", "circle-radius", []byte("NaN"))
	requireStyleCommandFailed(t, completion, err)
	if _, err := awaitForTest(m.GetLayerProperty("missing", "circle-radius")); !errors.Is(err, ErrNotFound) {
		t.Fatalf("LayerProperty(missing layer) error = %v, want ErrNotFound", err)
	}
}

func TestStyleLightPropertyJSONSnapshots(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if _, err := m.SetStyleLightJSON([]byte(`{"anchor":"viewport","color":"#ffffff","intensity":0.5}`)); err != nil {
		t.Fatalf("SetStyleLightJSON(): %v", err)
	}
	undefined, err := awaitForTest(m.GetStyleLightProperty("does-not-exist"))
	if err != nil {
		t.Fatalf("StyleLightProperty(does-not-exist): %v", err)
	}
	if undefined != nil {
		t.Fatalf("StyleLightProperty(does-not-exist) = %#v, want JSON null", undefined)
	}
	if _, err := m.SetStyleLightProperty("intensity", []byte("0.75")); err != nil {
		t.Fatalf("SetStyleLightProperty(intensity): %v", err)
	}
	intensity, err := awaitForTest(m.GetStyleLightProperty("intensity"))
	if err != nil {
		t.Fatalf("StyleLightProperty(intensity): %v", err)
	}
	if intensity == nil || string(*intensity) != "0.75" {
		t.Fatalf("StyleLightProperty(intensity) = %s, want 0.75", *intensity)
	}
	completion, err := m.SetStyleLightProperty("intensity", []byte("-Infinity"))
	requireStyleCommandFailed(t, completion, err)
}

func TestStyleLayerMetadataForMissingLayers(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := awaitForTest(m.ListStyleLayerIDs())
	if err != nil {
		t.Fatalf("StyleLayerIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleLayerIDs() unexpectedly contains missing layer: %v", ids)
		}
	}
	info, found, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("missing"))
	if err != nil {
		t.Fatalf("GetStyleLayerInfo(): %v", err)
	}
	if found || info.Info.Type != "" {
		t.Fatalf("GetStyleLayerInfo(missing) = (%#v, %v), want empty false", info, found)
	}
	completion, err := m.RemoveStyleLayer("missing")
	requireCommandFailedWith(t, completion, err, ErrNotFound)
	completion, err = m.MoveStyleLayer("missing", nil)
	requireStyleCommandFailed(t, completion, err)
	if _, _, err := takeOptionalStyleOperationForTest(m.GetStyleLayerInfo("")); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("GetStyleLayerInfo(empty) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleTransitionOptionsRoundTrip(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	defaults, err := awaitForTest(m.GetStyleTransitionOptions())
	if err != nil {
		t.Fatalf("GetStyleTransitionOptions(): %v", err)
	}
	// The placement flag always reports, because native always holds a value for it.
	if defaults.DurationMS != nil || defaults.DelayMS != nil {
		t.Fatalf("GetStyleTransitionOptions() = %#v, want no duration or delay", defaults)
	}
	if defaults.EnablePlacementTransitions == nil || !*defaults.EnablePlacementTransitions {
		t.Fatalf("GetStyleTransitionOptions() = %#v, want the cross-fade on", defaults)
	}

	// The style parser fills in its own 300ms duration for a style that declares no transition.
	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	parsed, err := awaitForTest(m.GetStyleTransitionOptions())
	if err != nil {
		t.Fatalf("GetStyleTransitionOptions(): %v", err)
	}
	if parsed.DurationMS == nil || *parsed.DurationMS != 300 {
		t.Fatalf("parsed DurationMS = %v, want 300", parsed.DurationMS)
	}
	if parsed.DelayMS != nil {
		t.Fatalf("parsed DelayMS = %v, want absent", *parsed.DelayMS)
	}

	const transitionStyle = `{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}`
	if _, err := m.SetStyleJSON([]byte(transitionStyle)); err != nil {
		t.Fatalf("SetStyleJSON(transition style): %v", err)
	}
	declared, err := awaitForTest(m.GetStyleTransitionOptions())
	if err != nil {
		t.Fatalf("GetStyleTransitionOptions(): %v", err)
	}
	if declared.DurationMS == nil || *declared.DurationMS != 750 {
		t.Fatalf("declared DurationMS = %v, want 750", declared.DurationMS)
	}
	if declared.DelayMS == nil || *declared.DelayMS != 100 {
		t.Fatalf("declared DelayMS = %v, want 100", declared.DelayMS)
	}
	if declared.EnablePlacementTransitions == nil || !*declared.EnablePlacementTransitions {
		t.Fatal("declared EnablePlacementTransitions = false, want true")
	}

	// A present zero stays distinguishable from an absent field, and an absent field clears
	// what the style declared rather than merging into it.
	zero := 0.0
	disabled := false
	options := StyleTransitionOptions{DurationMS: &zero, EnablePlacementTransitions: &disabled}
	if _, err := m.SetStyleTransitionOptions(options); err != nil {
		t.Fatalf("SetStyleTransitionOptions(): %v", err)
	}
	applied, err := awaitForTest(m.GetStyleTransitionOptions())
	if err != nil {
		t.Fatalf("GetStyleTransitionOptions(): %v", err)
	}
	if !reflect.DeepEqual(applied, options) {
		t.Fatalf("GetStyleTransitionOptions() = %#v, want %#v", applied, options)
	}

	// A literal that sets only a duration must leave the cross-fade alone. Go cannot default a
	// struct field to true, so an enable-shaped field would have disabled it here.
	durationOnly := 250.0
	if _, err := m.SetStyleTransitionOptions(StyleTransitionOptions{DurationMS: &durationOnly}); err != nil {
		t.Fatalf("SetStyleTransitionOptions(): %v", err)
	}
	kept, err := awaitForTest(m.GetStyleTransitionOptions())
	if err != nil {
		t.Fatalf("GetStyleTransitionOptions(): %v", err)
	}
	if kept.EnablePlacementTransitions == nil || !*kept.EnablePlacementTransitions {
		t.Fatal("a duration-only literal disabled the placement cross-fade")
	}

	if _, err := m.SetStyleJSON([]byte(transitionStyle)); err != nil {
		t.Fatalf("SetStyleJSON(transition style): %v", err)
	}
	reloaded, err := awaitForTest(m.GetStyleTransitionOptions())
	if err != nil {
		t.Fatalf("GetStyleTransitionOptions(): %v", err)
	}
	if !reflect.DeepEqual(reloaded, declared) {
		t.Fatalf("GetStyleTransitionOptions() = %#v, want %#v", reloaded, declared)
	}

	negative := -1.0
	completion, err := m.SetStyleTransitionOptions(StyleTransitionOptions{DelayMS: &negative})
	requireStyleCommandFailed(t, completion, err)
}
