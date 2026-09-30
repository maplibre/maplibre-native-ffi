//go:build darwin && cgo

package maplibre

import (
	"context"
	"errors"
	stdruntime "runtime"
	"sync"
	"testing"
	"time"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"
)

func awaitWithDeadline[T any](t *testing.T, future *Future[T]) T {
	t.Helper()
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	value, err := future.Await(ctx)
	if err != nil {
		t.Fatalf("Await(): %v", err)
	}
	return value
}

// frameWakes holds the frame wake of each session that newOwnedTextureSession
// attached, which awaitRenderedFrame blocks on between drains.
var frameWakes sync.Map

// awaitRenderedFrame demands frames until one of them renders.
func awaitRenderedFrame(t *testing.T, session *RenderSessionHandle) {
	t.Helper()
	value, _ := frameWakes.Load(session)
	wake := value.(chan struct{})
	deadline := time.NewTimer(10 * time.Second)
	defer deadline.Stop()
	for {
		snapshot, err := session.GetSnapshot()
		if err != nil {
			t.Fatalf("Snapshot(): %v", err)
		}
		token := snapshot.LatestDemandToken + 1
		if err := session.RequestFrame(FrameDemand{Token: token}); err != nil {
			t.Fatalf("RequestFrame(): %v", err)
		}
	drain:
		for {
			results, err := drainFramesForTest(session)
			if err != nil {
				t.Fatalf("DrainFrameResults(): %v", err)
			}
			for _, result := range results {
				if result.Token != token {
					continue
				}
				if result.Disposition == RenderResultRendered {
					return
				}
				break drain
			}
			select {
			case <-wake:
			case <-deadline.C:
				t.Fatal("the session did not render before the deadline")
			}
		}
	}
}

