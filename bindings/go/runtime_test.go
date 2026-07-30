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

	runtime, err := NewRuntimeWithOptions(RuntimeOptions{CachePath: ":memory:"}.WithMaximumCacheSize(0))
	if err != nil {
		t.Fatalf("NewRuntimeWithOptions(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
}

func TestRuntimeOptionsRejectEmbeddedNUL(t *testing.T) {
	_, err := NewRuntimeWithOptions(RuntimeOptions{AssetPath: "asset\x00root"})
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

func TestRuntimeAmbientCacheOperationDiscard(t *testing.T) {
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
	if operation.ID() == 0 {
		t.Fatal("operation ID is zero")
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard(): %v", err)
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("second Discard(): %v", err)
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
	if event, err := runtime.PollEvent(); err != nil {
		t.Fatalf("PollEvent(): %v", err)
	} else if event != nil && event.PayloadSize > 0 && event.PayloadType == RuntimeEventPayloadNone {
		t.Fatalf("PollEvent() payload metadata inconsistent: %#v", event)
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

// quiesce pumps until the runtime is idle, so a park that follows is released by
// the signal the test raises.
func quiesce(t *testing.T, runtime *RuntimeHandle) {
	t.Helper()
	for i := 0; i < 100; i++ {
		if err := runtime.Pump(0); err != nil {
			t.Fatalf("Pump(): %v", err)
		}
		drained := false
		for {
			event, err := runtime.PollEvent()
			if err != nil {
				t.Fatalf("PollEvent(): %v", err)
			}
			if event == nil {
				break
			}
			drained = true
		}
		if !drained {
			return
		}
	}
	t.Fatal("the runtime kept producing events while idle")
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
	quiesce(t, runtime)

	// The style is malformed, so native reports the failure from its own threads
	// and the failure reaches the parked owner thread.
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
		for {
			event, err := runtime.PollEvent()
			if err != nil {
				t.Fatalf("PollEvent(): %v", err)
			}
			if event == nil {
				break
			}
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

	// A source signalled from another goroutine matches a host's submission path,
	// and the park it releases has no other work to end it.
	source, err := runtime.WakeSource()
	if err != nil {
		t.Fatalf("WakeSource(): %v", err)
	}
	quiesce(t, runtime)
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

	// A wake source stays usable after its runtime closes, so hosts tear the two
	// down in either order.
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
	quiesce(t, runtime)

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
