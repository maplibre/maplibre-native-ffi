//go:build cgo && (mln_metal || mln_vulkan || mln_egl || mln_wgl)

package maplibre

import (
	"errors"
	"fmt"
	stdruntime "runtime"
	"testing"
	"time"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"
)

// The render tests build only with the backend tag that `mise run
// //bindings/go:test` passes for the preset, and render_<backend>_test.go
// supplies what differs between backends.

// backgroundStyle paints the whole target one opaque color, #d8f1ff.
const backgroundStyle = `{"version":8,"sources":{},"layers":[` +
	`{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}]}`

// frameView is what every backend's owned-texture frame view reports, and
// readTexture reads the texture that one lends, which the view's scope guards.
type frameView interface {
	Width() uint32
	Height() uint32
}

type renderFixture struct {
	*fixture
	graphics *testsupport.Graphics
	extent   LogicalExtent
	session  *RenderSessionHandle
	attached *Future[struct{}]
	// frames and driverWork receive the session's frame and driver-work wakes.
	frames, driverWork chan struct{}
	token              uint64
	closed             bool
}

// newRenderFixture starts attaching a session-owned 32x16 texture to a map,
// with the caller driver and a device or context from tests/graphics. The test
// goroutine is the graphics thread. It stays locked to its OS thread through
// the cleanups, which run on it last-first, so the session detaches there and
// an OpenGL context is never left current on a thread that Go reuses.
func newRenderFixture(t *testing.T) *renderFixture {
	t.Helper()
	r := newGraphicsFixture(t)
	r.start(t, r.options())
	return r
}

// newGraphicsFixture is newRenderFixture without the session.
func newGraphicsFixture(t *testing.T) *renderFixture {
	t.Helper()
	stdruntime.LockOSThread()
	t.Cleanup(stdruntime.UnlockOSThread)
	graphics, err := testsupport.NewGraphics(graphicsBackend)
	if err != nil {
		t.Fatalf("testsupport.NewGraphics: %v", err)
	}
	t.Cleanup(graphics.Close)
	if graphicsBackend == testsupport.BackendEgl || graphicsBackend == testsupport.BackendWgl {
		if err := graphics.MakeCurrent(); err != nil {
			t.Fatalf("Graphics.MakeCurrent: %v", err)
		}
	}

	extent := LogicalExtent{Width: 32, Height: 16, ScaleFactor: 1}
	mapOptions := testMapOptions()
	mapOptions.InitialExtent = extent
	r := &renderFixture{
		fixture:    newFixtureWith(t, mapOptions),
		graphics:   graphics,
		extent:     extent,
		frames:     make(chan struct{}, 1),
		driverWork: make(chan struct{}, 1),
	}
	await(t, submitted(r.m.SetStyleJson(backgroundStyle)))
	return r
}

// options returns the fixture's attach options, whose wakes notify frames and
// driverWork.
func (r *renderFixture) options() RenderSessionAttachOptions {
	return RenderSessionAttachOptions{
		Driver:                    RenderDriverKindCallerGraphicsThread,
		RequestedTextureRingDepth: 2,
		FrameWake:                 Wake{Callback: func() { notify(r.frames) }},
		DriverWorkWake:            Wake{Callback: func() { notify(r.driverWork) }},
	}
}

// attachTexture starts attaching the fixture's texture with options.
func (r *renderFixture) attachTexture(options RenderSessionAttachOptions) (*RenderSessionHandle, *Future[struct{}], error) {
	return attachOwnedTexture(r.m, r.graphics.Context, r.extent, options)
}

// start starts attaching the session, which the test's cleanup tears down.
func (r *renderFixture) start(t *testing.T, options RenderSessionAttachOptions) {
	t.Helper()
	var err error
	r.session, r.attached, err = r.attachTexture(options)
	if err != nil {
		t.Fatalf("owned texture attach: %v", err)
	}
	t.Cleanup(func() { r.teardown(t) })
}

// attach services the session until its attachment completes.
func (r *renderFixture) attach(t *testing.T) {
	t.Helper()
	r.service(t, r.attached.Done(), "the attachment")
	await(t, r.attached)
}

// teardown detaches the session on this thread when it can, then closes it
// and waits for its retirement, after which the cleanups release the map and
// destroy the graphics objects. A later teardown does nothing.
func (r *renderFixture) teardown(t *testing.T) {
	if r.closed {
		return
	}
	r.closed = true
	if detach, err := r.session.Detach(); err == nil {
		r.service(t, detach.Done(), "the detach")
	}
	await(t, submitted(r.session.Close()))
}

