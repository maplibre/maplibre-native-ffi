package maplibre

import (
	"errors"
	"slices"
	"testing"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
)

func liveCustomMVTVectorSources(t *testing.T, baseline int64) int64 {
	t.Helper()
	return callback.CustomMVTVectorSourceLiveCountForTest() - baseline
}

func TestCustomMVTVectorSourceDescriptors(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	baseline := callback.CustomMVTVectorSourceLiveCountForTest()
	loadStyleAndCollect(t, runtime, m, emptyStyleJSON)

	minZoom := 0.0
	maxZoom := 2.0
	fetches := 0
	cancels := 0
	if _, err := m.AddCustomMVTVectorSource("custom-mvt", CustomMVTVectorSourceOptions{
		FetchTile:  func(CanonicalTileID) { fetches++ },
		CancelTile: func(CanonicalTileID) { cancels++ },
		MinZoom:    &minZoom,
		MaxZoom:    &maxZoom,
	}); err != nil {
		t.Fatalf("AddCustomMVTVectorSource(): %v", err)
	}
	if fetches != 0 || cancels != 0 {
		t.Fatalf("callbacks invoked during registration: fetches=%d cancels=%d", fetches, cancels)
	}
	tileID := CanonicalTileID{Z: 0, X: 0, Y: 0}
	if _, err := m.SetCustomMVTVectorSourceTileData("custom-mvt", tileID, nil); err != nil {
		t.Fatalf("SetCustomMVTVectorSourceTileData(): %v", err)
	}
	if _, err := m.SetCustomMVTVectorSourceTileError("custom-mvt", tileID, "tile missing"); err != nil {
		t.Fatalf("SetCustomMVTVectorSourceTileError(): %v", err)
	}
	if _, err := m.InvalidateCustomMVTVectorSourceTile("custom-mvt", tileID); err != nil {
		t.Fatalf("InvalidateCustomMVTVectorSourceTile(): %v", err)
	}
	source, err := awaitForTest(m.StyleSourceInfo("custom-mvt"))
	if err != nil || !source.Found || source.Value.Type != StyleSourceTypeCustomMVTVector {
		t.Fatalf("StyleSourceInfo(custom-mvt) = (%#v, %v), want a found CustomMVTVector source", source, err)
	}
	removeID, err := m.RemoveStyleSource("custom-mvt")
	requireCommandCommitted(t, runtime, removeID, err)
	if live := liveCustomMVTVectorSources(t, baseline); live != 0 {
		t.Fatalf("live callback states after removal = %d, want 0", live)
	}

	if _, err := m.AddCustomMVTVectorSource("bad-custom", CustomMVTVectorSourceOptions{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddCustomMVTVectorSource(nil fetch) error = %v, want ErrInvalidArgument", err)
	}
	// A native add the style rejects owes no release callback, so the binding
	// frees the state it built for it before returning.
	if _, err := m.AddCustomMVTVectorSource("", CustomMVTVectorSourceOptions{
		FetchTile: func(CanonicalTileID) {},
	}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddCustomMVTVectorSource(empty ID) error = %v, want ErrInvalidArgument", err)
	}
	if live := liveCustomMVTVectorSources(t, baseline); live != 0 {
		t.Fatalf("live callback states after rejected adds = %d, want 0", live)
	}
}

func TestCustomMVTVectorSourceReleasedWhenStyleLoadDropsIt(t *testing.T) {
	mask := RuntimeEventMaskAll &^ RuntimeEventMaskMapStyleLoaded
	options := NewMapOptions(64, 64, 1)
	options.EventMask = mask
	runtime, m := newRuntimeAndMap(t, &options)
	baseline := callback.CustomMVTVectorSourceLiveCountForTest()

	loadStyleAndCollect(t, runtime, m, backgroundStyleJSON)
	if _, err := m.AddCustomMVTVectorSource("custom-mvt", CustomMVTVectorSourceOptions{
		FetchTile: func(CanonicalTileID) {},
	}); err != nil {
		t.Fatalf("AddCustomMVTVectorSource(): %v", err)
	}
	if live := liveCustomMVTVectorSources(t, baseline); live != 1 {
		t.Fatalf("live callback states after the add = %d, want 1", live)
	}
	if got, err := m.EventMask(); err != nil || got != mask {
		t.Fatalf("EventMask() = (%#x, %v), want %#x", uint64(got), err, uint64(mask))
	}

	events := loadStyleAndCollect(t, runtime, m, emptyStyleJSON)
	if live := liveCustomMVTVectorSources(t, baseline); live != 0 {
		t.Fatalf("live callback states after the style replacement = %d, want 0", live)
	}
	if slices.Contains(eventTypes(events), RuntimeEventMapStyleLoaded) {
		t.Fatal("drained a style-loaded event the map's mask cleared")
	}
}

func TestCustomMVTVectorSourceReleasedByRemovalAndMapClose(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	baseline := callback.CustomMVTVectorSourceLiveCountForTest()
	loadStyleAndCollect(t, runtime, m, backgroundStyleJSON)

	for _, sourceID := range []string{"removed", "surviving"} {
		if _, err := m.AddCustomMVTVectorSource(sourceID, CustomMVTVectorSourceOptions{
			FetchTile: func(CanonicalTileID) {},
		}); err != nil {
			t.Fatalf("AddCustomMVTVectorSource(%s): %v", sourceID, err)
		}
	}
	removeID, err := m.RemoveStyleSource("removed")
	requireCommandCommitted(t, runtime, removeID, err)
	if live := liveCustomMVTVectorSources(t, baseline); live != 1 {
		t.Fatalf("live callback states after the removal = %d, want 1", live)
	}

	// The map still holds the surviving source, so its teardown frees the state.
	if err := m.Close(); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	waitForRuntimeBarrier(t, runtime)
	if live := liveCustomMVTVectorSources(t, baseline); live != 0 {
		t.Fatalf("live callback states after the map close = %d, want 0", live)
	}
}
