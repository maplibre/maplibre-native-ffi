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
		func(out **nativeRuntime) int32 {
			createCalled = true
			return 0
		},
		func(runtime *nativeRuntime) (*handle.State[nativeRuntime], error) {
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

func TestRuntimeCreateRunOnceAndClose(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if err := runtime.RunOnce(); err != nil {
		t.Fatalf("RunOnce(): %v", err)
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
	if err := runtime.RunOnce(); err != nil {
		_ = runtime.Close()
		t.Fatalf("RunOnce() after failed close: %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close() on owner thread after failed close: %v", err)
	}
}

// drainLatchedWakes leaves the runtime idle with no latched signal, so a
// following park can only be released by the signal the test raises.
func drainLatchedWakes(t *testing.T, runtime *RuntimeHandle) {
	t.Helper()
	for i := 0; i < 100; i++ {
		signaled, err := runtime.Wait(0)
		if err != nil {
			t.Fatalf("Wait(): %v", err)
		}
		if !signaled {
			return
		}
		if err := runtime.RunOnce(); err != nil {
			t.Fatalf("RunOnce(): %v", err)
		}
		for {
			event, err := runtime.PollEvent()
			if err != nil {
				t.Fatalf("PollEvent(): %v", err)
			}
			if event == nil {
				break
			}
		}
	}
	t.Fatal("the runtime kept latching wakes while idle")
}

func TestRuntimeWaitWakesForNativeWorkAndForAWakeSource(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	mapHandle, err := runtime.NewMap()
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	drainLatchedWakes(t, runtime)

	// The style is malformed, so native reports the failure from its own
	// threads. What matters here is that the failure reaches a parked owner
	// thread at all.
	if err := mapHandle.SetStyleURL("unsupported://style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	loadingFailed := false
	for i := 0; i < 20; i++ {
		signaled, err := runtime.Wait(10 * time.Second)
		if err != nil {
			t.Fatalf("Wait(): %v", err)
		}
		if !signaled {
			t.Fatal("a park timed out while the style load was still pending")
		}
		if err := runtime.RunOnce(); err != nil {
			t.Fatalf("RunOnce(): %v", err)
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

	// A source used from another goroutine is what a host's submission path
	// holds, and the park it releases has no other work to end it.
	source, err := runtime.WakeSource()
	if err != nil {
		t.Fatalf("WakeSource(): %v", err)
	}
	drainLatchedWakes(t, runtime)
	signalErr := make(chan error, 1)
	go func() {
		time.Sleep(20 * time.Millisecond)
		signalErr <- source.Signal()
	}()
	signaled, err := runtime.Wait(10 * time.Second)
	if err != nil {
		t.Fatalf("Wait(): %v", err)
	}
	if !signaled {
		t.Fatal("the parked owner thread timed out instead of taking the signal")
	}
	if err := <-signalErr; err != nil {
		t.Fatalf("Signal(): %v", err)
	}

	// A wake source stays usable once its runtime is gone, so host teardown
	// ordering is free.
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

func TestRuntimeWaitConsumesOneLatchedSignal(t *testing.T) {
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
	drainLatchedWakes(t, runtime)

	if err := source.Signal(); err != nil {
		t.Fatalf("Signal(): %v", err)
	}
	if signaled, err := runtime.Wait(0); err != nil {
		t.Fatalf("Wait(): %v", err)
	} else if !signaled {
		t.Fatal("Wait() did not consume the latched signal")
	}
	// The latch is consumed, so an idle runtime reports the timeout instead.
	if signaled, err := runtime.Wait(0); err != nil {
		t.Fatalf("Wait(): %v", err)
	} else if signaled {
		t.Fatal("Wait() reported a second signal from one latch")
	}
}