// service runs the session's driver work until done closes, blocking on the
// driver-work wake between rounds.
func (r *renderFixture) service(t *testing.T, done <-chan struct{}, what string) {
	t.Helper()
	deadline := time.NewTimer(testTimeout())
	defer deadline.Stop()
	for {
		if err := r.session.ServiceDriverWork(); err != nil {
			t.Fatalf("ServiceDriverWork: %v", err)
		}
		select {
		case <-done:
			return
		case <-r.driverWork:
		case <-deadline.C:
			t.Fatalf("timed out waiting for %s", what)
		}
	}
}

// renderFrame demands frames until one renders, servicing the session and
// draining its results whenever one of its wakes fires.
func (r *renderFixture) renderFrame(t *testing.T) {
	t.Helper()
	deadline := time.NewTimer(testTimeout())
	defer deadline.Stop()
	for {
		r.token++
		if err := r.session.RequestFrame(FrameDemand{Flags: FrameDemandFlagForce, Token: r.token}); err != nil {
			t.Fatalf("RequestFrame: %v", err)
		}
		for {
			if err := r.session.ServiceDriverWork(); err != nil {
				t.Fatalf("ServiceDriverWork: %v", err)
			}
			if disposition, done := r.result(t, r.token); done {
				if disposition == RenderResultRendered {
					return
				}
				break
			}
			select {
			case <-r.frames:
			case <-r.driverWork:
			case <-deadline.C:
				t.Fatal("timed out waiting for a rendered frame")
			}
		}
	}
}

// result drains the frame results and reports the disposition of token's.
func (r *renderFixture) result(t *testing.T, token uint64) (RenderResult, bool) {
	t.Helper()
	results, err := r.session.DrainFrameResults()
	if err != nil {
		t.Fatalf("DrainFrameResults: %v", err)
	}
	for _, result := range results {
		if result.Token == token {
			return result.Disposition, true
		}
	}
	return 0, false
}

// A caller-driven session stays attaching until the goroutine that owns the
// graphics thread services it. The driver-work wake announces the work, and
// servicing on that wake completes the attachment and renders frames.
func TestCallerDriverIsServicedFromAGoroutine(t *testing.T) {
	r := newRenderFixture(t)
	select {
	case <-r.attached.Done():
		t.Fatal("the attachment completed before the host serviced the session")
	default:
	}
	if snapshot, err := r.session.Snapshot(); err != nil || snapshot.State != RenderSessionStateAttaching {
		t.Fatalf("snapshot while attaching = %+v, %v; want RenderSessionStateAttaching", snapshot, err)
	}
	receive(t, r.driverWork, "the driver-work wake")
	r.attach(t)
	if capabilities, err := r.session.Capabilities(); err != nil || capabilities.Driver != RenderDriverKindCallerGraphicsThread {
		t.Fatalf("capabilities = %+v, %v; want the caller driver", capabilities, err)
	}
	r.renderFrame(t)
}

// Closing an attached session retires it without a detach. Once the close
// completes, the map releases and the host destroys the graphics objects.
func TestCloseRetiresAnAttachedSession(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	r.renderFrame(t)
	r.closed = true
	await(t, submitted(r.session.Close()))
	await(t, submitted(r.m.Close()))
	r.graphics.Close()
}

// Before any demand, no frame result is queued and no frame has rendered,
// which native reports as not ready and the binding returns as empty and nil.
func TestDrainBeforeAnyDemandReturnsNoBatch(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	if results, err := r.session.DrainFrameResults(); len(results) != 0 || err != nil {
		t.Fatalf("DrainFrameResults = %v, %v; want empty, nil", results, err)
	}
	if frame, err := r.session.AcquireFrame(); frame != nil || err != nil {
		t.Fatalf("AcquireFrame = %v, %v; want nil, nil", frame, err)
	}
}

