package maplibre

import (
	"bytes"
	"errors"
	"math"
	"testing"
)

func TestDedicatedStyleLayerHelpers(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	demOptions := StyleTileSourceOptions{}.WithTileSize(512).WithRasterEncoding(StyleRasterDEMEncodingMapbox)
	if _, err := m.AddRasterDEMSourceTiles("dem", []string{"https://example.com/dem/{z}/{x}/{y}.png"}, &demOptions); err != nil {
		t.Fatalf("AddRasterDEMSourceTiles(): %v", err)
	}
	if _, err := m.AddHillshadeLayer("hillshade", "dem", ""); err != nil {
		t.Fatalf("AddHillshadeLayer(): %v", err)
	}
	if _, err := m.AddColorReliefLayer("relief", "dem", "hillshade"); err != nil {
		t.Fatalf("AddColorReliefLayer(): %v", err)
	}
	if _, err := m.AddLocationIndicatorLayer("location", ""); err != nil {
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
		info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo(id))
		if err != nil {
			t.Fatalf("StyleLayerInfo(%s): %v", id, err)
		}
		if !found || info.Type != wantType {
			t.Fatalf("StyleLayerInfo(%s) type = (%q, %v), want %q true", id, info.Type, found, wantType)
		}
	}
	ids, err := takeStyleOperationForTest(m.StyleLayerIDs())
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
	requireStyleCommandFailed(t, runtime, completion, err)
	completion, err = m.AddHillshadeLayer("bad-hillshade", "missing", "")
	requireStyleCommandFailed(t, runtime, completion, err)
}

const layerAccessorStyleJSON = `{"version":8,"sources":{"geo":{"type":"geojson",` +
	`"data":{"type":"FeatureCollection","features":[]}}},"layers":[` +
	`{"id":"bg","type":"background"},{"id":"fill","type":"fill","source":"geo"}]}`

func TestLayerBaseAccessorsRoundTrip(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(layerAccessorStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}

	info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("fill"))
	if err != nil || !found || info.SourceLayer != nil {
		t.Fatalf("StyleLayerInfo(fill) source layer = %v, %v; want absent", info.SourceLayer, err)
	}
	if _, err := m.SetLayerSourceLayer("fill", "roads"); err != nil {
		t.Fatalf("SetLayerSourceLayer(): %v", err)
	}
	info, found, err = takeOptionalStyleOperationForTest(m.StyleLayerInfo("fill"))
	if err != nil || !found || info.SourceLayer == nil || *info.SourceLayer != "roads" || info.SourceID == nil || *info.SourceID != "geo" {
		t.Fatalf("StyleLayerInfo(fill) sources = (%v, %v, %v)", info.SourceID, info.SourceLayer, err)
	}

	// A layer type that takes no source is rejected rather than silently ignored.
	completion, err := m.SetLayerSourceLayer("bg", "roads")
	requireStyleCommandFailed(t, runtime, completion, err)
	background, found, queryErr := takeOptionalStyleOperationForTest(m.StyleLayerInfo("bg"))
	if queryErr != nil || !found || background.SourceID != nil {
		t.Fatalf("StyleLayerInfo(bg) source = %v, %v; want absent", background.SourceID, queryErr)
	}

	// An unset zoom range crosses the boundary as infinities, and the layer
	// info reports the source-ID and source-layer sizes that feed the copies.
	info, found, err = takeOptionalStyleOperationForTest(m.StyleLayerInfo("fill"))
	if err != nil || !found {
		t.Fatalf("StyleLayerInfo(fill) = (%#v, %v, %v), want found", info, found, err)
	}
	if info.Type != "fill" {
		t.Fatalf("StyleLayerInfo(fill) type = %q, want fill", info.Type)
	}
	if !math.IsInf(info.MinZoom, -1) || !math.IsInf(info.MaxZoom, 1) {
		t.Fatalf("StyleLayerInfo(fill) zoom range = (%v, %v), want infinities", info.MinZoom, info.MaxZoom)
	}
	if !info.HasSourceID || info.SourceIDSize != uint64(len("geo")) {
		t.Fatalf("StyleLayerInfo(fill) source ID size = (%v, %d), want (true, %d)", info.HasSourceID, info.SourceIDSize, len("geo"))
	}
	if !info.HasSourceLayer || info.SourceLayerSize != uint64(len("roads")) {
		t.Fatalf("StyleLayerInfo(fill) source layer size = (%v, %d), want (true, %d)", info.HasSourceLayer, info.SourceLayerSize, len("roads"))
	}
	if _, err := m.SetLayerMinZoom("fill", 4); err != nil {
		t.Fatalf("SetLayerMinZoom(): %v", err)
	}
	if _, err := m.SetLayerMaxZoom("fill", 12.5); err != nil {
		t.Fatalf("SetLayerMaxZoom(): %v", err)
	}
	if info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("fill")); err != nil || !found || info.MinZoom != 4 || info.MaxZoom != 12.5 {
		t.Fatalf("StyleLayerInfo(fill) zoom range = (%v, %v, %v, %v); want 4 and 12.5", info.MinZoom, info.MaxZoom, found, err)
	}

	if info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("fill")); err != nil || !found || info.Visibility != StyleLayerVisibilityVisible {
		t.Fatalf("StyleLayerInfo(fill) visibility = (%v, %v, %v); want visible", info.Visibility, found, err)
	}
	if _, err := m.SetLayerVisibility("fill", StyleLayerVisibilityNone); err != nil {
		t.Fatalf("SetLayerVisibility(): %v", err)
	}
	if info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("fill")); err != nil || !found || info.Visibility != StyleLayerVisibilityNone {
		t.Fatalf("StyleLayerInfo(fill) visibility = (%v, %v, %v); want none", info.Visibility, found, err)
	}

	// An unknown raw visibility passes through to C, which rejects it.
	completion, err = m.SetLayerVisibility("fill", StyleLayerVisibility(900))
	requireStyleCommandFailed(t, runtime, completion, err)

	// A background layer carries neither a source ID nor a source layer.
	if info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("bg")); err != nil || !found ||
		info.HasSourceID || info.SourceIDSize != 0 || info.HasSourceLayer || info.SourceLayerSize != 0 {
		t.Fatalf("StyleLayerInfo(bg) = (%#v, %v, %v), want found without source fields", info, found, err)
	}

	// Removing an existing layer commits, and the info getter stops finding it.
	completion, err = m.RemoveStyleLayer("fill")
	requireCommandCommitted(t, runtime, completion, err)
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("fill")); err != nil || found {
		t.Fatalf("StyleLayerInfo(fill) after removal = (%v, %v), want (false, nil)", found, err)
	}
}

