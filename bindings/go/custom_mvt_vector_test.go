package maplibre

import (
	"errors"
	"slices"
	"testing"
)

func liveCustomMVTVectorSources(baseline int64) int64 {
	return bindingCallbackCount.Load() - baseline
}

func TestCustomMVTVectorSourceDescriptors(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	baseline := bindingCallbackCount.Load()
	loadStyleForTest(t, runtime, m, emptyStyleJSON)

	minZoom := 0.0
	maxZoom := 2.0
	fetches := 0
	cancels := 0
	if _, err := m.AddCustomMvtVectorSource("custom-mvt", CustomMvtVectorSourceOptions{
		FetchTile:  func(CanonicalTileId) { fetches++ },
		CancelTile: func(CanonicalTileId) { cancels++ },
		MinZoom:    &minZoom,
		MaxZoom:    &maxZoom,
	}); err != nil {
		t.Fatalf("AddCustomMvtVectorSource(): %v", err)
	}
	if fetches != 0 || cancels != 0 {
		t.Fatalf("callbacks invoked during registration: fetches=%d cancels=%d", fetches, cancels)
	}
	tileID := CanonicalTileId{Z: 0, X: 0, Y: 0}
	if _, err := m.SetCustomMvtVectorSourceTileData("custom-mvt", tileID, nil); err != nil {
		t.Fatalf("SetCustomMvtVectorSourceTileData(): %v", err)
	}
	if _, err := m.SetCustomMvtVectorSourceTileError("custom-mvt", tileID, "tile missing"); err != nil {
		t.Fatalf("SetCustomMvtVectorSourceTileError(): %v", err)
	}
	if _, err := m.InvalidateCustomMvtVectorSourceTile("custom-mvt", tileID); err != nil {
		t.Fatalf("InvalidateCustomMvtVectorSourceTile(): %v", err)
	}
	source, found, err := takeOptionalStyleOperationForTest(m.GetStyleSourceInfo("custom-mvt"))
	if err != nil || !found || source.Info.Type != StyleSourceTypeCustomMvtVector {
		t.Fatalf("GetStyleSourceInfo(custom-mvt) = (%#v, %v, %v), want a found CustomMVTVector source", source, found, err)
	}
	removeID, err := m.RemoveStyleSource("custom-mvt")
	requireCommandCommitted(t, removeID, err)
	if live := liveCustomMVTVectorSources(baseline); live != 0 {
		t.Fatalf("live callback states after removal = %d, want 0", live)
	}

	if _, err := m.AddCustomMvtVectorSource("bad-custom", CustomMvtVectorSourceOptions{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddCustomMvtVectorSource(nil fetch) error = %v, want ErrInvalidArgument", err)
	}
	rejected, err := m.AddCustomMvtVectorSource("", CustomMvtVectorSourceOptions{FetchTile: func(CanonicalTileId) {}})
	requireCommandFailedWith(t, rejected, err, ErrInvalidArgument)
	// The failed command releases its captured descriptor after completion returns.
	waitForRuntimeBarrier(t, runtime)
	if live := liveCustomMVTVectorSources(baseline); live != 0 {
		t.Fatalf("live callback states after rejected adds = %d, want 0", live)
	}
}

func TestCustomMVTVectorSourceReleasedWhenStyleLoadDropsIt(t *testing.T) {
	mask := RuntimeEventMaskAll &^ RuntimeEventMaskMapStyleLoaded
	options := mapOptionsForTest(64, 64, 1)
	options.EventMask = mask
	runtime, m := newRuntimeAndMap(t, &options)
	baseline := bindingCallbackCount.Load()

	loadStyleForTest(t, runtime, m, backgroundStyleJSON)
	if _, err := m.AddCustomMvtVectorSource("custom-mvt", CustomMvtVectorSourceOptions{
		FetchTile: func(CanonicalTileId) {},
	}); err != nil {
		t.Fatalf("AddCustomMvtVectorSource(): %v", err)
	}
	if live := liveCustomMVTVectorSources(baseline); live != 1 {
		t.Fatalf("live callback states after the add = %d, want 1", live)
	}
	if got := mapEventMaskForTest(t, m); got != mask {
		t.Fatalf("MapSnapshot.EventMask = %#x, want %#x", uint64(got), uint64(mask))
	}

	events := loadStyleAndDrain(t, runtime, m, emptyStyleJSON)
	if live := liveCustomMVTVectorSources(baseline); live != 0 {
		t.Fatalf("live callback states after the style replacement = %d, want 0", live)
	}
	if slices.Contains(eventTypes(events), RuntimeEventTypeMapStyleLoaded) {
		t.Fatal("drained a style-loaded event the map's mask cleared")
	}
}

func TestCustomMVTVectorSourceReleasedByRemovalAndMapClose(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)
	baseline := bindingCallbackCount.Load()
	loadStyleForTest(t, runtime, m, backgroundStyleJSON)

	for _, sourceID := range []string{"removed", "surviving"} {
		if _, err := m.AddCustomMvtVectorSource(sourceID, CustomMvtVectorSourceOptions{
			FetchTile: func(CanonicalTileId) {},
		}); err != nil {
			t.Fatalf("AddCustomMvtVectorSource(%s): %v", sourceID, err)
		}
	}
	removeID, err := m.RemoveStyleSource("removed")
	requireCommandCommitted(t, removeID, err)
	if live := liveCustomMVTVectorSources(baseline); live != 1 {
		t.Fatalf("live callback states after the removal = %d, want 1", live)
	}

	// The map still holds the surviving source, so its teardown frees the state.
	if err := closeMapForTest(m); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	waitForRuntimeBarrier(t, runtime)
	if live := liveCustomMVTVectorSources(baseline); live != 0 {
		t.Fatalf("live callback states after the map close = %d, want 0", live)
	}
}
