//go:build mlntest

package maplibre

import (
	"errors"
	"strings"
	"testing"
)

// The first call that carries a native registration consumes it. When native
// code rejects that call, or the binding fails it first, the binding calls the
// registration's release once itself, and a later call that carries the
// registration returns StatusInvalidState without releasing it again.
func TestNativeRegistrationIsReleasedWhenTheCallIsNotAccepted(t *testing.T) {
	f := newFixture(t)
	wake := newNativeRecorderForTest(nil)
	lock := newNativeRecorderForTest(nil)
	options := RenderSessionAttachOptions{}
	options.FrameWake = wake.wake()
	options.QueueLock = lock.queueLock()
	// The descriptor names no device, which every build rejects.
	var rejected *Error
	if _, _, err := f.m.AttachVulkanOwnedTexture(VulkanOwnedTextureDescriptor{}, options); !errors.As(err, &rejected) || errors.Is(err, StatusInvalidState) {
		t.Fatalf("attach without a device = %v, want a native rejection", err)
	}
	if _, _, err := f.m.AttachVulkanOwnedTexture(VulkanOwnedTextureDescriptor{}, options); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("attach with consumed native registrations = %v, want StatusInvalidState", err)
	}
	for name, recorder := range map[string]*nativeRecorderForTest{"wake": wake, "queue lock": lock} {
		if releases := recorder.releases.Load(); releases != 1 {
			t.Errorf("the rejected %s was released %d times, want once", name, releases)
		}
	}

	mixed := newNativeRecorderForTest(nil)
	options = RenderSessionAttachOptions{}
	options.DriverWorkWake = mixed.wake()
	options.DriverWorkWake.Callback = func() {}
	// The binding fails the attach before it reaches native code.
	_, _, err := f.m.AttachVulkanOwnedTexture(VulkanOwnedTextureDescriptor{}, options)
	if !errors.As(err, &rejected) || rejected.Status != StatusInvalidArgument || !strings.Contains(rejected.Diagnostic, "native registration") {
		t.Fatalf("attach with a wake that is both native and a func = %v, want the binding's StatusInvalidArgument", err)
	}
	if releases := mixed.releases.Load(); releases != 1 {
		t.Fatalf("the wake that the binding rejected was released %d times, want once", releases)
	}
}
