package main

import (
	"context"
	"errors"
	"fmt"
	"time"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

const glTexture2D = 0x0DE1

// ownedTextureRingDepth keeps a texture to compose while the map renders the
// next one.
const ownedTextureRingDepth = 2

// renderTarget is a render session that the SDL thread drives. The render loop
// requests frames, services driver work after its wake, and drains frame
// results after theirs.
type renderTarget interface {
	// RequestFrame asks for a frame. A forced frame renders even when the map
	// has no newer update.
	RequestFrame(force bool) error
	ServiceDriverWork() error
	// DrainFrameResults presents each rendered frame and reports whether one
	// reached the window.
	DrainFrameResults() (bool, error)
	// RetryAt reports when a paced retry is due, or false with none pending.
	RetryAt() (time.Time, bool)
	// RetryIfDue forces the pending paced retry once it is due.
	RetryIfDue() error
	// Resize keeps the session attached, either resizing the target in place
	// or handing the session a replacement.
	Resize(viewport) error
	Close() error
}

// callerDriver is the part of every target that drives its session: frame
// demand, driver service, result drains, and detachment. Its present hook
// shows the frame the session just rendered.
type callerDriver struct {
	session *maplibre.RenderSessionHandle
	wake    *loopWake
	present func() (bool, error)
	// released is set once a failed handover detached the session, so Close
	// only retires the handle.
	released bool
	// retryAt is when a paced retry is due, and zero with none pending.
	retryAt time.Time
}

// retryDelay is how long a frame that did not reach the window waits to
// retry, about one display refresh.
const retryDelay = 16 * time.Millisecond

// attach services the attachment until it completes.
func (driver *callerDriver) attach(session *maplibre.RenderSessionHandle, completion *maplibre.Future[struct{}], wakes loopWakes) error {
	driver.session = session
	driver.wake = wakes.driverWork
	if err := driver.await(completion); err != nil {
		_, abandonErr := driver.session.Abandon()
		return errors.Join(err, abandonErr, driver.session.Close())
	}
	return nil
}

// await services driver work on the SDL thread until future completes,
// sleeping until the driver-work wake or the completion arrives.
func (driver *callerDriver) await(future *maplibre.Future[struct{}]) error {
	for {
		if _, err := driver.session.ServiceDriverWork(0); err != nil {
			return err
		}
		select {
		case <-future.Done():
			_, err := future.Await(context.Background())
			return err
		case <-driver.wake.pending:
		}
	}
}

func (driver *callerDriver) RequestFrame(force bool) error {
	demand := maplibre.DefaultFrameDemand()
	demand.Flags = maplibre.FrameDemandFlagPresent
	if !force {
		demand.Flags |= maplibre.FrameDemandFlagIfNeeded
	}
	return driver.session.RequestFrame(demand)
}

func (driver *callerDriver) ServiceDriverWork() error {
	_, err := driver.session.ServiceDriverWork(0)
	return err
}

// DrainFrameResults drains every frame result and presents each rendered
// frame. A result that asks for another frame, as during a paint transition,
// requests it. A target that was not ready, or a frame that missed the window,
// consumed its map update, so a paced retry forces the next frame.
func (driver *callerDriver) DrainFrameResults() (bool, error) {
	batch, err := driver.session.DrainFrameResults()
	if errors.Is(err, maplibre.ErrNotReady) {
		return false, nil
	}
	if err != nil {
		return false, err
	}
	defer batch.Close()
	count, err := batch.Count()
	if err != nil {
		return false, err
	}
	presented, missed, needsRepaint := false, false, false
	for i := uint(0); i < count; i++ {
		result, err := batch.Get(i)
		if err != nil {
			return presented, err
		}
		switch result.Disposition {
		case maplibre.RenderResultRendered:
			shown, err := driver.present()
			if err != nil {
				return presented, err
			}
			presented = presented || shown
			missed = missed || !shown
		case maplibre.RenderResultTargetNotReady:
			missed = true
		default:
			continue
		}
		needsRepaint = needsRepaint || result.NeedsRepaint
	}
	if missed {
		driver.retryAt = time.Now().Add(retryDelay)
		return presented, nil
	}
	if needsRepaint {
		return presented, driver.RequestFrame(false)
	}
	return presented, nil
}

func (driver *callerDriver) RetryAt() (time.Time, bool) {
	return driver.retryAt, !driver.retryAt.IsZero()
}

func (driver *callerDriver) RetryIfDue() error {
	if driver.retryAt.IsZero() || time.Now().Before(driver.retryAt) {
		return nil
	}
	driver.retryAt = time.Time{}
	return driver.RequestFrame(true)
}

// Resize carries a new logical extent to the map through the attached
// session, which is the only extent authority while it stays attached.
func (driver *callerDriver) Resize(v viewport) error {
	_, err := driver.session.Resize(v.extent())
	return err
}

// handOver gives the session a replacement target and waits for it. Frames
// that ran before the handover drew into the outgoing target, so they are
// drained and presented while that target is still current. A failed
// handover leaves it unknown which target the session holds, so the session
// is detached before the caller releases either one.
func (driver *callerDriver) handOver(future *maplibre.Future[struct{}], err error) error {
	if err == nil {
		err = driver.await(future)
	}
	if err != nil {
		driver.released = true
		return errors.Join(err, driver.detach())
	}
	_, err = driver.DrainFrameResults()
	return err
}

// resizeMap carries the new logical extent to the map on the paths where the
// session cannot: a replaced target changes only the graphics resource.
func (driver *callerDriver) resizeMap(m *maplibre.MapHandle, v viewport) error {
	_, err := m.Resize(maplibre.LogicalExtent{
		Width:       v.logicalWidth,
		Height:      v.logicalHeight,
		ScaleFactor: v.scaleFactor,
	})
	return err
}

func (driver *callerDriver) detach() error {
	future, err := driver.session.Detach()
	if err == nil {
		err = driver.await(future)
	}
	if err != nil {
		_, abandonErr := driver.session.Abandon()
		err = errors.Join(err, abandonErr)
	}
	return err
}

// Close detaches on the SDL thread, then destroys the session.
func (driver *callerDriver) Close() error {
	if driver.session == nil {
		return nil
	}
	var err error
	if !driver.released {
		err = driver.detach()
	}
	err = errors.Join(err, driver.session.Close())
	driver.session = nil
	return err
}

func newOpenGLRenderTarget(context *openGLContext, v viewport, mode renderTargetMode, m *maplibre.MapHandle, wakes loopWakes) (renderTarget, error) {
	switch mode {
	case modeOwnedTexture:
		return newOpenGLOwnedTextureTarget(context, v, m, wakes)
	case modeBorrowedTexture:
		return newOpenGLBorrowedTextureTarget(context, v, m, wakes)
	case modeNativeSurface:
		return newOpenGLSurfaceTarget(context, v, m, wakes)
	default:
		return nil, fmt.Errorf("unsupported render target mode: %s", mode)
	}
}

type openGLOwnedTextureTarget struct {
	callerDriver
	compositor *openGLTextureCompositor
}

func newOpenGLOwnedTextureTarget(context *openGLContext, v viewport, m *maplibre.MapHandle, wakes loopWakes) (*openGLOwnedTextureTarget, error) {
	descriptor, err := context.descriptor(true)
	if err != nil {
		return nil, err
	}
	compositor, err := newOpenGLTextureCompositor(context, v)
	if err != nil {
		return nil, err
	}
	target := &openGLOwnedTextureTarget{compositor: compositor}
	target.present = target.drawFrame
	attachment, err := m.OpenglOwnedTextureAttach(
		maplibre.OpenglOwnedTextureDescriptor{Extent: v.extent(), Context: descriptor},
		wakes.attachOptions(ownedTextureRingDepth),
	)
	if err == nil {
		err = target.attach(attachment.Session, attachment.Completion, wakes)
	}
	if err != nil {
		return nil, errors.Join(fmt.Errorf("OpenGL texture attach failed: %w", err), compositor.Close())
	}
	return target, nil
}

func (target *openGLOwnedTextureTarget) Close() error {
	return errors.Join(target.callerDriver.Close(), target.compositor.Close())
}

func (target *openGLOwnedTextureTarget) Resize(v viewport) error {
	if err := target.compositor.Resize(v); err != nil {
		return err
	}
	return target.callerDriver.Resize(v)
}

// drawFrame composes the oldest rendered frame. An empty ring leaves the
// previously composed frame on screen, and nothing new reaches the window.
func (target *openGLOwnedTextureTarget) drawFrame() (bool, error) {
	frame, err := target.session.AcquireFrame()
	if errors.Is(err, maplibre.ErrNotReady) {
		return false, nil
	}
	if err != nil {
		return false, err
	}
	accessErr := requireCPUCompleteProducer(frame)
	if accessErr == nil {
		accessErr = frame.WithOpenglTexture(func(info maplibre.OpenglOwnedTextureFrameView) error {
			targetID, err := info.Target()
			if err != nil {
				return err
			}
			texture, err := info.Texture()
			if err != nil {
				return err
			}
			return target.compositor.DrawTexture(targetID, texture)
		})
	}
	// DrawTexture finishes its GPU reads, so the frame releases CPU-complete.
	releaseErr := frame.Close(maplibre.GpuSync{Kind: maplibre.GpuSyncKindCpuComplete})
	return accessErr == nil, errors.Join(accessErr, releaseErr)
}

func requireCPUCompleteProducer(frame *maplibre.AcquiredFrameHandle) error {
	return frame.WithProducerSync(func(producer maplibre.GpuSyncView) error {
		kind, err := producer.Kind()
		if err != nil {
			return err
		}
		if kind != maplibre.GpuSyncKindCpuComplete {
			return fmt.Errorf("go-map cannot wait on producer synchronization kind %d", kind)
		}
		return nil
	})
}

type openGLBorrowedTextureTarget struct {
	callerDriver
	compositor *openGLTextureCompositor
	mapRef     *maplibre.MapHandle
	texture    uint32
}

func newOpenGLBorrowedTextureTarget(context *openGLContext, v viewport, m *maplibre.MapHandle, wakes loopWakes) (*openGLBorrowedTextureTarget, error) {
	compositor, err := newOpenGLTextureCompositor(context, v)
	if err != nil {
		return nil, err
	}
	target := &openGLBorrowedTextureTarget{compositor: compositor, mapRef: m}
	target.present = target.drawTexture
	descriptor, err := target.describe(v)
	if err == nil {
		target.texture = descriptor.Texture
		var attachment maplibre.OpenglBorrowedTextureAttachResult
		attachment, err = m.OpenglBorrowedTextureAttach(descriptor, wakes.attachOptions(0))
		if err == nil {
			err = target.attach(attachment.Session, attachment.Completion, wakes)
		}
	}
	if err != nil {
		target.deleteTexture(target.texture)
		return nil, errors.Join(fmt.Errorf("OpenGL borrowed texture attach failed: %w", err), compositor.Close())
	}
	return target, nil
}

// describe allocates a texture at the viewport's size and describes it.
func (target *openGLBorrowedTextureTarget) describe(v viewport) (maplibre.OpenglBorrowedTextureDescriptor, error) {
	context, err := target.compositor.context.descriptor(true)
	if err != nil {
		return maplibre.OpenglBorrowedTextureDescriptor{}, err
	}
	texture, err := createBorrowedTexture(target.compositor.context, v)
	if err != nil {
		return maplibre.OpenglBorrowedTextureDescriptor{}, err
	}
	return maplibre.OpenglBorrowedTextureDescriptor{
		Extent:         v.extent(),
		PhysicalWidth:  v.physicalWidth,
		PhysicalHeight: v.physicalHeight,
		Context:        context,
		Texture:        texture,
		Target:         glTexture2D,
	}, nil
}

func (target *openGLBorrowedTextureTarget) deleteTexture(texture uint32) {
	if texture != 0 && target.compositor.context.MakeCurrent() == nil {
		glDeleteTexture(texture)
	}
}

func (target *openGLBorrowedTextureTarget) Close() error {
	err := target.callerDriver.Close()
	target.deleteTexture(target.texture)
	return errors.Join(err, target.compositor.Close())
}

// Resize hands the live session a texture at the new size; the session keeps
// its renderer across the handover.
func (target *openGLBorrowedTextureTarget) Resize(v viewport) error {
	if err := target.compositor.Resize(v); err != nil {
		return err
	}
	descriptor, err := target.describe(v)
	if err != nil {
		return err
	}
	// The outgoing texture stays current until the handover completes.
	if err := target.handOver(target.session.OpenglBorrowedTextureSetTarget(descriptor)); err != nil {
		target.deleteTexture(descriptor.Texture)
		return fmt.Errorf("OpenGL borrowed texture set target failed: %w", err)
	}
	target.deleteTexture(target.texture)
	target.texture = descriptor.Texture
	return target.resizeMap(target.mapRef, v)
}

func (target *openGLBorrowedTextureTarget) drawTexture() (bool, error) {
	return true, target.compositor.DrawTexture(glTexture2D, target.texture)
}

type openGLSurfaceTarget struct {
	callerDriver
	context *openGLContext
	mapRef  *maplibre.MapHandle
}

func newOpenGLSurfaceTarget(context *openGLContext, v viewport, m *maplibre.MapHandle, wakes loopWakes) (*openGLSurfaceTarget, error) {
	if err := context.refreshPlatformSurface(); err != nil {
		return nil, fmt.Errorf("OpenGL surface refresh failed: %w", err)
	}
	descriptor, err := context.descriptor(false)
	if err != nil {
		return nil, err
	}
	target := &openGLSurfaceTarget{context: context, mapRef: m}
	// The driver presented the frame already.
	target.present = func() (bool, error) { return true, nil }
	attachment, err := m.OpenglSurfaceAttach(
		maplibre.OpenglSurfaceDescriptor{Extent: v.extent(), Context: descriptor, Surface: context.surface()},
		wakes.attachOptions(0),
	)
	if err == nil {
		err = target.attach(attachment.Session, attachment.Completion, wakes)
	}
	if err != nil {
		return nil, fmt.Errorf("OpenGL surface attach failed: %w", err)
	}
	return target, nil
}

// Resize handles SDL returning a different EGL window surface for the resized
// window by handing the live session the replacement.
func (target *openGLSurfaceTarget) Resize(v viewport) error {
	outgoing := target.context.surface()
	if err := target.context.refreshPlatformSurface(); err != nil {
		// SDL may already have dropped the surface the session presents
		// through, so detach rather than leave it naming a dead surface.
		target.released = true
		return errors.Join(err, target.detach())
	}
	if target.context.surface() == outgoing {
		return target.callerDriver.Resize(v)
	}
	descriptor, err := target.context.descriptor(false)
	if err != nil {
		return err
	}
	if err := target.handOver(target.session.OpenglSurfaceSetTarget(maplibre.OpenglSurfaceDescriptor{
		Extent:  v.extent(),
		Context: descriptor,
		Surface: target.context.surface(),
	})); err != nil {
		return fmt.Errorf("OpenGL surface set target failed: %w", err)
	}
	return target.resizeMap(target.mapRef, v)
}
