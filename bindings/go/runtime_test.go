package maplibre

import (
	"context"
	"errors"
	"testing"
	"time"
)

func TestRuntimeCreateWithOptionsAndClose(t *testing.T) {
	runtime, err := RuntimeCreate(runtimeOptionsForTest("", ":memory:"))
	if err != nil {
		t.Fatalf("RuntimeCreate(): %v", err)
	}
	if err := closeRuntimeForTest(runtime); err != nil {
		t.Fatalf("Close(): %v", err)
	}
	if err := closeRuntimeForTest(runtime); err != nil {
		t.Fatalf("second Close(): %v", err)
	}
	if _, err := runtime.MapCreate(DefaultMapOptions()); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("NewMap() after Close error = %v, want ErrInvalidState", err)
	}
}

func TestRuntimeOptionsRejectEmbeddedNUL(t *testing.T) {
	_, err := RuntimeCreate(runtimeOptionsForTest("asset\x00root", ""))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewRuntimeWithOptions embedded NUL error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeBarrierProgressesAutonomously(t *testing.T) {
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

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
		runtime, err := RuntimeCreate(DefaultRuntimeOptions())
		if err != nil {
			errCh <- err
			return
		}
		runtimeCh <- runtime
	}()

	var runtime *RuntimeHandle
	select {
	case err := <-errCh:
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()) on goroutine: %v", err)
	case runtime = <-runtimeCh:
	}
	m, err := awaitForTest(runtime.MapCreate(mapOptionsForTest(128, 128, 1)))
	if err != nil {
		t.Fatalf("NewMap() after goroutine migration: %v", err)
	}
	closed := make(chan error, 1)
	go func() { closed <- closeMapForTest(m) }()
	if err := <-closed; err != nil {
		t.Fatalf("Map Close() on another goroutine: %v", err)
	}
	go func() { closed <- closeRuntimeForTest(runtime) }()
	if err := <-closed; err != nil {
		t.Fatalf("Runtime Close() on another goroutine: %v", err)
	}
}