// newOwnedTextureSession attaches one session-owned texture target with driver
// and a device or context from tests/graphics for the build's backend, and
// registers the teardown every path shares. An EGL session shares the
// fixture's context, which becomes current on the calling OS thread, so it
// takes the caller driver. A Metal fixture releases its device as soon as the
// attachment is accepted, so the rest of a test runs on the reference the
// session retained for itself.
func newOwnedTextureSession(
	t *testing.T, driver RenderDriverKind,
) (*RuntimeHandle, *MapHandle, *RenderSessionHandle, *Future[struct{}]) {
	t.Helper()
	backends, err := SupportedRenderBackendMask()
	if err != nil {
		t.Fatal(err)
	}
	backend := testsupport.BackendEGL
	switch {
	case backends.Has(RenderBackendFlagMetal):
		backend = testsupport.BackendMetal
	case backends.Has(RenderBackendFlagVulkan):
		backend = testsupport.BackendVulkan
	}
	// A caller driver runs on the thread its context is current on. The test
	// goroutine keeps that thread through its cleanups, which run on it last
	// first, so the context is released there and not left current on a thread
	// that Go reuses.
	callerDriven := driver == RenderDriverKindCallerGraphicsThread
	if callerDriven {
		stdruntime.LockOSThread()
		t.Cleanup(stdruntime.UnlockOSThread)
	}
	graphics, err := testsupport.NewGraphics(backend)
	if err != nil {
		t.Fatalf("testsupport.NewGraphics(): %v", err)
	}
	// The graphics object outlives the session, whose cleanup comes later.
	t.Cleanup(graphics.Close)
	if callerDriven && backend == testsupport.BackendEGL {
		if err := graphics.MakeCurrent(); err != nil {
			t.Fatalf("Graphics.MakeCurrent(): %v", err)
		}
	}

	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	mapFuture, err := runtime.MapCreate(mapOptionsForTest(32, 16, 1))
	if err != nil {
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	m := awaitWithDeadline(t, mapFuture)

	wake := make(chan struct{}, 1)
	options := DefaultRenderSessionAttachOptions()
	options.Driver = driver
	options.RequestedTextureRingDepth = 2
	options.FrameWake = Wake{Callback: func() {
		select {
		case wake <- struct{}{}:
		default:
		}
	}}
	extent := RenderTargetExtent{Width: 32, Height: 16, ScaleFactor: 1}
	context := graphics.Context
	var session *RenderSessionHandle
	var attach *Future[struct{}]
	switch backend {
	case testsupport.BackendMetal:
		attachment, attachErr := m.MetalOwnedTextureAttach(MetalOwnedTextureDescriptor{
			Extent:  extent,
			Context: MetalContextDescriptor{Device: context.MetalDevice},
		}, options)
		graphics.Close()
		session, attach, err = attachment.Session, attachment.Completion, attachErr
	case testsupport.BackendVulkan:
		attachment, attachErr := m.VulkanOwnedTextureAttach(VulkanOwnedTextureDescriptor{
			Extent: extent,
			Context: VulkanContextDescriptor{
				Instance:                 context.VulkanInstance,
				PhysicalDevice:           context.VulkanPhysicalDevice,
				Device:                   context.VulkanDevice,
				GraphicsQueue:            context.VulkanQueue,
				GraphicsQueueFamilyIndex: context.VulkanQueueFamilyIndex,
				GetInstanceProcAddr:      context.VulkanGetInstanceProcAddr,
				GetDeviceProcAddr:        context.VulkanGetDeviceProcAddr,
			},
		}, options)
		session, attach, err = attachment.Session, attachment.Completion, attachErr
	default:
		attachment, attachErr := m.OpenglOwnedTextureAttach(OpenglOwnedTextureDescriptor{
			Extent: extent,
			Context: OpenglContextDescriptor{
				Ownership: OpenglContextOwnershipShared,
				Data: OpenglContextDescriptorDataEglVariant{Value: EglContextDescriptor{
					Display:      context.EGLDisplay,
					Config:       context.EGLConfig,
					ShareContext: context.EGLContext,
				}},
			},
		}, options)
		session, attach, err = attachment.Session, attachment.Completion, attachErr
	}
	if err != nil {
		t.Fatalf("owned texture attach: %v", err)
	}
	if session == nil || attach == nil {
		t.Fatal("the owned texture attach did not publish both session and completion")
	}
	frameWakes.Store(session, wake)

	// Every handle below tolerates a second close, so this runs after the
	// explicit teardown a test performs and reclaims what a failure left behind.
	t.Cleanup(func() {
		frameWakes.Delete(session)
		_, _ = session.Abandon()
		_ = session.Close()
		_ = closeMapForTest(m)
		if teardown, err := runtime.Close(); err == nil {
			awaitWithDeadline(t, teardown)
		}
	})
	return runtime, m, session, attach
}

// requireMetal skips a test whose assertions read Metal frame views.
func requireMetal(t *testing.T) {
	t.Helper()
	backends, err := SupportedRenderBackendMask()
	if err != nil {
		t.Fatal(err)
	}
	if !backends.Has(RenderBackendFlagMetal) {
		t.Skip("the test reads Metal frame views, and Metal is not the configured render backend")
	}
}

// requireCoreWorkerFrames skips a test that acquires frames from a core-worker
// session on OpenGL, where a core-worker owned texture is readback-only.
func requireCoreWorkerFrames(t *testing.T) {
	t.Helper()
	backends, err := SupportedRenderBackendMask()
	if err != nil {
		t.Fatal(err)
	}
	if backends.Has(RenderBackendFlagOpengl) {
		t.Skip("an OpenGL core-worker owned texture is readback-only, so it has no frames to acquire")
	}
}

func awaitCallerDriverCompletion(t *testing.T, session *RenderSessionHandle, future *Future[struct{}]) {
	t.Helper()
	deadline := time.Now().Add(10 * time.Second)
	for time.Now().Before(deadline) {
		select {
		case <-future.Done():
			awaitWithDeadline(t, future)
			return
		default:
		}
		if _, err := session.ServiceDriverWork(64); err != nil {
			t.Fatalf("ServiceDriverWork(): %v", err)
		}
		stdruntime.Gosched()
	}
	t.Fatal("caller-driver operation did not complete before the deadline")
}

func TestMetalOwnedTextureCompletionLifecycleDarwin(t *testing.T) {
	requireMetal(t)
	runtime, m, session, attach := newOwnedTextureSession(t, RenderDriverKindCoreWorker)
	awaitWithDeadline(t, attach)

	capabilities, err := session.GetCapabilities()
	if err != nil {
		t.Fatalf("Capabilities(): %v", err)
	}
	wantCapabilities := RenderSessionCapabilityFlagFrameAcquisition |
		RenderSessionCapabilityFlagReadback |
		RenderSessionCapabilityFlagConsumerSync
	if capabilities.Driver != RenderDriverKindCoreWorker || capabilities.TextureRingDepth != 2 ||
		capabilities.Flags&wantCapabilities != wantCapabilities {
		t.Fatalf("Capabilities() = %#v, want core worker and a two-slot acquirable/readable ring", capabilities)
	}

	style, err := m.SetStyleJson([]byte(minimalStyleJSON))
	if err != nil {
		t.Fatalf("SetStyleJson(): %v", err)
	}
	awaitWithDeadline(t, style)
	barrier, err := runtime.Barrier()
	if err != nil {
		t.Fatalf("Runtime Barrier(): %v", err)
	}
	awaitWithDeadline(t, barrier)

	awaitRenderedFrame(t, session)
	frame, err := session.AcquireFrame()
	if err != nil {
		t.Fatalf("AcquireFrame(): %v", err)
	}
	var expired MetalOwnedTextureFrameView
	err = frame.WithMetalTexture(func(view MetalOwnedTextureFrameView) error {
		expired = view
		width, err := view.Width()
		if err != nil {
			return err
		}
		height, err := view.Height()
		if err != nil {
			return err
		}
		texture, err := view.UnsafeTexture()
		if err != nil {
			return err
		}
		device, err := view.UnsafeDevice()
		if err != nil {
			return err
		}
		if width != 32 || height != 16 || texture == 0 || device == 0 {
			t.Fatalf("texture = %dx%d, %x, %x", width, height, texture, device)
		}
		if err := frame.Close(GpuSync{Kind: GpuSyncKindCpuComplete}); !errors.Is(err, ErrBusy) {
			t.Fatalf("close during view: %v", err)
		}
		return nil
	})
	if err != nil {
		t.Fatal(err)
	}
	if _, err := expired.UnsafeTexture(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("expired texture view: %v", err)
	}

	if err := frame.Close(GpuSync{Kind: GpuSyncKindCpuComplete}); err != nil {
		t.Fatalf("AcquiredFrameHandle.Release(): %v", err)
	}
	// The lease is consumed, so the frame no longer reads its texture.
	if err := frame.WithMetalTexture(func(MetalOwnedTextureFrameView) error { t.Fatal("closed frame callback ran"); return nil }); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("WithMetalTexture() after release error = %v, want ErrInvalidArgument", err)
	}

	readback, err := session.TextureReadPremultipliedRgba8()
	if err != nil {
		t.Fatalf("TextureReadPremultipliedRgba8(): %v", err)
	}
	image := awaitWithDeadline(t, readback)
	if image.Info.Width != 32 || image.Info.Height != 16 || len(image.Data) != int(image.Info.ByteLength) {
		t.Fatalf("TextureReadPremultipliedRgba8() info = %#v, bytes = %d", image.Info, len(image.Data))
	}

	// Leave both old-size ring entries available. Resize must retire them so
	// acquisition cannot return an older 32x16 frame ahead of the new one.
	awaitRenderedFrame(t, session)
	awaitRenderedFrame(t, session)
	resized := RenderTargetExtent{Width: 48, Height: 24, ScaleFactor: 1}
	resize, err := session.Resize(resized)
	if err != nil {
		t.Fatalf("Resize(): %v", err)
	}
	awaitWithDeadline(t, resize)

	// The scale factor is fixed at attachment, so only the logical size moves.
	rescale, err := session.Resize(RenderTargetExtent{Width: 48, Height: 24, ScaleFactor: 2})
	if rescale != nil || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Resize(changed scale factor) = (%v, %v), want nil and ErrInvalidArgument", rescale, err)
	}

	awaitRenderedFrame(t, session)
	resizedFrame, err := session.AcquireFrame()
	if err != nil {
		t.Fatalf("AcquireFrame() after resize: %v", err)
	}
	err = resizedFrame.WithMetalTexture(func(view MetalOwnedTextureFrameView) error {
		width, err := view.Width()
		if err != nil {
			return err
		}
		height, err := view.Height()
		if err != nil {
			return err
		}
		if width != 48 || height != 24 {
			t.Fatalf("resized texture = %dx%d", width, height)
		}
		return nil
	})
	if err != nil {
		t.Fatal(err)
	}

	if err := resizedFrame.Close(GpuSync{Kind: GpuSyncKindCpuComplete}); err != nil {
		t.Fatalf("resized AcquiredFrameHandle.Release(): %v", err)
	}

	// A live session holds the map open, and the refusal leaves the handle
	// usable so the caller can detach and retry.
	if teardown, err := m.Close(); teardown != nil || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("MapHandle.Close() with an attached session = (%v, %v), want nil and ErrInvalidState", teardown, err)
	}
	if _, err := m.SnapshotGet(); err != nil {
		t.Fatalf("Snapshot() after a refused close: %v", err)
	}

	detach, err := session.Detach()
	if err != nil {
		t.Fatalf("Detach(): %v", err)
	}
	awaitWithDeadline(t, detach)
	if err := session.Close(); err != nil {
		t.Fatalf("RenderSessionHandle.Close(): %v", err)
	}
	if err := closeMapForTest(m); err != nil {
		t.Fatalf("MapHandle.Close(): %v", err)
	}
	teardown, err := runtime.Close()
	if err != nil {
		t.Fatalf("RuntimeHandle.Close(): %v", err)
	}
	awaitWithDeadline(t, teardown)
}

