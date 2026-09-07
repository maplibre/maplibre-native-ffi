package maplibre

import (
	"errors"
	"math"
	"testing"
)

func TestRuntimeMapLifecycle(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}

	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	if err := closeRuntimeForTest(runtime); !errors.Is(err, ErrInvalidState) {
		_ = closeMapForTest(m)
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("Close() with live map error = %v, want ErrInvalidState", err)
	} else {
		var bindingErr *Error
		if !errors.As(err, &bindingErr) ||
			bindingErr.Diagnostic() != "handle still owns live or pending children" {
			_ = closeMapForTest(m)
			_ = closeRuntimeForTest(runtime)
			t.Fatalf("Close() with live map diagnostic = %v", err)
		}
	}
	if err := closeMapForTest(m); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("Map Close(): %v", err)
	}
	if err := closeMapForTest(m); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("second Map Close(): %v", err)
	}
	if err := closeRuntimeForTest(runtime); err != nil {
		t.Fatalf("Runtime Close(): %v", err)
	}
}

func TestMapIDIdentifiesEachMapUntilClose(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	first, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	second, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeMapForTest(first)
		t.Fatalf("second NewMap(): %v", err)
	}
	defer func() {
		if err := closeMapForTest(second); err != nil {
			t.Errorf("second Map Close(): %v", err)
		}
	}()

	firstID, err := first.ID()
	if err != nil {
		_ = closeMapForTest(first)
		t.Fatalf("ID(): %v", err)
	}
	secondID, err := second.ID()
	if err != nil {
		_ = closeMapForTest(first)
		t.Fatalf("second ID(): %v", err)
	}
	if firstID == 0 || firstID == secondID {
		_ = closeMapForTest(first)
		t.Fatalf("map IDs = %d and %d, want distinct nonzero IDs", firstID, secondID)
	}
	if repeated, err := first.ID(); err != nil || repeated != firstID {
		_ = closeMapForTest(first)
		t.Fatalf("repeated ID() = %d, %v, want %d, nil", repeated, err, firstID)
	}

	if err := closeMapForTest(first); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	if _, err := first.ID(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ID() after Close error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapCommandsAndStyleLoadingUseNativeABI(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.RequestRepaint(); err != nil {
		t.Fatalf("RequestRepaint(): %v", err)
	}
	if _, err := m.RequestStillImage(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("RequestStillImage() on continuous map error = %v, want ErrInvalidState", err)
	}
	if _, err := m.SetStyleJSON([]byte(minimalStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	if _, err := m.SetStyleURL("http://example.com/style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
}

func TestMapReportsLoadedStyleDocumentAndURL(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if document, err := awaitForTest(m.LoadedStyleJSON()); err != nil || len(document) != 0 {
		t.Fatalf("LoadedStyleJSON() before load = %q, %v, want \"\", nil", document, err)
	}
	if url, err := awaitForTest(m.StyleURL()); err != nil || url != "" {
		t.Fatalf("StyleURL() before load = %q, %v, want \"\", nil", url, err)
	}

	if _, err := m.SetStyleJSON([]byte(minimalStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	document, err := awaitForTest(m.LoadedStyleJSON())
	if err != nil {
		t.Fatalf("LoadedStyleJSON(): %v", err)
	}
	if string(document) != minimalStyleJSON {
		t.Fatalf("LoadedStyleJSON() = %q, want %q", document, minimalStyleJSON)
	}
	if url, err := awaitForTest(m.StyleURL()); err != nil || url != "" {
		t.Fatalf("StyleURL() after inline JSON = %q, %v, want \"\", nil", url, err)
	}

	// The URL is request state, recorded before the load can succeed, while the
	// document still reports the style that last parsed.
	const styleURL = "http://example.com/style.json"
	if _, err := m.SetStyleURL(styleURL); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	url, err := awaitForTest(m.StyleURL())
	if err != nil {
		t.Fatalf("StyleURL(): %v", err)
	}
	if url != styleURL {
		t.Fatalf("StyleURL() = %q, want %q", url, styleURL)
	}
	if document, err := awaitForTest(m.LoadedStyleJSON()); err != nil || string(document) != minimalStyleJSON {
		t.Fatalf("LoadedStyleJSON() after URL request = %q, %v, want the previously parsed document", document, err)
	}
}

// A committed command's terminal event carries the published map snapshot
// generation, so a snapshot at or past that generation shows the committed
// value.
func TestMapSnapshotObservesCommittedCommands(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	options := MapDebugTileBorders | MapDebugCollision
	completion, err := m.SetDebugOptions(options)
	committed := requireCommandCommitted(t, completion, err)
	snapshot, err := m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.Generation < committed {
		t.Fatalf("Snapshot() generation = %d, want at least the committed %d", snapshot.Generation, committed)
	}
	if !snapshot.DebugOptions.Has(options) {
		t.Fatalf("Snapshot() DebugOptions = %v, want bits %v", snapshot.DebugOptions, options)
	}

	completion, err = m.SetRenderingStatsViewEnabled(true)
	committed = requireCommandCommitted(t, completion, err)
	snapshot, err = m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.Generation < committed || !snapshot.RenderingStatsViewEnabled {
		t.Fatalf("Snapshot() = (generation %d, stats view %v), want at least %d and true",
			snapshot.Generation, snapshot.RenderingStatsViewEnabled, committed)
	}
	if _, err := m.DumpDebugLogs(); err != nil {
		t.Fatalf("DumpDebugLogs(): %v", err)
	}
}

// The tile, bounds, and free-camera snapshot fields round-trip through their
// set commands.
func TestMapSnapshotRoundTripsOptionCommands(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(256, 256, 1)))
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := closeMapForTest(m); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	completion, err := m.SetTileOptions(TileOptions{}.WithPrefetchZoomDelta(3))
	committed := requireCommandCommitted(t, completion, err)
	snapshot, err := m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.Generation < committed {
		t.Fatalf("Snapshot() generation = %d, want at least the committed %d", snapshot.Generation, committed)
	}
	if snapshot.Tile.PrefetchZoomDelta == nil || *snapshot.Tile.PrefetchZoomDelta != 3 {
		t.Fatalf("Snapshot() Tile = %#v, want prefetch zoom delta 3", snapshot.Tile)
	}

	completion, err = m.SetBounds(BoundOptions{}.WithMinZoom(2).WithMaxZoom(15))
	committed = requireCommandCommitted(t, completion, err)
	snapshot, err = m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.Generation < committed ||
		snapshot.Bounds.MinZoom == nil || *snapshot.Bounds.MinZoom != 2 ||
		snapshot.Bounds.MaxZoom == nil || *snapshot.Bounds.MaxZoom != 15 {
		t.Fatalf("Snapshot() Bounds = %#v, want zoom range 2 to 15", snapshot.Bounds)
	}

	position := Vec3{X: 0.25, Y: 0.25, Z: 0.1}
	completion, err = m.SetFreeCameraOptions(FreeCameraOptions{}.WithPosition(position))
	committed = requireCommandCommitted(t, completion, err)
	snapshot, err = m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.Generation < committed || snapshot.FreeCamera.Position == nil {
		t.Fatalf("Snapshot() FreeCamera = %#v, want a position", snapshot.FreeCamera)
	}
	got := *snapshot.FreeCamera.Position
	if math.Abs(got.X-position.X) > 1e-6 || math.Abs(got.Y-position.Y) > 1e-6 || math.Abs(got.Z-position.Z) > 1e-6 {
		t.Fatalf("Snapshot() FreeCamera position = %#v, want %#v", got, position)
	}
}

func TestMapResizeChangesTheExtentAndKeepsTheCreationScaleFactor(t *testing.T) {
	options := NewMapOptions(512, 256, 2)
	_, m := newRuntimeAndMap(t, &options)

	snapshot, err := m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	created := LogicalExtent{Width: 512, Height: 256, ScaleFactor: 2}
	if snapshot.LogicalExtent != created {
		t.Fatalf("MapSnapshot.LogicalExtent = %#v, want %#v", snapshot.LogicalExtent, created)
	}

	resized := LogicalExtent{Width: 640, Height: 480, ScaleFactor: 2}
	completion, err := m.Resize(resized)
	committed := requireCommandCommitted(t, completion, err)
	if snapshot, err = m.Snapshot(); err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.Generation < committed || snapshot.LogicalExtent != resized {
		t.Fatalf("MapSnapshot.LogicalExtent = %#v, want %#v", snapshot.LogicalExtent, resized)
	}

	// The scale factor is fixed at creation, because the renderer bakes its
	// pixel ratio into compiled shaders.
	rescale, err := m.Resize(LogicalExtent{Width: 640, Height: 480, ScaleFactor: 3})
	if rescale != nil || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Resize(changed scale factor) = (%v, %v), want nil and ErrInvalidArgument", rescale, err)
	}
	if snapshot, err = m.Snapshot(); err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.LogicalExtent != resized {
		t.Fatalf("rejected resize changed the extent to %#v", snapshot.LogicalExtent)
	}
}

func TestMapAcceptsFastPFORDecoding(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	options := NewMapOptions(256, 256, 1)
	if options.FastPFOREnabled {
		t.Fatalf("NewMapOptions().FastPFOREnabled = true; want false")
	}
	options.FastPFOREnabled = true
	m, err := awaitForTest(runtime.NewMapWithOptions(options))
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	if err := closeMapForTest(m); err != nil {
		t.Errorf("Map Close(): %v", err)
	}
	if err := closeRuntimeForTest(runtime); err != nil {
		t.Errorf("Runtime Close(): %v", err)
	}
}

func TestMapDebugOptionsRejectUnknownBitsBeforeSubmission(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	future, err := m.SetDebugOptions(MapDebugOptions(1 << 31))
	if future != nil || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetDebugOptions(unknown) = (%v, %v), want nil and ErrInvalidArgument", future, err)
	}
}

func TestMapStyleStringsRejectEmbeddedNUL(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleURL("http://example.com/\x00style.json"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleURL embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.SetStyleJSON([]byte("{\x00}")); err == nil {
		t.Fatal("SetStyleJSON embedded NUL error = nil")
	}
}

func TestMapCommandsCanMigrateAcrossGoroutines(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	result := make(chan struct {
		generation uint64
		err        error
	}, 1)
	go func() {
		completion, err := awaitForTest(m.RequestRepaint())
		result <- struct {
			generation uint64
			err        error
		}{generation: completion.Generation, err: err}
	}()
	got := <-result
	if got.err != nil {
		t.Fatalf("RequestRepaint() from another goroutine: %v", got.err)
	}
	if got.generation == 0 {
		t.Fatal("RequestRepaint() completed without a published generation")
	}
}
