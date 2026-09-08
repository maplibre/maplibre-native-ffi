package maplibre

import (
	"context"
	"errors"
	"math"
	"sync"
	"testing"
	"time"
)

// transitionFinishedIDs returns the transition IDs the camera-transition
// finished events in events carry, in queue order.
func transitionFinishedIDs(t *testing.T, events []RuntimeEvent) []uint64 {
	t.Helper()
	var ids []uint64
	for _, event := range events {
		if event.Type != RuntimeEventMapCameraTransitionFinished {
			continue
		}
		payload, ok := event.Payload.(RuntimeEventCameraTransitionFinishedPayload)
		if !ok {
			t.Fatalf("camera transition event payload = %T, want a transition payload", event.Payload)
		}
		ids = append(ids, payload.TransitionID)
	}
	return ids
}

func countTransitionID(ids []uint64, want uint64) int {
	count := 0
	for _, id := range ids {
		if id == want {
			count++
		}
	}
	return count
}

func TestCameraSnapshotAndOrderedQueryCopyValues(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	command, err := m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 12, Longitude: 34}).WithZoom(4))
	if _, err := awaitForTest(command, err); err != nil {
		t.Fatalf("JumpTo completion: %v", err)
	}

	ordered, err := awaitForTest(m.QueryCamera())
	if err != nil {
		t.Fatalf("QueryCamera completion: %v", err)
	}
	if ordered.Generation == 0 {
		t.Fatal("QueryCamera() reported no snapshot generation")
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
	_, m := newRuntimeAndMap(t, nil)

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

// collectTransitionFinishedIDs drains until every wanted transition has
// reported its end, fences the runtime, and returns every transition ID it saw
// in queue order. The fence makes a duplicate event visible to the caller.
func collectTransitionFinishedIDs(t *testing.T, runtime *RuntimeHandle, wanted ...uint64) []uint64 {
	t.Helper()
	var ids []uint64
	for range make([]struct{}, 5000) {
		drained, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		ids = append(ids, transitionFinishedIDs(t, drained)...)
		complete := true
		for _, want := range wanted {
			if countTransitionID(ids, want) == 0 {
				complete = false
				break
			}
		}
		if complete {
			waitForRuntimeBarrier(t, runtime)
			return append(ids, transitionFinishedIDs(t, drainQueuedRuntimeEvents(t, runtime))...)
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("timed out waiting for camera transitions %v to finish (saw %v)", wanted, ids)
	return nil
}

// A transition a later one supersedes and a transition the host cancels each
// raise exactly one finished event carrying the transition ID the host chose.
// A map with no render session advances no transition on its own, so those are
// the two terminal outcomes it reaches.
func TestCameraTransitionsRaiseOneFinishedEventPerTransitionID(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	const (
		superseded uint64 = 11
		cancelled  uint64 = 12
	)

	start, err := m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 0, Longitude: 0}).WithZoom(2))
	requireCommandCommitted(t, start, err)

	eased, err := m.EaseTo(
		CameraOptions{}.WithCenter(LatLng{Latitude: 40, Longitude: 50}),
		&AnimationOptions{DurationMS: pointerTo(60_000.0), TransitionID: pointerTo(superseded)},
	)
	requireCommandCommitted(t, eased, err)

	flown, err := m.FlyTo(
		CameraOptions{}.WithCenter(LatLng{Latitude: 41, Longitude: 51}).WithZoom(6),
		&AnimationOptions{DurationMS: pointerTo(60_000.0), TransitionID: pointerTo(cancelled)},
	)
	requireCommandCommitted(t, flown, err)

	ids := collectTransitionFinishedIDs(t, runtime, superseded)
	if got := countTransitionID(ids, superseded); got != 1 {
		t.Fatalf("finished events for the superseded transition = %d, want 1 (saw %v)", got, ids)
	}
	if got := countTransitionID(ids, cancelled); got != 0 {
		t.Fatalf("the running transition reported its end %d times before it was cancelled", got)
	}

	cancel, err := m.CancelTransitions()
	requireCommandCommitted(t, cancel, err)

	ids = collectTransitionFinishedIDs(t, runtime, cancelled)
	if got := countTransitionID(ids, cancelled); got != 1 {
		t.Fatalf("finished events for the cancelled transition = %d, want 1 (saw %v)", got, ids)
	}

	// The cancelled transition left the camera where it had reached, short of
	// the target it was flying to.
	camera, err := awaitForTest(m.QueryCamera())
	if err != nil {
		t.Fatalf("QueryCamera completion: %v", err)
	}
	if camera.Camera.Center == nil {
		t.Fatal("QueryCamera() omitted the center")
	}
	if camera.Camera.Center.Latitude > 40 {
		t.Fatalf("cancelled transition reached %#v, want a center short of its target", camera.Camera.Center)
	}
}

