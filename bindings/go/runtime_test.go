package maplibre

import (
	"errors"
	"testing"
	"time"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

func TestRuntimeCreateWithOptionsAndClose(t *testing.T) {
	runtime, err := NewRuntimeWithOptions(NewRuntimeOptions("", ":memory:"))
	if err != nil {
		t.Fatalf("NewRuntimeWithOptions(): %v", err)
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
		func(*nativeRuntime) int32 {
			createCalled = true
			return 0
		},
		func(nativeRuntime) (*handle.State[nativeRuntime], error) {
			storeCalled = true
			return nil, nil
		},
	)
	if runtime != nil || !errors.Is(err, ErrABIVersionMismatch) {
		t.Fatalf("createRuntimeWithStateFactory() = (%v, %v)", runtime, err)
	}
	if createCalled || storeCalled {
		t.Fatal("ABI mismatch invoked a native create or stored a handle")
	}
}

func TestRuntimeBarrierProgressesAutonomously(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer runtime.Close()

	operation, err := runtime.Barrier()
	if err != nil {
		t.Fatalf("Barrier(): %v", err)
	}
	defer operation.Release()
	completed, err := operation.Wait(2 * time.Second)
	if err != nil {
		t.Fatalf("Wait(): %v", err)
	}
	if !completed {
		t.Fatal("runtime barrier did not progress autonomously")
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard(): %v", err)
	}
}

func TestRuntimeLifecycleMigratesAcrossGoroutines(t *testing.T) {
	runtimeCh := make(chan *RuntimeHandle, 1)
	errCh := make(chan error, 1)
	go func() {
		runtime, err := NewRuntime()
		if err != nil {
			errCh <- err
			return
		}
		runtimeCh <- runtime
	}()

	var runtime *RuntimeHandle
	select {
	case err := <-errCh:
		t.Fatalf("NewRuntime() on goroutine: %v", err)
	case runtime = <-runtimeCh:
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(128, 128, 1))
	if err != nil {
		t.Fatalf("NewMap() after goroutine migration: %v", err)
	}
	closed := make(chan error, 1)
	go func() { closed <- m.Close() }()
	if err := <-closed; err != nil {
		t.Fatalf("Map Close() on another goroutine: %v", err)
	}
	go func() { closed <- runtime.Close() }()
	if err := <-closed; err != nil {
		t.Fatalf("Runtime Close() on another goroutine: %v", err)
	}
}

func TestNotificationCallbackOnlySchedulesOwnedReadyDrain(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer runtime.Close()

	scheduled := make(chan struct{}, 1)
	if err := runtime.SetNotificationCallback(func() {
		select {
		case scheduled <- struct{}{}:
		default:
		}
	}); err != nil {
		t.Fatalf("SetNotificationCallback(): %v", err)
	}
	operation, err := runtime.Barrier()
	if err != nil {
		t.Fatalf("Barrier(): %v", err)
	}
	defer operation.Release()
	select {
	case <-scheduled:
	case <-time.After(2 * time.Second):
		t.Fatal("notification callback did not schedule the receiver")
	}
	if _, err := runtime.DrainReady(); err != nil {
		t.Fatalf("DrainReady(): %v", err)
	}
	if completed, err := operation.Wait(-1); err != nil || !completed {
		t.Fatalf("Wait() = %v, %v; want true, nil", completed, err)
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard(): %v", err)
	}
}
