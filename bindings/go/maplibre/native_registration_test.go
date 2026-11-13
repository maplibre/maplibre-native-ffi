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
	options := RenderSessionAttachOptions{}
	options.FrameWake = wake.wake()
	// A ring of no textures, which every build rejects.
	var empty RenderTarget = RenderTargetVulkanBorrowedTexturesVariant{}
	var rejected *Error
	if _, _, err := f.m.Attach(empty, nil, options); !errors.As(err, &rejected) || errors.Is(err, StatusInvalidState) {
		t.Fatalf("attach of an empty ring = %v, want a native rejection", err)
	}
	if _, _, err := f.m.Attach(empty, nil, options); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("attach with a consumed native wake = %v, want StatusInvalidState", err)
	}
	if releases := wake.releases.Load(); releases != 1 {
		t.Errorf("the rejected wake was released %d times, want once", releases)
	}

	lock := newNativeRecorderForTest(nil)
	deviceOptions := &GPUDeviceOptions{QueueLock: lock.queueLock()}
	backend := missingBackend(t)
	deviceOptions.Backend = &backend
	if _, err := NewGPUDevice(deviceOptions); !errors.As(err, &rejected) || errors.Is(err, StatusInvalidState) {
		t.Fatalf("a device on a backend this build lacks = %v, want a native rejection", err)
	}
	if _, err := NewGPUDevice(deviceOptions); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("a device with a consumed native queue lock = %v, want StatusInvalidState", err)
	}
	if releases := lock.releases.Load(); releases != 1 {
		t.Errorf("the rejected queue lock was released %d times, want once", releases)
	}

	mixed := newNativeRecorderForTest(nil)
	options = RenderSessionAttachOptions{}
	options.DriverWorkWake = mixed.wake()
	options.DriverWorkWake.Callback = func() {}
	// The binding fails the attach before it reaches native code.
	_, _, err := f.m.Attach(empty, nil, options)
	if !errors.As(err, &rejected) || rejected.Status != StatusInvalidArgument || !strings.Contains(rejected.Diagnostic, "native registration") {
		t.Fatalf("attach with a wake that is both native and a func = %v, want the binding's StatusInvalidArgument", err)
	}
	if releases := mixed.releases.Load(); releases != 1 {
		t.Fatalf("the wake that the binding rejected was released %d times, want once", releases)
	}
}

// missingBackend returns a render backend that this build lacks.
func missingBackend(t *testing.T) RenderBackend {
	t.Helper()
	supported := SupportedRenderBackendMask()
	for _, backend := range []struct {
		backend RenderBackend
		flag    RenderBackendFlag
	}{
		{RenderBackendMetal, RenderBackendFlagMetal},
		{RenderBackendVulkan, RenderBackendFlagVulkan},
		{RenderBackendOpenGL, RenderBackendFlagOpenGL},
		{RenderBackendWebGPU, RenderBackendFlagWebGPU},
	} {
		if supported&backend.flag == 0 {
			return backend.backend
		}
	}
	t.Fatal("this build has every render backend")
	return 0
}