// A gesture boundary sets and clears the published gesture flag.
func TestGesturePhasesDriveTheGestureFlag(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if snapshot, err := m.Snapshot(); err != nil || snapshot.GestureInProgress {
		t.Fatalf("MapSnapshot.GestureInProgress before any gesture = (%v, %v), want false", snapshot.GestureInProgress, err)
	}

	begin, err := m.UpdateCamera(CameraUpdate{
		Mode:         CameraUpdateModeJump,
		Camera:       CameraOptions{}.WithCenter(LatLng{Latitude: 1, Longitude: 2}),
		GesturePhase: GesturePhaseBegin,
	})
	requireCommandCommitted(t, begin, err)
	snapshot, err := m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if !snapshot.GestureInProgress {
		t.Fatal("MapSnapshot.GestureInProgress after a begin phase = false, want true")
	}

	update, err := m.UpdateCamera(CameraUpdate{
		Mode:         CameraUpdateModeJump,
		Camera:       CameraOptions{}.WithCenter(LatLng{Latitude: 3, Longitude: 4}),
		GesturePhase: GesturePhaseUpdate,
	})
	requireCommandCommitted(t, update, err)
	if snapshot, err = m.Snapshot(); err != nil || !snapshot.GestureInProgress {
		t.Fatalf("MapSnapshot.GestureInProgress during a gesture = (%v, %v), want true", snapshot.GestureInProgress, err)
	}

	end, err := m.UpdateCamera(CameraUpdate{
		Mode:         CameraUpdateModeJump,
		Camera:       CameraOptions{}.WithCenter(LatLng{Latitude: 5, Longitude: 6}),
		GesturePhase: GesturePhaseEnd,
	})
	requireCommandCommitted(t, end, err)
	if snapshot, err = m.Snapshot(); err != nil || snapshot.GestureInProgress {
		t.Fatalf("MapSnapshot.GestureInProgress after an end phase = (%v, %v), want false", snapshot.GestureInProgress, err)
	}
}

func TestApplyCameraDeltaMovesTheCameraInOrder(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	start, err := m.JumpTo(CameraOptions{}.WithCenter(LatLng{Latitude: 0, Longitude: 0}).WithZoom(4))
	requireCommandCommitted(t, start, err)

	scale, err := m.ApplyCameraDelta(CameraDelta{Kind: CameraDeltaKindScale, Amount: 2})
	requireCommandCommitted(t, scale, err)
	bearing, err := m.ApplyCameraDelta(CameraDelta{Kind: CameraDeltaKindBearing, Amount: 30})
	requireCommandCommitted(t, bearing, err)

	// The ordered query runs behind both deltas, so it observes them both.
	camera, err := awaitForTest(m.QueryCamera())
	if err != nil {
		t.Fatalf("QueryCamera completion: %v", err)
	}
	if camera.Camera.Zoom == nil || math.Abs(*camera.Camera.Zoom-5) > 1e-6 {
		t.Fatalf("zoom after a scale of two = %v, want 5", camera.Camera.Zoom)
	}
	if camera.Camera.Bearing == nil || math.Abs(*camera.Camera.Bearing) < 1e-6 {
		t.Fatalf("bearing after a rotate = %v, want a rotated camera", camera.Camera.Bearing)
	}
}

