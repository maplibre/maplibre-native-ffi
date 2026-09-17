package maplibre

import (
	"math"
	"runtime"
	"runtime/cgo"
	"testing"
	"time"
)

type completionRetentionProbe struct {
	finalized chan struct{}
}

func TestCompletionBridgeRetainsValueAfterFutureIsDropped(t *testing.T) {
	finalized := make(chan struct{})
	probe := &completionRetentionProbe{finalized: finalized}
	runtime.SetFinalizer(probe, func(probe *completionRetentionProbe) {
		close(probe.finalized)
	})

	state := &futureState[struct{}]{ready: make(chan struct{})}
	bridge := &completionBridge[struct{}]{state: state}
	handle := cgo.NewHandle(completionReceiver(bridge))
	future := &Future[struct{}]{state: state}
	future.retain(probe)

	probe = nil
	future = nil
	state = nil
	bridge = nil
	// Two cycles plus a settle give a finalizer that a single cycle only queued
	// the chance to run, so the assertion below rejects a value the collector
	// would have freed.
	runtime.GC()
	runtime.GC()
	time.Sleep(50 * time.Millisecond)
	select {
	case <-finalized:
		t.Fatal("retained value was finalized while native still owned the completion bridge")
	default:
	}

	handle.Value().(completionReceiver).release()
	handle.Delete()

	deadline := time.Now().Add(5 * time.Second)
	for {
		runtime.GC()
		select {
		case <-finalized:
			return
		default:
			if time.Now().After(deadline) {
				t.Fatal("retained value was not released with the native completion bridge")
			}
			runtime.Gosched()
		}
	}
}

// A host that drops a future without awaiting it abandons only its own view of
// the work: the command still reaches native and commits.
func TestAbandonedFutureStillCommitsItsCommand(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 7, Longitude: 8}).WithZoom(5)); err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	// The future above goes out of scope unawaited; the ordered query behind it
	// observes what the abandoned command committed.
	camera, err := awaitForTest(m.QueryCamera())
	if err != nil {
		t.Fatalf("QueryCamera completion: %v", err)
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