func TestStyleLayerJSONAndPropertySnapshots(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if _, err := m.AddStyleSourceJSON("points", []byte(`{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}`)); err != nil {
		t.Fatalf("AddStyleSourceJSON(points): %v", err)
	}
	layerJSON := []byte(`{"id":"points-layer","type":"circle","source":"points","paint":{"circle-radius":2}}`)
	if _, err := m.AddStyleLayerJSON(layerJSON, ""); err != nil {
		t.Fatalf("AddStyleLayerJSON(): %v", err)
	}
	layerJSON[0] = 'x'
	info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("points-layer"))
	if err != nil {
		t.Fatalf("StyleLayerInfo(): %v", err)
	}
	if !found || info.Type != "circle" {
		t.Fatalf("StyleLayerInfo(points-layer) type = (%q, %v), want circle true", info.Type, found)
	}
	copiedLayer, found, err := takeOptionalStyleOperationForTest(m.StyleLayerJSON("points-layer"))
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
	property, err := takeStyleOperationForTest(m.LayerProperty("points-layer", "circle-radius"))
	if err != nil {
		t.Fatalf("LayerProperty(circle-radius): %v", err)
	}
	if string(property) != "5.0" {
		t.Fatalf("LayerProperty(circle-radius) = %s, want 5", property)
	}
	filter := []byte(`["==",["get","kind"],"unit"]`)
	if _, err := m.SetLayerFilter("points-layer", filter); err != nil {
		t.Fatalf("SetLayerFilter(): %v", err)
	}
	gotFilter, found, err := takeOptionalStyleOperationForTest(m.LayerFilter("points-layer"))
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
	requireStyleCommandFailed(t, runtime, completion, err)
	if _, err := takeStyleOperationForTest(m.LayerProperty("missing", "circle-radius")); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("LayerProperty(missing layer) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleLightPropertyJSONSnapshots(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if _, err := m.SetStyleLightJSON([]byte(`{"anchor":"viewport","color":"#ffffff","intensity":0.5}`)); err != nil {
		t.Fatalf("SetStyleLightJSON(): %v", err)
	}
	undefined, err := takeStyleOperationForTest(m.StyleLightProperty("does-not-exist"))
	if err != nil {
		t.Fatalf("StyleLightProperty(does-not-exist): %v", err)
	}
	if undefined != nil {
		t.Fatalf("StyleLightProperty(does-not-exist) = %#v, want JSON null", undefined)
	}
	if _, err := m.SetStyleLightProperty("intensity", []byte("0.75")); err != nil {
		t.Fatalf("SetStyleLightProperty(intensity): %v", err)
	}
	intensity, err := takeStyleOperationForTest(m.StyleLightProperty("intensity"))
	if err != nil {
		t.Fatalf("StyleLightProperty(intensity): %v", err)
	}
	if string(intensity) != "0.75" {
		t.Fatalf("StyleLightProperty(intensity) = %s, want 0.75", intensity)
	}
	completion, err := m.SetStyleLightProperty("intensity", []byte("-Infinity"))
	requireStyleCommandFailed(t, runtime, completion, err)
}

