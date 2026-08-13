package maplibre

import (
	"errors"
	"testing"
)

func TestRuntimeMapLifecycle(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}

	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	if err := runtime.Close(); !errors.Is(err, ErrInvalidState) {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("Close() with live map error = %v, want ErrInvalidState", err)
	} else {
		var bindingErr *Error
		if !errors.As(err, &bindingErr) || bindingErr.Diagnostic() != "RuntimeHandle has live child handles" {
			_ = m.Close()
			_ = runtime.Close()
			t.Fatalf("Close() with live map diagnostic = %v", err)
		}
	}
	if err := m.Close(); err != nil {
		_ = runtime.Close()
		t.Fatalf("Map Close(): %v", err)
	}
	if err := m.Close(); err != nil {
		_ = runtime.Close()
		t.Fatalf("second Map Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Runtime Close(): %v", err)
	}
}

func TestMapIDIdentifiesEachMapUntilClose(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	first, err := runtime.NewMap()
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	second, err := runtime.NewMap()
	if err != nil {
		_ = first.Close()
		t.Fatalf("second NewMap(): %v", err)
	}
	defer func() {
		if err := second.Close(); err != nil {
			t.Errorf("second Map Close(): %v", err)
		}
	}()

	firstID, err := first.ID()
	if err != nil {
		_ = first.Close()
		t.Fatalf("ID(): %v", err)
	}
	secondID, err := second.ID()
	if err != nil {
		_ = first.Close()
		t.Fatalf("second ID(): %v", err)
	}
	if firstID == 0 || firstID == secondID {
		_ = first.Close()
		t.Fatalf("map IDs = %d and %d, want distinct nonzero IDs", firstID, secondID)
	}
	if repeated, err := first.ID(); err != nil || repeated != firstID {
		_ = first.Close()
		t.Fatalf("repeated ID() = %d, %v, want %d, nil", repeated, err, firstID)
	}

	if err := first.Close(); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	if _, err := first.ID(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ID() after Close error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapCommandsAndStyleLoadingUseNativeABI(t *testing.T) {
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

	if document, err := m.LoadedStyleJSON(); err != nil || len(document) != 0 {
		t.Fatalf("LoadedStyleJSON() before load = %q, %v, want \"\", nil", document, err)
	}
	if url, err := m.StyleURL(); err != nil || url != "" {
		t.Fatalf("StyleURL() before load = %q, %v, want \"\", nil", url, err)
	}

	if _, err := m.SetStyleJSON([]byte(minimalStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	document, err := m.LoadedStyleJSON()
	if err != nil {
		t.Fatalf("LoadedStyleJSON(): %v", err)
	}
	if string(document) != minimalStyleJSON {
		t.Fatalf("LoadedStyleJSON() = %q, want %q", document, minimalStyleJSON)
	}
	if url, err := m.StyleURL(); err != nil || url != "" {
		t.Fatalf("StyleURL() after inline JSON = %q, %v, want \"\", nil", url, err)
	}

	// The URL is request state, recorded before the load can succeed, while the
	// document still reports the style that last parsed.
	const styleURL = "http://example.com/style.json"
	if _, err := m.SetStyleURL(styleURL); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	url, err := m.StyleURL()
	if err != nil {
		t.Fatalf("StyleURL(): %v", err)
	}
	if url != styleURL {
		t.Fatalf("StyleURL() = %q, want %q", url, styleURL)
	}
	if document, err := m.LoadedStyleJSON(); err != nil || string(document) != minimalStyleJSON {
		t.Fatalf("LoadedStyleJSON() after URL request = %q, %v, want the previously parsed document", document, err)
	}
}

func TestMapDebugAndStatusHelpersUseNativeABI(t *testing.T) {
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

	options := MapDebugTileBorders | MapDebugCollision
	if _, err := m.SetDebugOptions(options); err != nil {
		t.Fatalf("SetDebugOptions(): %v", err)
	}
	got, err := m.DebugOptions()
	if err != nil {
		t.Fatalf("DebugOptions(): %v", err)
	}
	if !got.Has(options) {
		t.Fatalf("DebugOptions() = %v, want bits %v", got, options)
	}
	if _, err := m.SetRenderingStatsViewEnabled(true); err != nil {
		t.Fatalf("SetRenderingStatsViewEnabled(true): %v", err)
	}
	if got, err := m.RenderingStatsViewEnabled(); err != nil || !got {
		t.Fatalf("RenderingStatsViewEnabled() = %v, %v; want true, nil", got, err)
	}
	if _, err := m.IsFullyLoaded(); err != nil {
		t.Fatalf("IsFullyLoaded(): %v", err)
	}
	if _, err := m.DumpDebugLogs(); err != nil {
		t.Fatalf("DumpDebugLogs(): %v", err)
	}
}

func TestMapSizeReportsCreationExtentAndPixelRatio(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 256, 2))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	width, height, scaleFactor, err := m.Size()
	if err != nil {
		t.Fatalf("Size(): %v", err)
	}
	if width != 512 || height != 256 || scaleFactor != 2 {
		t.Fatalf("Size() = %d, %d, %v; want 512, 256, 2", width, height, scaleFactor)
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
	m, err := runtime.NewMapWithOptions(options)
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	if err := m.Close(); err != nil {
		t.Errorf("Map Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Errorf("Runtime Close(): %v", err)
	}
}

func TestMapDebugOptionsRejectUnknownBitsBeforeSubmission(t *testing.T) {
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

	commandID, err := m.SetDebugOptions(MapDebugOptions(1 << 31))
	if commandID != 0 || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetDebugOptions(unknown) = (%d, %v), want 0 and ErrInvalidArgument", commandID, err)
	}
}

func TestMapStyleStringsRejectEmbeddedNUL(t *testing.T) {
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

	if _, err := m.SetStyleURL("http://example.com/\x00style.json"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleURL embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.SetStyleJSON([]byte("{\x00}")); err == nil {
		t.Fatal("SetStyleJSON embedded NUL error = nil")
	}
}

func TestMapCommandsCanMigrateAcrossGoroutines(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()

	result := make(chan struct {
		id  uint64
		err error
	}, 1)
	go func() {
		id, err := m.RequestRepaint()
		result <- struct {
			id  uint64
			err error
		}{id: id, err: err}
	}()
	got := <-result
	if got.err != nil {
		t.Fatalf("RequestRepaint() from another goroutine: %v", got.err)
	}
	if got.id == 0 {
		t.Fatal("RequestRepaint() returned a zero command ID")
	}
}
