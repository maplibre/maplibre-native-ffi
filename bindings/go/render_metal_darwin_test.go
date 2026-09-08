//go:build darwin && cgo

package maplibre

import (
	"context"
	"errors"
	stdruntime "runtime"
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

// awaitRenderedMetalFrame demands frames until one of them renders.
func awaitRenderedMetalFrame(t *testing.T, session *RenderSessionHandle) {
	t.Helper()
	deadline := time.Now().Add(10 * time.Second)
	for time.Now().Before(deadline) {
		snapshot, err := session.Snapshot()
		if err != nil {
			t.Fatalf("Snapshot(): %v", err)
		}
		token := snapshot.LatestDemandToken + 1
		if err := session.RequestFrame(FrameDemand{Token: token}); err != nil {
			t.Fatalf("RequestFrame(): %v", err)
		}
		for time.Now().Before(deadline) {
			results, err := session.DrainFrameResults()
			if err != nil {
				t.Fatalf("DrainFrameResults(): %v", err)
			}
			if len(results) == 0 {
				time.Sleep(time.Millisecond)
				continue
			}
			for _, result := range results {
				if result.Token != token {
					continue
				}
				if result.Disposition == RenderResultRendered {
					return
				}
				break
			}
			break
		}
	}
	t.Fatal("Metal session did not render before the deadline")
}

// newMetalOwnedTextureSession attaches one Metal session-owned texture target
// with driver, and registers the teardown every path shares. It releases the
// caller's device reference as soon as the attachment is accepted, so the rest
// of a test runs on the reference the session retained for itself.
func newMetalOwnedTextureSession(
	t *testing.T, driver RenderDriver,
) (*RuntimeHandle, *MapHandle, *RenderSessionHandle, *Future[struct{}]) {
	t.Helper()
	if !SupportedRenderBackends().Has(RenderBackendMetal) {
		t.Skip("Metal is not the configured render backend")
	}
	device := testsupport.DefaultMetalDevice()
	if device == 0 {
		t.Fatal("the system default Metal device is unavailable")
	}

	runtime, err := NewRuntime()
	if err != nil {
		testsupport.ReleaseMetalDevice(device)
		t.Fatalf("NewRuntime(): %v", err)
	}
	mapFuture, err := runtime.NewMapWithOptions(NewMapOptions(32, 16, 1))
	if err != nil {
		testsupport.ReleaseMetalDevice(device)
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	m := awaitWithDeadline(t, mapFuture)

	options := NewRenderSessionAttachOptions()
	options.Driver = driver
	options.RequestedTextureRingDepth = 2
	session, attach, err := m.AttachMetalOwnedTexture(
		MetalOwnedTextureDescriptor{
			Extent:  RenderTargetExtent{Width: 32, Height: 16, ScaleFactor: 1},
			Context: MetalContextDescriptor{Device: NativePointer(device)},
		},
		options,
	)
	testsupport.ReleaseMetalDevice(device)
	if err != nil {
		t.Fatalf("AttachMetalOwnedTexture(): %v", err)
	}
	if session == nil || attach == nil {
		t.Fatal("AttachMetalOwnedTexture() did not publish both session and completion")
	}

	// Every handle below tolerates a second close, so this runs after the
	// explicit teardown a test performs and reclaims what a failure left behind.
	t.Cleanup(func() {
		_, _ = session.Abandon()
		_ = session.Close()
		_ = closeMapForTest(m)
		if teardown, err := runtime.Close(); err == nil {
			awaitWithDeadline(t, teardown)
		}
	})
	return runtime, m, session, attach
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
	runtime, m, session, attach := newMetalOwnedTextureSession(t, RenderDriverCoreWorker)
	awaitWithDeadline(t, attach)

	capabilities, err := session.Capabilities()
	if err != nil {
		t.Fatalf("Capabilities(): %v", err)
	}
	wantCapabilities := RenderSessionCapabilityFrameAcquisition |
		RenderSessionCapabilityReadback |
		RenderSessionCapabilityConsumerSync
	if capabilities.Driver != RenderDriverCoreWorker || capabilities.TextureRingDepth != 2 ||
		capabilities.Flags&wantCapabilities != wantCapabilities {
		t.Fatalf("Capabilities() = %#v, want core worker and a two-slot acquirable/readable ring", capabilities)
	}

	style, err := m.SetStyleJSON([]byte(minimalStyleJSON))
	if err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	awaitWithDeadline(t, style)
	barrier, err := runtime.Barrier()
	if err != nil {
		t.Fatalf("Runtime Barrier(): %v", err)
	}
	awaitWithDeadline(t, barrier)

	awaitRenderedMetalFrame(t, session)
	frame, err := session.AcquireFrame()
	if err != nil {
		t.Fatalf("AcquireFrame(): %v", err)
	}
	metadata, err := frame.MetalTexture()
	if err != nil {
		t.Fatalf("MetalTexture(): %v", err)
	}
	if metadata.Width != 32 || metadata.Height != 16 || metadata.Texture == 0 || metadata.Device == 0 {
		t.Fatalf("MetalTexture() = %#v, want a live 32x16 texture", metadata)
	}
	if err := frame.Release(GPUSync{Kind: GPUSyncCPUComplete}); err != nil {
		t.Fatalf("AcquiredFrame.Release(): %v", err)
	}
	// The lease is consumed, so the frame no longer reads its texture.
	if _, err := frame.MetalTexture(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("MetalTexture() after release error = %v, want ErrInvalidArgument", err)
	}

	readback, err := session.ReadPremultipliedRGBA8()
	if err != nil {
		t.Fatalf("ReadPremultipliedRGBA8(): %v", err)
	}
	image := awaitWithDeadline(t, readback)
	if image.Info.Width != 32 || image.Info.Height != 16 || len(image.Data) != int(image.Info.ByteLength) {
		t.Fatalf("ReadPremultipliedRGBA8() info = %#v, bytes = %d", image.Info, len(image.Data))
	}

	// Leave both old-size ring entries available. Resize must retire them so
	// acquisition cannot return an older 32x16 frame ahead of the new one.
	awaitRenderedMetalFrame(t, session)
	awaitRenderedMetalFrame(t, session)
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

	awaitRenderedMetalFrame(t, session)
	resizedFrame, err := session.AcquireFrame()
	if err != nil {
		t.Fatalf("AcquireFrame() after resize: %v", err)
	}
	resizedMetadata, err := resizedFrame.MetalTexture()
	if err != nil {
		t.Fatalf("MetalTexture() after resize: %v", err)
	}
	if resizedMetadata.Width != 48 || resizedMetadata.Height != 24 {
		t.Fatalf("MetalTexture() after resize = %#v, want 48x24", resizedMetadata)
	}
	if err := resizedFrame.Release(GPUSync{Kind: GPUSyncCPUComplete}); err != nil {
		t.Fatalf("resized AcquiredFrame.Release(): %v", err)
	}

	// A live session holds the map open, and the refusal leaves the handle
	// usable so the caller can detach and retry.
	if teardown, err := m.Close(); teardown != nil || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("MapHandle.Close() with an attached session = (%v, %v), want nil and ErrInvalidState", teardown, err)
	}
	if _, err := m.Snapshot(); err != nil {
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

func TestMetalCallerDriverServicesPublishedAttachingSessionDarwin(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	_, m, session, attach := newMetalOwnedTextureSession(t, RenderDriverCallerGraphicsThread)

	// Caller-driver initialization has not run yet, so the published session is
	// still attaching.
	select {
	case <-attach.Done():
		t.Fatal("caller-driver attach completed before the host serviced its published session")
	default:
	}
	snapshot, err := session.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot() while attaching: %v", err)
	}
	if snapshot.State != RenderSessionAttaching {
		t.Fatalf("Snapshot().State = %v, want RenderSessionAttaching", snapshot.State)
	}
	awaitCallerDriverCompletion(t, session, attach)

	capabilities, err := session.Capabilities()
	if err != nil {
		t.Fatalf("Capabilities(): %v", err)
	}
	if capabilities.Driver != RenderDriverCallerGraphicsThread {
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
