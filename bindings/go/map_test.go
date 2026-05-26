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

	if err := m.RequestRepaint(); err != nil {
		t.Fatalf("RequestRepaint(): %v", err)
	}
	if err := m.RequestStillImage(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("RequestStillImage() on continuous map error = %v, want ErrInvalidState", err)
	}
	if err := m.SetStyleJSON(minimalStyleJSON); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	if err := m.SetStyleURL("http://example.com/style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
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
	if err := m.SetDebugOptions(options); err != nil {
		t.Fatalf("SetDebugOptions(): %v", err)
	}
	got, err := m.DebugOptions()
	if err != nil {
		t.Fatalf("DebugOptions(): %v", err)
	}
	if !got.Has(options) {
		t.Fatalf("DebugOptions() = %v, want bits %v", got, options)
	}
	if err := m.SetRenderingStatsViewEnabled(true); err != nil {
		t.Fatalf("SetRenderingStatsViewEnabled(true): %v", err)
	}
	if got, err := m.RenderingStatsViewEnabled(); err != nil || !got {
		t.Fatalf("RenderingStatsViewEnabled() = %v, %v; want true, nil", got, err)
	}
	if _, err := m.IsFullyLoaded(); err != nil {
		t.Fatalf("IsFullyLoaded(): %v", err)
	}
	if err := m.DumpDebugLogs(); err != nil {
		t.Fatalf("DumpDebugLogs(): %v", err)
	}
}
func TestMapDebugOptionsRejectUnknownBits(t *testing.T) {
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

	if err := m.SetDebugOptions(MapDebugOptions(1 << 31)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetDebugOptions(unknown) error = %v, want ErrInvalidArgument", err)
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

	if err := m.SetStyleURL("http://example.com/\x00style.json"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleURL embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if err := m.SetStyleJSON("{\x00}"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleJSON embedded NUL error = %v, want ErrInvalidArgument", err)
	}
}
