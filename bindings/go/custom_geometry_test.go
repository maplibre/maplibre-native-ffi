package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
)

func TestCustomGeometrySourceDescriptors(t *testing.T) {
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
	if err := m.SetCustomGeometrySourceTileData("custom", tileID, GeoJSONFeatureCollection(nil)); err != nil {
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
}
func drainRuntimeEvents(t *testing.T, runtime *RuntimeHandle) {
	t.Helper()
	for range make([]struct{}, 100) {
		if err := runtime.RunOnce(); err != nil {
			t.Fatalf("RunOnce(): %v", err)
		}
		event, err := runtime.PollEvent()
		if err != nil {
			t.Fatalf("PollEvent(): %v", err)
		}
		if event == nil {
			return
		}
	}
}
func TestCustomGeometrySourceStateReleasesAfterStyleURLReplacement(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	const emptyStyle = `{"version":8,"sources":{},"layers":[]}`

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if err := runtime.SetResourceProvider(func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if request.URL != "custom://style.json" {
			return ResourceProviderDecisionPassThrough
		}
		if err := handle.Complete(ResourceResponse{Status: ResourceResponseStatusOK, Bytes: []byte(emptyStyle)}); err != nil {
			return ResourceProviderDecisionPassThrough
		}
		return ResourceProviderDecisionHandle
	}); err != nil {
		_ = runtime.Close()
		t.Fatalf("SetResourceProvider(): %v", err)
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

	if err := m.SetStyleJSON(emptyStyle); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	drainRuntimeEvents(t, runtime)
	if err := m.AddCustomGeometrySource("custom", CustomGeometrySourceOptions{FetchTile: func(CanonicalTileID) {}}); err != nil {
		t.Fatalf("AddCustomGeometrySource(): %v", err)
	}
	if got := m.customGeometrySourceCountForTesting(); got != 1 {
		t.Fatalf("customGeometrySourceCountForTesting() = %d, want 1", got)
	}

	if err := m.SetStyleURL("custom://style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	waitForRuntimeEvent(t, runtime, RuntimeEventMapStyleLoaded)
	if got := m.customGeometrySourceCountForTesting(); got != 0 {
		t.Fatalf("customGeometrySourceCountForTesting() = %d, want 0", got)
	}
}