func TestCallerDriverServicesPublishedAttachingSessionDarwin(t *testing.T) {
	_, m, session, attach := newOwnedTextureSession(t, RenderDriverKindCallerGraphicsThread)

	// Caller-driver initialization has not run yet, so the published session is
	// still attaching.
	select {
	case <-attach.Done():
		t.Fatal("caller-driver attach completed before the host serviced its published session")
	default:
	}
	snapshot, err := session.GetSnapshot()
	if err != nil {
		t.Fatalf("Snapshot() while attaching: %v", err)
	}
	if snapshot.State != RenderSessionStateAttaching {
		t.Fatalf("Snapshot().State = %v, want RenderSessionStateAttaching", snapshot.State)
	}
	awaitCallerDriverCompletion(t, session, attach)

	capabilities, err := session.GetCapabilities()
	if err != nil {
		t.Fatalf("Capabilities(): %v", err)
	}
	if capabilities.Driver != RenderDriverKindCallerGraphicsThread {
		t.Fatalf("Capabilities().Driver = %v, want caller graphics thread", capabilities.Driver)
	}

	detach, err := session.Detach()
	if err != nil {
		t.Fatalf("Detach(): %v", err)
	}
	awaitCallerDriverCompletion(t, session, detach)
	if err := session.Close(); err != nil {
		t.Fatalf("RenderSessionHandle.Close(): %v", err)
	}
	if err := closeMapForTest(m); err != nil {
		t.Fatalf("MapHandle.Close(): %v", err)
	}
}
