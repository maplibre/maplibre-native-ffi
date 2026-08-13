package maplibre

import (
	"errors"
	"slices"
	"testing"
	"time"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
)

// backgroundStyleJSON declares one layer, so a replacement style is a different
// document and the map reports a second style load.
const backgroundStyleJSON = `{"version":8,"sources":{},"layers":` +
	`[{"id":"background","type":"background"}]}`

// liveCustomGeometrySources reports how many custom geometry callback states the
// binding still holds, relative to the count when the test started. The C API
// frees a state through the release callback, so this reaching zero is the only
// binding-visible proof that the release ran.
func liveCustomGeometrySources(t *testing.T, baseline int64) int64 {
	t.Helper()
	return callback.CustomGeometrySourceLiveCountForTest() - baseline
}

// loadStyleAndCollect loads an inline style and pumps until the map settles,
// returning every event it drained.
func loadStyleAndCollect(t *testing.T, runtime *RuntimeHandle, m *MapHandle, style string) []RuntimeEvent {
	t.Helper()
	if err := m.SetStyleJSON([]byte(style)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	var events []RuntimeEvent
	for range make([]struct{}, 200) {
		if err := runtime.Pump(2 * time.Millisecond); err != nil {
			t.Fatalf("Pump(): %v", err)
		}
		batch, err := runtime.DrainEvents(0)
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		events = append(events, batch.Events...)
	}
	return events
}

func TestCustomGeometrySourceDescriptors(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	baseline := callback.CustomGeometrySourceLiveCountForTest()
	loadStyleAndCollect(t, runtime, m, emptyStyleJSON)

	minZoom := 0.0
	maxZoom := 2.0
	tolerance := 0.375
	tileSize := uint32(512)
	buffer := uint32(64)
	clip := true
	wrap := false
	fetches := 0
	cancels := 0
	if err := m.AddCustomGeometrySource("custom", CustomGeometrySourceOptions{
		FetchTile:  func(CanonicalTileID) { fetches++ },
		CancelTile: func(CanonicalTileID) { cancels++ },
		MinZoom:    &minZoom,
		MaxZoom:    &maxZoom,
		Tolerance:  &tolerance,
		TileSize:   &tileSize,
		Buffer:     &buffer,
		Clip:       &clip,
		Wrap:       &wrap,
	}); err != nil {
		t.Fatalf("AddCustomGeometrySource(): %v", err)
	}
	if fetches != 0 || cancels != 0 {
		t.Fatalf("callbacks invoked during registration: fetches=%d cancels=%d", fetches, cancels)
	}
	tileID := CanonicalTileID{Z: 0, X: 0, Y: 0}
	if err := m.SetCustomGeometrySourceTileData("custom", tileID, []byte(`{"type":"FeatureCollection","features":[]}`)); err != nil {
		t.Fatalf("SetCustomGeometrySourceTileData(): %v", err)
	}
	if err := m.InvalidateCustomGeometrySourceTile("custom", tileID); err != nil {
		t.Fatalf("InvalidateCustomGeometrySourceTile(): %v", err)
	}
	if err := m.InvalidateCustomGeometrySourceRegion("custom", LatLngBounds{Southwest: LatLng{Latitude: -1, Longitude: -1}, Northeast: LatLng{Latitude: 1, Longitude: 1}}); err != nil {
		t.Fatalf("InvalidateCustomGeometrySourceRegion(): %v", err)
	}
	removed, err := m.RemoveStyleSource("custom")
	if err != nil {
		t.Fatalf("RemoveStyleSource(custom): %v", err)
	}
	if !removed {
		t.Fatal("RemoveStyleSource(custom) removed=false, want true")
	}
	if err := m.AddCustomGeometrySource("bad-custom", CustomGeometrySourceOptions{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddCustomGeometrySource(nil fetch) error = %v, want ErrInvalidArgument", err)
	}
	// A native add the style rejects owes no release callback, so the binding
	// frees the state it built for it before returning.
	if err := m.AddCustomGeometrySource("", CustomGeometrySourceOptions{
		FetchTile: func(CanonicalTileID) {},
	}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddCustomGeometrySource(empty ID) error = %v, want ErrInvalidArgument", err)
	}
	if live := liveCustomGeometrySources(t, baseline); live != 0 {
		t.Fatalf("live callback states after removal and a rejected add = %d, want 0", live)
	}
}

// A host that never selects style-loaded events still gets its callback state
// freed when a style replacement drops the source, and the mask it set stays
// exactly what it set.
func TestCustomGeometrySourceReleasedWhenStyleLoadDropsIt(t *testing.T) {
	mask := RuntimeEventMaskAll &^ RuntimeEventMaskMapStyleLoaded
	options := NewMapOptions(64, 64, 1)
	options.EventMask = mask
	runtime, m := newRuntimeAndMap(t, &options)
	baseline := callback.CustomGeometrySourceLiveCountForTest()

	loadStyleAndCollect(t, runtime, m, backgroundStyleJSON)
	if err := m.AddCustomGeometrySource("custom", CustomGeometrySourceOptions{
		FetchTile: func(CanonicalTileID) {},
	}); err != nil {
		t.Fatalf("AddCustomGeometrySource(): %v", err)
	}
	if live := liveCustomGeometrySources(t, baseline); live != 1 {
		t.Fatalf("live callback states after the add = %d, want 1", live)
	}
	// The binding installs the mask the host chose, and nothing else.
	if got, err := m.EventMask(); err != nil || got != mask {
		t.Fatalf("EventMask() = (%#x, %v), want %#x", uint64(got), err, uint64(mask))
	}

	events := loadStyleAndCollect(t, runtime, m, emptyStyleJSON)
	if live := liveCustomGeometrySources(t, baseline); live != 0 {
		t.Fatalf("live callback states after the style replacement = %d, want 0", live)
	}
	if slices.Contains(eventTypes(events), RuntimeEventMapStyleLoaded) {
		t.Fatal("drained a style-loaded event the map's mask cleared")
	}
}

func TestCustomGeometrySourceReleasedByRemovalAndMapClose(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	baseline := callback.CustomGeometrySourceLiveCountForTest()
	loadStyleAndCollect(t, runtime, m, backgroundStyleJSON)

	for _, sourceID := range []string{"removed", "surviving"} {
		if err := m.AddCustomGeometrySource(sourceID, CustomGeometrySourceOptions{
			FetchTile: func(CanonicalTileID) {},
		}); err != nil {
			t.Fatalf("AddCustomGeometrySource(%s): %v", sourceID, err)
		}
	}
	if removed, err := m.RemoveStyleSource("removed"); err != nil || !removed {
		t.Fatalf("RemoveStyleSource(removed) = (%v, %v), want (true, nil)", removed, err)
	}
	if live := liveCustomGeometrySources(t, baseline); live != 1 {
		t.Fatalf("live callback states after the removal = %d, want 1", live)
	}

	// The map still holds the surviving source, so its teardown frees the state.
	if err := m.Close(); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	if live := liveCustomGeometrySources(t, baseline); live != 0 {
		t.Fatalf("live callback states after the map close = %d, want 0", live)
	}
}
