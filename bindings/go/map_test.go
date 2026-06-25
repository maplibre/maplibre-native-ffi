package maplibre

import (
	"errors"
	stdruntime "runtime"
	"sync/atomic"
	"testing"
	"time"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

func TestRuntimeMapLifecycle(t *testing.T) {
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

func TestMapCloseFailedDestroyLeavesHandleRetryable(t *testing.T) {
	state, err := handle.New(&nativeMap{}, "MapHandle")
	if err != nil {
		t.Fatal(err)
	}
	m := &MapHandle{state: state}

	oldDestroy := destroyMapHandle
	defer func() {
		destroyMapHandle = oldDestroy
	}()
	var calls atomic.Int32
	destroyMapHandle = func(*nativeMap) int32 {
		if calls.Add(1) == 1 {
			return -3
		}
		return 0
	}

	if err := m.Close(); !errors.Is(err, ErrWrongThread) {
		t.Fatalf("first Close() error = %v, want ErrWrongThread", err)
	}
	ptr, release, err := m.ptr()
	if err != nil {
		t.Fatalf("ptr() after failed Close(): %v", err)
	}
	if ptr == nil {
		t.Fatal("ptr() after failed Close() returned nil")
	}
	release()
	if err := m.Close(); err != nil {
		t.Fatalf("second Close(): %v", err)
	}
	if err := m.Close(); err != nil {
		t.Fatalf("third Close(): %v", err)
	}
	if got := calls.Load(); got != 2 {
		t.Fatalf("destroy calls = %d, want 2", got)
	}
}

func TestMapCloseWaitsForActiveBorrow(t *testing.T) {
	state, err := handle.New(&nativeMap{}, "MapHandle")
	if err != nil {
		t.Fatal(err)
	}
	m := &MapHandle{state: state}

	ptr, release, err := m.ptr()
	if err != nil {
		t.Fatalf("ptr(): %v", err)
	}
	if ptr == nil {
		t.Fatal("ptr() returned nil")
	}

	oldDestroy := destroyMapHandle
	defer func() {
		destroyMapHandle = oldDestroy
	}()
	destroyCalled := make(chan struct{})
	destroyMapHandle = func(*nativeMap) int32 {
		close(destroyCalled)
		return 0
	}

	closeDone := make(chan error, 1)
	go func() {
		closeDone <- m.Close()
	}()

	deadline := time.After(2 * time.Second)
	for {
		if _, extraRelease, err := m.ptr(); err == nil {
			extraRelease()
			select {
			case <-destroyCalled:
				t.Fatal("destroy ran before active borrow was released")
			case <-deadline:
				t.Fatal("Close did not enter releasing state")
			case <-time.After(100 * time.Microsecond):
				continue
			}
		}
		break
	}

	release()
	if err := <-closeDone; err != nil {
		t.Fatalf("Close(): %v", err)
	}
	select {
	case <-destroyCalled:
	default:
		t.Fatal("destroy did not run after borrow release")
	}
}

func TestMapCommandsAndStyleLoadingUseNativeABI(t *testing.T) {
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

	if err := m.SetDebugOptions(MapDebugOptions(1 << 31)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetDebugOptions(unknown) error = %v, want ErrInvalidArgument", err)
	}
}
func TestMapStyleStringsRejectEmbeddedNUL(t *testing.T) {
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

	if err := m.SetStyleURL("http://example.com/\x00style.json"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleURL embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if err := m.SetStyleJSON("{\x00}"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleJSON embedded NUL error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapWrongThreadReturnsWrongThread(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

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

	errCh := make(chan error, 1)
	go func() {
		errCh <- m.RequestRepaint()
	}()
	if err := <-errCh; !errors.Is(err, ErrWrongThread) {
		t.Fatalf("RequestRepaint() from another thread error = %v, want ErrWrongThread", err)
	}
	if err := m.RequestRepaint(); err != nil {
		t.Fatalf("RequestRepaint() on owner thread after wrong-thread call: %v", err)
	}
}
