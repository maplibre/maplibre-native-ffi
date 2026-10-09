//go:build cgo && (mln_metal || mln_vulkan || mln_egl)

package maplibre

import (
	"errors"
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

// frameView is what every backend's owned-texture frame view reports.
type frameView interface {
	Width() (uint32, error)
	Height() (uint32, error)
}

type renderFixture struct {
	*fixture
	session  *RenderSessionHandle
	attached *Future[struct{}]
	// frames and driverWork receive the session's frame and driver-work wakes.
	frames, driverWork chan struct{}
	token              uint64
}

// newRenderFixture starts attaching a session-owned 32x16 texture to a map,
// with the caller driver and a device or context from tests/graphics. The test
// goroutine is the graphics thread. It stays locked to its OS thread through
// the cleanups, which run on it last-first, so the session detaches there and
// an EGL context is never left current on a thread that Go reuses.
func newRenderFixture(t *testing.T) *renderFixture {
	t.Helper()
	stdruntime.LockOSThread()
	t.Cleanup(stdruntime.UnlockOSThread)
	graphics, err := testsupport.NewGraphics(graphicsBackend)
	if err != nil {
		t.Fatalf("testsupport.NewGraphics: %v", err)
	}
	t.Cleanup(graphics.Close)
	if graphicsBackend == testsupport.BackendEGL {
		if err := graphics.MakeCurrent(); err != nil {
			t.Fatalf("Graphics.MakeCurrent: %v", err)
		}
	}

	extent := RenderTargetExtent{Width: 32, Height: 16, ScaleFactor: 1}
	mapOptions := DefaultMapOptions()
	mapOptions.InitialExtent = LogicalExtent{Width: extent.Width, Height: extent.Height, ScaleFactor: extent.ScaleFactor}
	r := &renderFixture{
		fixture:    newFixtureWith(t, mapOptions),
		frames:     make(chan struct{}, 1),
		driverWork: make(chan struct{}, 1),
	}
	awaitCommitted(t, submitted(r.m.SetStyleJson([]byte(backgroundStyle))))
	options := DefaultRenderSessionAttachOptions()
	options.Driver = RenderDriverKindCallerGraphicsThread
	options.RequestedTextureRingDepth = 2
	options.FrameWake = Wake{Callback: func() { notify(r.frames) }}
	options.DriverWorkWake = Wake{Callback: func() { notify(r.driverWork) }}
	r.session, r.attached, err = attachOwnedTexture(r.m, graphics.Context, extent, options)
	if err != nil {
		t.Fatalf("owned texture attach: %v", err)
	}
	t.Cleanup(func() { r.teardown(t) })
	return r
}

// attach services the session until its attachment completes.
func (r *renderFixture) attach(t *testing.T) {
	t.Helper()
	r.service(t, r.attached.Done(), "the attachment")
	await(t, r.attached)
}

// teardown detaches the session on this thread, or abandons it when it cannot
// detach, and destroys it.
func (r *renderFixture) teardown(t *testing.T) {
	if detach, err := r.session.Detach(); err == nil {
		r.service(t, detach.Done(), "the detach")
	} else if _, err := r.session.Abandon(); err != nil {
		t.Errorf("Abandon: %v", err)
	}
	if err := r.session.Close(); err != nil {
		t.Errorf("session close: %v", err)
	}
}

// service runs the session's driver work until done closes, blocking on the
// driver-work wake between rounds.
func (r *renderFixture) service(t *testing.T, done <-chan struct{}, what string) {
	t.Helper()
	deadline := time.NewTimer(testTimeout())
	defer deadline.Stop()
	for {
		if _, err := r.session.ServiceDriverWork(0); err != nil {
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
		if err := r.session.RequestFrame(FrameDemand{Token: r.token}); err != nil {
			t.Fatalf("RequestFrame: %v", err)
		}
		for {
			if _, err := r.session.ServiceDriverWork(0); err != nil {
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
	batch, err := r.session.DrainFrameResults()
	if errors.Is(err, ErrNotReady) {
		return 0, false
	}
	if err != nil {
		t.Fatalf("DrainFrameResults: %v", err)
	}
	defer batch.Close()
	count, err := batch.Count()
	if err != nil {
		t.Fatal(err)
	}
	for index := range count {
		result, err := batch.Get(index)
		if err != nil {
			t.Fatal(err)
		}
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
	if snapshot, err := r.session.GetSnapshot(); err != nil || snapshot.State != RenderSessionStateAttaching {
		t.Fatalf("snapshot while attaching = %+v, %v; want RenderSessionStateAttaching", snapshot, err)
	}
	receive(t, r.driverWork, "the driver-work wake")
	r.attach(t)
	if capabilities, err := r.session.GetCapabilities(); err != nil || capabilities.Driver != RenderDriverKindCallerGraphicsThread {
		t.Fatalf("capabilities = %+v, %v; want the caller driver", capabilities, err)
	}
	r.renderFrame(t)
}

// The binding reads a rendered frame back as premultiplied RGBA8 pixels.
func TestOwnedTextureFrameReadsBackAsPixels(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	r.renderFrame(t)
	readback, err := r.session.TextureReadPremultipliedRgba8()
	if err != nil {
		t.Fatalf("TextureReadPremultipliedRgba8: %v", err)
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

// A frame's texture view lives only for its callback. While it lives, it
// refuses a read from another thread, and the frame refuses to close from
// one; once the callback returns, the view has expired and the frame closes.
func TestFrameViewIsScopedToItsCallback(t *testing.T) {
	r := newRenderFixture(t)
	r.attach(t)
	r.renderFrame(t)
	frame, err := r.session.AcquireFrame()
	if err != nil {
		t.Fatalf("AcquireFrame: %v", err)
	}
	var expired frameView
	err = withFrameView(frame, func(view frameView) error {
		expired = view
		width, widthErr := view.Width()
		height, heightErr := view.Height()
		if widthErr != nil || heightErr != nil || width != 32 || height != 16 {
			t.Errorf("view size = %dx%d (%v, %v), want 32x16", width, height, widthErr, heightErr)
		}
		// This goroutine holds its OS thread, so another one runs elsewhere.
		foreign := make(chan [2]error, 1)
		go func() {
			_, viewErr := view.Width()
			foreign <- [2]error{viewErr, frame.Close(GpuSync{Kind: GpuSyncKindCpuComplete})}
		}()
		result := <-foreign
		if !errors.Is(result[0], ErrInvalidState) {
			t.Errorf("view read from another thread = %v, want ErrInvalidState", result[0])
		}
		if !errors.Is(result[1], ErrInvalidState) || result[1].Error() != "AcquiredFrameHandle is in use" {
			t.Errorf("frame close during the view = %v, want ErrInvalidState for a frame in use", result[1])
		}
		return nil
	})
	if err != nil {
		t.Fatalf("frame view: %v", err)
	}
	if _, err := expired.Width(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("expired view read = %v, want ErrInvalidState", err)
	}
	if err := frame.Close(GpuSync{Kind: GpuSyncKindCpuComplete}); err != nil {
		t.Fatalf("frame close after the view: %v", err)
	}
	if err := withFrameView(frame, func(frameView) error { return nil }); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("view of a closed frame = %v, want ErrInvalidState", err)
	}
}