func TestStyleLayerMetadataForMissingLayers(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := takeStyleOperationForTest(m.StyleLayerIDs())
	if err != nil {
		t.Fatalf("StyleLayerIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleLayerIDs() unexpectedly contains missing layer: %v", ids)
		}
	}
	info, found, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("missing"))
	if err != nil {
		t.Fatalf("StyleLayerInfo(): %v", err)
	}
	if found || info.Type != "" {
		t.Fatalf("StyleLayerInfo(missing) = (%#v, %v), want empty false", info, found)
	}
	completion, err := m.RemoveStyleLayer("missing")
	requireCommandFailedWith(t, runtime, completion, err, ErrNotFound)
	completion, err = m.MoveStyleLayer("missing", "")
	requireStyleCommandFailed(t, runtime, completion, err)
	if _, _, err := takeOptionalStyleOperationForTest(m.StyleLayerInfo("")); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StyleLayerInfo(empty) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleTransitionOptionsRoundTrip(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	defaults, err := takeStyleOperationForTest(m.StyleTransitionOptions())
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	// The placement flag always reports, because native always holds a value for it.
	if defaults.DurationMS != nil || defaults.DelayMS != nil {
		t.Fatalf("StyleTransitionOptions() = %#v, want no duration or delay", defaults)
	}
	if defaults.EnablePlacementTransitions == nil || !*defaults.EnablePlacementTransitions {
		t.Fatalf("StyleTransitionOptions() = %#v, want the cross-fade on", defaults)
	}

	// The style parser fills in its own 300ms duration for a style that declares no transition.
	if _, err := m.SetStyleJSON([]byte(`{"version":8,"sources":{},"layers":[]}`)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	parsed, err := takeStyleOperationForTest(m.StyleTransitionOptions())
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
	if _, err := m.SetStyleJSON([]byte(transitionStyle)); err != nil {
		t.Fatalf("SetStyleJSON(transition style): %v", err)
	}
	declared, err := takeStyleOperationForTest(m.StyleTransitionOptions())
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
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
	applied, err := takeStyleOperationForTest(m.StyleTransitionOptions())
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if !applied.Equal(options) {
		t.Fatalf("StyleTransitionOptions() = %#v, want %#v", applied, options)
	}

	// A literal that sets only a duration must leave the cross-fade alone. Go cannot default a
	// struct field to true, so an enable-shaped field would have disabled it here.
	durationOnly := 250.0
	if _, err := m.SetStyleTransitionOptions(StyleTransitionOptions{DurationMS: &durationOnly}); err != nil {
		t.Fatalf("SetStyleTransitionOptions(): %v", err)
	}
	kept, err := takeStyleOperationForTest(m.StyleTransitionOptions())
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if kept.EnablePlacementTransitions == nil || !*kept.EnablePlacementTransitions {
		t.Fatal("a duration-only literal disabled the placement cross-fade")
	}

	if _, err := m.SetStyleJSON([]byte(transitionStyle)); err != nil {
		t.Fatalf("SetStyleJSON(transition style): %v", err)
	}
	reloaded, err := takeStyleOperationForTest(m.StyleTransitionOptions())
	if err != nil {
		t.Fatalf("StyleTransitionOptions(): %v", err)
	}
	if !reloaded.Equal(declared) {
		t.Fatalf("StyleTransitionOptions() = %#v, want %#v", reloaded, declared)
	}

	negative := -1.0
	completion, err := m.SetStyleTransitionOptions(StyleTransitionOptions{DelayMS: &negative})
	requireStyleCommandFailed(t, runtime, completion, err)
}