func TestCameraFittingQueriesRoundTripThroughBounds(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	bounds := LatLngBounds{
		Southwest: LatLng{Latitude: -10, Longitude: -20},
		Northeast: LatLng{Latitude: 10, Longitude: 20},
	}
	fitted, err := awaitForTest(m.CameraForLatLngBounds(bounds, nil))
	if err != nil {
		t.Fatalf("CameraForLatLngBounds completion: %v", err)
	}
	if fitted.Center == nil || fitted.Zoom == nil {
		t.Fatalf("CameraForLatLngBounds() = %#v, want a center and a zoom", fitted)
	}
	if math.Abs(fitted.Center.Latitude) > 1 || math.Abs(fitted.Center.Longitude) > 1 {
		t.Fatalf("fitted center = %#v, want the center of the bounds", fitted.Center)
	}

	corners := []LatLng{bounds.Southwest, bounds.Northeast}
	fittedCorners, err := awaitForTest(m.CameraForLatLngs(corners, nil))
	if err != nil {
		t.Fatalf("CameraForLatLngs completion: %v", err)
	}
	if fittedCorners.Zoom == nil || math.Abs(*fittedCorners.Zoom-*fitted.Zoom) > 1e-6 {
		t.Fatalf("CameraForLatLngs() zoom = %v, want the bounds zoom %v", fittedCorners.Zoom, *fitted.Zoom)
	}

	// The camera that fits the bounds covers them again when it is projected
	// back to a bounding box.
	covered, err := awaitForTest(m.LatLngBoundsForCamera(fitted))
	if err != nil {
		t.Fatalf("LatLngBoundsForCamera completion: %v", err)
	}
	if covered.Southwest.Latitude > bounds.Southwest.Latitude+1e-6 ||
		covered.Northeast.Latitude < bounds.Northeast.Latitude-1e-6 {
		t.Fatalf("LatLngBoundsForCamera() = %#v, want it to cover %#v", covered, bounds)
	}
}

func TestViewportAndProjectionModeRoundTripThroughTheSnapshot(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	viewport := ViewportOptions{}.
		WithNorthOrientation(NorthOrientationRight).
		WithConstrainMode(ConstrainModeNone).
		WithFrustumOffset(EdgeInsets{Top: 1, Left: 2, Bottom: 3, Right: 4})
	command, err := m.SetViewportOptions(viewport)
	committed := requireCommandCommitted(t, command, err)

	snapshot, err := m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.Generation < committed {
		t.Fatalf("MapSnapshot.Generation = %d, want at least %d", snapshot.Generation, committed)
	}
	if snapshot.Viewport.NorthOrientation == nil || *snapshot.Viewport.NorthOrientation != NorthOrientationRight {
		t.Fatalf("MapSnapshot.Viewport.NorthOrientation = %v, want right", snapshot.Viewport.NorthOrientation)
	}
	if snapshot.Viewport.ConstrainMode == nil || *snapshot.Viewport.ConstrainMode != ConstrainModeNone {
		t.Fatalf("MapSnapshot.Viewport.ConstrainMode = %v, want none", snapshot.Viewport.ConstrainMode)
	}

	projection := ProjectionModeOptions{}.WithAxonometric(true).WithSkew(0.25, 0.5)
	command, err = m.SetProjectionMode(projection)
	requireCommandCommitted(t, command, err)
	if snapshot, err = m.Snapshot(); err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	if snapshot.ProjectionMode.Axonometric == nil || !*snapshot.ProjectionMode.Axonometric {
		t.Fatalf("MapSnapshot.ProjectionMode.Axonometric = %v, want true", snapshot.ProjectionMode.Axonometric)
	}
	if snapshot.ProjectionMode.XSkew == nil || math.Abs(*snapshot.ProjectionMode.XSkew-0.25) > 1e-9 {
		t.Fatalf("MapSnapshot.ProjectionMode.XSkew = %v, want 0.25", snapshot.ProjectionMode.XSkew)
	}
	if snapshot.ProjectionMode.YSkew == nil || math.Abs(*snapshot.ProjectionMode.YSkew-0.5) > 1e-9 {
		t.Fatalf("MapSnapshot.ProjectionMode.YSkew = %v, want 0.5", snapshot.ProjectionMode.YSkew)
	}
}

// A static map renders nothing on its own, so a still-image request stays
// pending until the map close cancels it.
func TestMapCloseCancelsAPendingStillImage(t *testing.T) {
	options := NewMapOptions(64, 64, 1)
	options.Mode = MapModeStatic
	_, m := newRuntimeAndMap(t, &options)

	pending, err := m.RequestStillImage()
	if err != nil {
		t.Fatalf("RequestStillImage(): %v", err)
	}
	if err := closeMapForTest(m); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	if _, err := awaitForTest(pending, nil); !errors.Is(err, ErrCancelled) {
		t.Fatalf("pending still image completion error = %v, want ErrCancelled", err)
	}
}
