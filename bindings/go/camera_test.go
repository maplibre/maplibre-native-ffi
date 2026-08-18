package maplibre

import (
	"context"
	"math"
	"sync"
	"testing"
	"time"
)

func TestCameraSnapshotAndOrderedQueryCopyValues(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(512, 512, 1)))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()

	command, err := m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 12, Longitude: 34}).WithZoom(4))
	if err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	if _, err := awaitForTest(command, err); err != nil {
		t.Fatalf("JumpTo completion: %v", err)
	}

	operation, err := m.QueryCamera()
	if err != nil {
		t.Fatalf("QueryCamera(): %v", err)
	}
	ordered, err := awaitForTest(operation, nil)
	if err != nil {
		t.Fatalf("Take(): %v", err)
	}
	if ordered.Camera.Center == nil ||
		math.Abs(ordered.Camera.Center.Latitude-12) > 1e-9 ||
		math.Abs(ordered.Camera.Center.Longitude-34) > 1e-9 {
		t.Fatalf("ordered camera center = %#v", ordered.Camera.Center)
	}

	published, err := m.CameraSnapshot()
	if err != nil {
		t.Fatalf("CameraSnapshot(): %v", err)
	}
	if published.Camera.Center == nil {
		t.Fatal("CameraSnapshot() omitted the center")
	}
	published.Camera.Center.Latitude = -80
	again, err := m.CameraSnapshot()
	if err != nil {
		t.Fatalf("CameraSnapshot(): %v", err)
	}
	if again.Camera.Center == nil || math.Abs(again.Camera.Center.Latitude-12) > 1e-9 {
		t.Fatalf("snapshot mutation affected native state: %#v", again.Camera.Center)
	}
}

func TestCameraCommandsAcceptConcurrentGoroutines(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(256, 256, 1)))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()

	const count = 16
	generations := make(chan uint64, count)
	errs := make(chan error, count)
	var wg sync.WaitGroup
	for i := 0; i < count; i++ {
		wg.Add(1)
		go func(zoom int) {
			defer wg.Done()
			future, err := m.JumpTo(CameraOptions{}.WithZoom(float64(zoom)))
			if err != nil {
				errs <- err
				return
			}
			completion, err := future.Await(context.Background())
			if err != nil {
				errs <- err
				return
			}
			generations <- completion.Generation
		}(i)
	}
	wg.Wait()
	close(generations)
	close(errs)
	for err := range errs {
		t.Errorf("JumpTo() from goroutine: %v", err)
	}
	seen := make(map[uint64]struct{}, count)
	for generation := range generations {
		if generation == 0 {
			t.Error("JumpTo() returned a zero generation")
		}
		if _, exists := seen[generation]; exists {
			t.Errorf("duplicate generation %d", generation)
		}
		seen[generation] = struct{}{}
	}
	if len(seen) != count {
		t.Fatalf("committed generations = %d, want %d", len(seen), count)
	}
}

func TestCameraOperationProgressesAutonomously(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMapWithOptions(NewMapOptions(64, 64, 1)))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()

	operation, err := m.QueryCamera()
	if err != nil {
		t.Fatalf("QueryCamera(): %v", err)
	}
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	if _, err := operation.Await(ctx); err != nil {
		t.Fatalf("Await(): %v", err)
	}
}
