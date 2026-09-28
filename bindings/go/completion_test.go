package maplibre

import (
	"math"
	"runtime"
	"testing"
	"time"
)

func TestAbandonedFutureStillCommitsItsCommand(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := jumpForTest(m, CameraOptions{Center: pointerTo(LatLng{Latitude: 7, Longitude: 8}), Zoom: pointerTo(float64(5))}); err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	// The future above goes out of scope unawaited; the ordered query behind it
	// observes what the abandoned command committed.
	camera, err := awaitForTest(m.CameraQuery())
	if err != nil {
		t.Fatalf("CameraQuery completion: %v", err)
	}
	if camera.Camera.Center == nil ||
		math.Abs(camera.Camera.Center.Latitude-7) > 1e-9 ||
		math.Abs(camera.Camera.Center.Longitude-8) > 1e-9 {
		t.Fatalf("camera center after an abandoned command = %#v, want 7, 8", camera.Camera.Center)
	}
	if camera.Camera.Zoom == nil || math.Abs(*camera.Camera.Zoom-5) > 1e-9 {
		t.Fatalf("camera zoom after an abandoned command = %v, want 5", camera.Camera.Zoom)
	}
}

func waitForCompletionCollection(t *testing.T, collected <-chan struct{}) {
	t.Helper()
	deadline := time.Now().Add(5 * time.Second)
	for {
		runtime.GC()
		select {
		case <-collected:
			return
		default:
			if time.Now().After(deadline) {
				t.Fatal("owned completion was not collected")
			}
			runtime.Gosched()
		}
	}
}

func discardMapCreationForTest(t *testing.T, host *RuntimeHandle) <-chan struct{} {
	t.Helper()
	future, err := host.MapCreate(DefaultMapOptions())
	if err != nil {
		t.Fatal(err)
	}
	disposed := make(chan struct{})
	<-future.Done()
	future.state.mu.Lock()
	state := future.state.result.value.state
	original := state.dispose
	state.dispose = func(raw uint64) { original(raw); close(disposed) }
	future.state.mu.Unlock()
	return disposed
}

func TestUnclaimedMapCreationIsDisposed(t *testing.T) {
	host, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatal(err)
	}
	disposed := discardMapCreationForTest(t, host)
	if _, err := awaitForTest(host.Barrier()); err != nil {
		t.Fatal(err)
	}
	waitForCompletionCollection(t, disposed)
	if err := closeRuntimeForTest(host); err != nil {
		t.Fatalf("runtime retained unclaimed map: %v", err)
	}
}

func adoptCopiedCreationForTest(t *testing.T, host *RuntimeHandle) (*MapHandle, <-chan struct{}) {
	t.Helper()
	original, err := host.MapCreate(DefaultMapOptions())
	if err != nil {
		t.Fatal(err)
	}
	copied := *original
	original = nil
	runtime.GC()
	runtime.GC()
	first, err := copied.Await(t.Context())
	if err != nil {
		t.Fatalf("copied Future lost its value: %v", err)
	}
	second, err := copied.Await(t.Context())
	if err != nil || first != second {
		t.Fatalf("repeated Await = %p, %v; want %p", second, err, first)
	}
	collected := make(chan struct{})
	runtime.SetFinalizer(copied.state, nil)
	runtime.SetFinalizer(copied.state, func(state *futureState[*MapHandle]) {
		close(collected)
	})
	return first, collected
}

func TestCopiedCreationFutureAdoptsOnceAndPermitsRepeatedAwait(t *testing.T) {
	host, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatal(err)
	}
	owned, collected := adoptCopiedCreationForTest(t, host)
	waitForCompletionCollection(t, collected)
	if _, err := awaitForTest(owned.CameraQuery()); err != nil {
		t.Fatalf("adopted map was disposed: %v", err)
	}
	if err := closeMapForTest(owned); err != nil {
		t.Fatal(err)
	}
	if err := closeRuntimeForTest(host); err != nil {
		t.Fatal(err)
	}
}
