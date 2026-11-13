//go:build mlntest && cgo && (mln_metal || mln_vulkan || mln_egl || mln_wgl)

package maplibre

import (
	"errors"
	"testing"
)

// Native wakes reach the session unchanged: their C callbacks service the
// attachment and a frame. The attach consumes them, so a second attach that
// carries them fails before it reaches native code, and native code releases
// each one once, after the session can no longer wake it.
func TestNativeWakesServiceASessionAndAreReleasedOnce(t *testing.T) {
	r := newGraphicsFixture(t)
	frames := newNativeRecorderForTest(r.frames)
	driverWork := newNativeRecorderForTest(r.driverWork)
	options := r.options()
	options.FrameWake = frames.wake()
	options.DriverWorkWake = driverWork.wake()
	r.start(t, options)
	r.attach(t)
	r.renderFrame(t)

	if _, _, err := r.attachSession(options); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("attach with consumed native wakes = %v, want StatusInvalidState", err)
	}
	if frames.releases.Load() != 0 || driverWork.releases.Load() != 0 {
		t.Fatal("the binding released a wake that the session holds")
	}

	r.teardown(t)
	for _, wake := range []struct {
		name     string
		recorder *nativeRecorderForTest
	}{{"frame", frames}, {"driver-work", driverWork}} {
		receive(t, wake.recorder.released, "the release of the "+wake.name+" wake")
		if calls := wake.recorder.called.Load(); calls == 0 {
			t.Errorf("the %s wake was never called", wake.name)
		}
		if releases := wake.recorder.releases.Load(); releases != 1 {
			t.Errorf("the %s wake was released %d times, want once", wake.name, releases)
		}
	}
}

// A native queue lock reaches native code unchanged. The sessions of a Vulkan
// device take the lock around their queue calls, and native code releases it
// once after the device and its last session end. Any other backend refuses a
// device with a lock with StatusUnsupported, and the binding releases the lock
// itself.
func TestNativeQueueLockReachesTheDevice(t *testing.T) {
	r := newGraphicsFixture(t)
	lock := newNativeRecorderForTest(nil)
	if !queueLockSupported {
		if _, err := NewGPUDevice(&GPUDeviceOptions{QueueLock: lock.queueLock()}); !errors.Is(err, StatusUnsupported) {
			t.Fatalf("a device with a queue lock = %v, want StatusUnsupported", err)
		}
		if releases := lock.releases.Load(); releases != 1 {
			t.Fatalf("the rejected queue lock was released %d times, want once", releases)
		}
		return
	}
	r.wrapDevice(t, lock.queueLock())
	r.start(t, r.options())
	// The session keeps the device, and the device the lock, until the
	// session ends.
	if err := r.device.Release(); err != nil {
		t.Fatalf("GPUDeviceHandle.Release: %v", err)
	}
	r.attach(t)
	r.renderFrame(t)
	if releases := lock.releases.Load(); releases != 0 {
		t.Fatal("native code released the queue lock while a session holds it")
	}
	r.teardown(t)
	receive(t, lock.released, "the release of the queue lock")
	locks, unlocks := lock.called.Load(), lock.unlocked.Load()
	if locks == 0 || locks != unlocks {
		t.Errorf("the session locked the queue %d times and unlocked it %d times", locks, unlocks)
	}
	if releases := lock.releases.Load(); releases != 1 {
		t.Errorf("the queue lock was released %d times, want once", releases)
	}
}