// The binding reads a rendered frame back as premultiplied RGBA8 pixels.
func TestOwnedTextureFrameReadsBackAsPixels(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	r.renderFrame(t)
	readback, err := r.session.ReadTexture()
	if err != nil {
		t.Fatalf("ReadTexture: %v", err)
	}
	r.service(t, readback.Done(), "the readback")
	image := await(t, readback)
	if image.Info.Width != 32 || image.Info.Height != 16 || uint(len(image.Data)) != image.Info.ByteLength {
		t.Fatalf("readback info = %+v with %d bytes, want 32x16", image.Info, len(image.Data))
	}
	offset := int(image.Info.Height/2*image.Info.Stride + image.Info.Width/2*4)
	want := [4]byte{0xd8, 0xf1, 0xff, 0xff}
	for channel, value := range want {
		if difference := int(image.Data[offset+channel]) - int(value); difference < -1 || difference > 1 {
			t.Fatalf("center pixel = %v, want %v", image.Data[offset:offset+4], want)
		}
	}
}

// A frame view lends its texture only for its callback. While it lives, it
// refuses a read of the texture from another thread, and the frame refuses to
// close from one; once the callback returns, the view refuses the texture but
// still reads its plain values, and the frame closes.
func TestFrameViewIsScopedToItsCallback(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	r.renderFrame(t)
	frame, err := r.session.AcquireFrame()
	if err != nil || frame == nil {
		t.Fatalf("AcquireFrame = %v, %v; want a rendered frame", frame, err)
	}
	var expired frameView
	err = withFrameView(frame, func(view frameView) error {
		expired = view
		if width, height := view.Width(), view.Height(); width != 32 || height != 16 {
			t.Errorf("view size = %dx%d, want 32x16", width, height)
		}
		if err := readTexture(view); err != nil {
			t.Errorf("view texture read = %v, want nil", err)
		}
		// This goroutine holds its OS thread, so another one runs elsewhere.
		foreign := make(chan [2]error, 1)
		go func() {
			foreign <- [2]error{readTexture(view), frame.Close(nil)}
		}()
		result := <-foreign
		if !errors.Is(result[0], StatusInvalidState) {
			t.Errorf("view read from another thread = %v, want StatusInvalidState", result[0])
		}
		if !errors.Is(result[1], StatusBusy) {
			t.Errorf("frame close during the view = %v, want StatusBusy for a frame in use", result[1])
		}
		return nil
	})
	if err != nil {
		t.Fatalf("frame view: %v", err)
	}
	if err := readTexture(expired); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("expired view texture read = %v, want StatusInvalidState", err)
	}
	if width := expired.Width(); width != 32 {
		t.Fatalf("expired view width = %d, want 32", width)
	}
	if err := frame.Close(&GpuSync{Kind: GpuSyncKindCpuComplete}); err != nil {
		t.Fatalf("frame close after the view: %v", err)
	}
	if err := withFrameView(frame, func(frameView) error { return nil }); !errors.Is(err, StatusInvalidState) {
		t.Fatalf("view of a closed frame = %v, want StatusInvalidState", err)
	}
}

// A frame closes once: a second discard or a close after one does nothing.
func TestFrameDiscardsOnce(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	r.renderFrame(t)
	frame, err := r.session.AcquireFrame()
	if err != nil || frame == nil {
		t.Fatalf("AcquireFrame = %v, %v; want a rendered frame", frame, err)
	}
	checkHandleMembers(t, frame, "AcquiredFrameHandle", false)
	for range 2 {
		if err := frame.Discard(); err != nil {
			t.Fatalf("AcquiredFrameHandle.Discard: %v", err)
		}
	}
	if err := frame.Close(nil); err != nil {
		t.Fatalf("AcquiredFrameHandle.Close after Discard: %v, want a no-op", err)
	}
	checkHandleMembers(t, frame, "AcquiredFrameHandle", true)
}

func TestFrameViewSurvivesYielding(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	r.renderFrame(t)
	frame, err := r.session.AcquireFrame()
	if err != nil || frame == nil {
		t.Fatalf("AcquireFrame = %v, %v; want a rendered frame", frame, err)
	}
	defer func() {
		if err := frame.Close(nil); err != nil {
			t.Errorf("frame close: %v", err)
		}
	}()

	// The fixture pins its graphics goroutine; the view runs on a fresh goroutine.
	finished := make(chan error, 1)
	go func() {
		finished <- withFrameView(frame, func(view frameView) error {
			for range 1000 {
				stdruntime.Gosched()
				if err := readTexture(view); err != nil {
					return err
				}
				if width := view.Width(); width != 32 {
					return fmt.Errorf("view width = %d, want 32", width)
				}
			}
			return nil
		})
	}()
	if err := <-finished; err != nil {
		t.Fatalf("view after yielding: %v", err)
	}
}
