package maplibre

import (
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
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()

	commandID, err := m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 12, Longitude: 34}).WithZoom(4))
	if err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	if commandID == 0 {
		t.Fatal("JumpTo() returned a zero command ID")
	}

	operation, err := m.QueryCamera()
	if err != nil {
		t.Fatalf("QueryCamera(): %v", err)
	}
	defer operation.Release()
	if completed, err := operation.Wait(-1); err != nil || !completed {
		t.Fatalf("Wait() = %v, %v; want true, nil", completed, err)
	}
	ordered, err := operation.Take()
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
	m, err := runtime.NewMapWithOptions(NewMapOptions(256, 256, 1))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()

	const count = 16
	ids := make(chan uint64, count)
	errs := make(chan error, count)
	var wg sync.WaitGroup
	for i := 0; i < count; i++ {
		wg.Add(1)
		go func(zoom int) {
			defer wg.Done()
			id, err := m.JumpTo(CameraOptions{}.WithZoom(float64(zoom)))
			if err != nil {
				errs <- err
				return
			}
			ids <- id
		}(i)
	}
	wg.Wait()
	close(ids)
	close(errs)
	for err := range errs {
		t.Errorf("JumpTo() from goroutine: %v", err)
	}
	seen := make(map[uint64]struct{}, count)
	for id := range ids {
		if id == 0 {
			t.Error("JumpTo() returned a zero command ID")
		}
		if _, exists := seen[id]; exists {
			t.Errorf("duplicate command ID %d", id)
		}
		seen[id] = struct{}{}
	}
	if len(seen) != count {
		t.Fatalf("accepted command IDs = %d, want %d", len(seen), count)
	}
}

func TestCameraOperationProgressesAutonomously(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(64, 64, 1))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	defer runtime.Close()
	defer m.Close()

	operation, err := m.QueryCamera()
	if err != nil {
		t.Fatalf("QueryCamera(): %v", err)
	}
	defer operation.Release()
	completed, err := operation.Wait(2 * time.Second)
	if err != nil {
		t.Fatalf("Wait(): %v", err)
	}
	if !completed {
		t.Fatal("camera query did not progress autonomously")
	}
	if _, err := operation.Take(); err != nil {
		t.Fatalf("Take(): %v", err)
	}
}
