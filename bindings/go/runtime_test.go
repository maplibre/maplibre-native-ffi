package maplibre

import (
	"context"
	"errors"
	"testing"
	"time"
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

func TestRuntimeBarrierProgressesAutonomously(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer runtime.Close()

	future, err := runtime.Barrier()
	if err != nil {
		t.Fatalf("Barrier(): %v", err)
	}
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	if _, err := future.Await(ctx); err != nil {
		t.Fatalf("Await(): %v", err)
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
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(128, 128, 1)))
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
