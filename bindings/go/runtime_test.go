package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
	"time"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

func TestRuntimeCreateWithOptions(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntimeWithOptions(NewRuntimeOptions("", ":memory:"))
	if err != nil {
		t.Fatalf("NewRuntimeWithOptions(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
}

func TestRuntimeOptionsRejectEmbeddedNUL(t *testing.T) {
	_, err := NewRuntimeWithOptions(NewRuntimeOptions("asset\x00root", ""))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewRuntimeWithOptions embedded NUL error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeCreationRejectsABIMismatchBeforeNativeCreateOrHandleStore(t *testing.T) {
	createCalled := false
	storeCalled := false

	runtime, err := createRuntimeWithStateFactory(
		ExpectedCABIVersion+1,
		func(out *nativeRuntime) int32 {
			createCalled = true
			return 0
		},
		func(runtime nativeRuntime) (*handle.State[nativeRuntime], error) {
			storeCalled = true
			return nil, nil
		},
	)

	if runtime != nil {
		t.Fatalf("createRuntimeWithStateFactory() runtime = %v, want nil", runtime)
	}
	if !errors.Is(err, ErrABIVersionMismatch) {
		t.Fatalf("createRuntimeWithStateFactory() error = %v, want ErrABIVersionMismatch", err)
	}
	if createCalled {
		t.Fatal("runtime create hook was called after ABI mismatch")
	}
	if storeCalled {
		t.Fatal("runtime handle store hook was called after ABI mismatch")
	}
}

func TestRuntimeCreationDestroysNativeRuntimeWhenHandleStoreFails(t *testing.T) {
	const raw = nativeRuntime(42)
	var destroyed nativeRuntime
	restore := replaceRuntimeDestroyForTest(func(runtime nativeRuntime) int32 {
		destroyed = runtime
		return 0
	})
	defer restore()

	runtime, err := createRuntimeWithStateFactory(
		ExpectedCABIVersion,
		func(out *nativeRuntime) int32 {
			*out = raw
			return 0
		},
		func(nativeRuntime) (*handle.State[nativeRuntime], error) {
			return nil, errors.New("handle store failed")
		},
	)
	if runtime != nil || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("createRuntimeWithStateFactory() = (%v, %v), want (nil, ErrInvalidArgument)", runtime, err)
	}
	if destroyed != raw {
		t.Fatalf("destroyed runtime = %d, want %d", destroyed, raw)
	}
}

func TestRuntimeAmbientCacheOperationRelease(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	operation, err := runtime.StartAmbientCacheOperation(AmbientCacheOperationClear)
	if err != nil {
		t.Fatalf("StartAmbientCacheOperation(): %v", err)
	}
	operation.Release()
	operation.Release()
	if _, err := operation.Poll(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Poll() after Release() error = %v, want ErrInvalidArgument", err)
	}
	if err := operation.Cancel(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Cancel() after Release() error = %v, want ErrInvalidArgument", err)
	}
	if _, err := operation.Diagnostic(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Diagnostic() after Release() error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeAmbientCacheOperationRejectsUnknownOperation(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	_, err = runtime.StartAmbientCacheOperation(AmbientCacheOperation(999_999))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartAmbientCacheOperation(unknown) error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeCreatePumpAndClose(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if err := runtime.Pump(0); err != nil {
		t.Fatalf("Pump(): %v", err)
	}
	if batch, err := runtime.DrainEvents(0); err != nil {
		t.Fatalf("DrainEvents(): %v", err)
	} else if batch.RemainingCount != 0 {
		t.Fatalf("DrainEvents(0) left %d events queued", batch.RemainingCount)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("second Close(): %v", err)
	}
	if _, err := runtime.NewMap(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewMap() after Close error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeCloseWrongThreadLeavesHandleRetryable(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}

	errCh := make(chan error, 1)
	go func() {
		errCh <- runtime.Close()
	}()
	if err := <-errCh; !errors.Is(err, ErrWrongThread) {
		_ = runtime.Close()
		t.Fatalf("Close() from another thread error = %v, want ErrWrongThread", err)
	}
	if err := runtime.Pump(0); err != nil {
		_ = runtime.Close()
		t.Fatalf("Pump() after failed close: %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close() on owner thread after failed close: %v", err)
	}
}

func TestRuntimePumpWakesForNativeWorkAndForAWakeSource(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	mapHandle, err := runtime.NewMap()
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	drainAllRuntimeEvents(t, runtime)

	// Native reports the malformed style from its own threads; the failure has
	// to reach the parked owner thread.
	if err := mapHandle.SetStyleURL("unsupported://style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	loadingFailed := false
	loadStarted := time.Now()
	for i := 0; i < 20; i++ {
		if err := runtime.Pump(10 * time.Second); err != nil {
			t.Fatalf("Pump(): %v", err)
		}
		if time.Since(loadStarted) > 5*time.Second {
			t.Fatal("parks sat out their timeouts while the style load was pending")
		}
		batch, err := runtime.DrainEvents(0)
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		for _, event := range batch.Events {
			if event.Type == RuntimeEventMapLoadingFailed {
				loadingFailed = true
			}
		}
		if loadingFailed {
			break
		}
	}
	if !loadingFailed {
		t.Fatal("the parked owner thread never saw the loading failure")
	}

	// Nothing else ends the park below, so only the signal from the other
	// goroutine can release it.
	source, err := runtime.WakeSource()
	if err != nil {
		t.Fatalf("WakeSource(): %v", err)
	}
	drainAllRuntimeEvents(t, runtime)
	signalErr := make(chan error, 1)
	go func() {
		time.Sleep(20 * time.Millisecond)
		signalErr <- source.Signal()
	}()
	parkStarted := time.Now()
	if err := runtime.Pump(10 * time.Second); err != nil {
		t.Fatalf("Pump(): %v", err)
	}
	if time.Since(parkStarted) > 5*time.Second {
		t.Fatal("the parked owner thread timed out instead of taking the signal")
	}
	if err := <-signalErr; err != nil {
		t.Fatalf("Signal(): %v", err)
	}

	// A wake source stays usable after its runtime closes.
	if err := mapHandle.Close(); err != nil {
		t.Fatalf("map Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
	if err := source.Signal(); err != nil {
		t.Fatalf("Signal() after runtime close: %v", err)
	}
	source.Close()
	if err := source.Signal(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Signal() after source Close error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimePumpConsumesOneLatchedSignal(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Fatalf("Close(): %v", err)
		}
	}()
	source, err := runtime.WakeSource()
	if err != nil {
		t.Fatalf("WakeSource(): %v", err)
	}
	defer source.Close()
	drainAllRuntimeEvents(t, runtime)

	if err := source.Signal(); err != nil {
		t.Fatalf("Signal(): %v", err)
	}
	signalledStarted := time.Now()
	if err := runtime.Pump(10 * time.Second); err != nil {
		t.Fatalf("Pump(): %v", err)
	}
	if time.Since(signalledStarted) > 5*time.Second {
		t.Fatal("a pump waited even though the wake flag was set")
	}

	// The pump above cleared the wake flag, so this one waits its full timeout.
	idleStarted := time.Now()
	if err := runtime.Pump(200 * time.Millisecond); err != nil {
		t.Fatalf("Pump(): %v", err)
	}
	if time.Since(idleStarted) < 100*time.Millisecond {
		t.Fatal("the first pump left the wake flag set")
	}
}
