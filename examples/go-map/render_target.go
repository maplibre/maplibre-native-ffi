package main

import (
	"context"
	"errors"
	"fmt"
	"time"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

const glTexture2D = 0x0DE1

// textureRingDepth is the depth of a texture ring, session-owned or borrowed.
// The target holds the newest frame until a newer one arrives, and the session
// renders into the other slot meanwhile.
const textureRingDepth = 2

// retryDelay is how long the loop waits before it retries a frame that did not
// reach the window, about one display refresh. No map-update event prompts
// that retry.
const retryDelay = 16 * time.Millisecond

// renderTarget is a render session and the frames it shows. OpenGL on an EGL
// context requires the caller driver, so the SDL thread services the session
// after its driver-work wake. The loop demands a frame for each map update,
// and after the frame wake HandleWakes drains the results and shows the newest
// rendered frame.
type renderTarget interface {
	// RequestFrame demands a frame. A forced frame renders even when the map
	// has no newer update.
	RequestFrame(force bool) error
	// HandleWakes runs the session's work after its wakes: queued driver
	// work, a due retry, and the frame results. It reports whether a frame
	// reached the window.
	HandleWakes() (bool, error)
	// RetryAt reports when a paced retry is due, or false with none pending.
	RetryAt() (time.Time, bool)
	// Resize follows a resized window and keeps the session attached.
	Resize(viewport) error
	Close() error
}

// sessionWakes are the frame and driver-work wakes of one session.
type sessionWakes struct {
	frames     *loopWake
	driverWork *loopWake
}

// callerDriver is the part of every target that drives its session: frame
// demand, driver service, result drains, and detachment. Its present hook
// shows the frame the session just rendered.
type callerDriver struct {
	session *maplibre.RenderSessionHandle
	wakes   sessionWakes
	// presents is set for a native surface, the only target whose demands
	// ask the session to present.
	presents bool
	present  func() (bool, error)
	// releaseFrames releases the frames that the target holds, before the
	// session resizes or detaches.
	releaseFrames func() error
	nextToken     uint64
	// retryAt is when a paced retry is due, and zero with none pending.
	retryAt time.Time
}

func newCallerDriver(eventType sdl.EventType) callerDriver {
	return callerDriver{
		wakes: sessionWakes{
			frames:     newLoopWake(eventType),
			driverWork: newLoopWake(eventType),
		},
		present:       func() (bool, error) { return true, nil },
		releaseFrames: func() error { return nil },
	}
}

func (driver *callerDriver) attachOptions(ringDepth uint32) maplibre.RenderSessionAttachOptions {
	options := maplibre.DefaultRenderSessionAttachOptions()
	options.Driver = maplibre.RenderDriverKindCallerGraphicsThread
	options.RequestedTextureRingDepth = ringDepth
	options.FrameWake = driver.wakes.frames.wake()
	options.DriverWorkWake = driver.wakes.driverWork.wake()
	return options
}

// attach services the attachment until it completes. A failed attachment
// abandons its session.
func (driver *callerDriver) attach(session *maplibre.RenderSessionHandle, completion *maplibre.Future[struct{}]) error {
	driver.session = session
	if err := driver.await(completion); err != nil {
		_, abandonErr := driver.session.Abandon()
		err = errors.Join(err, abandonErr, driver.session.Close())
		driver.session = nil
		return err
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
		case <-driver.wakes.driverWork.pending:
		}
	}
}

func (driver *callerDriver) RequestFrame(force bool) error {
	driver.nextToken++
	demand := maplibre.DefaultFrameDemand()
	demand.Token = driver.nextToken
	if force {
		demand.Flags &^= maplibre.FrameDemandFlagIfNeeded
	}
	if driver.presents {
		demand.Flags |= maplibre.FrameDemandFlagPresent
	}
	return driver.session.RequestFrame(demand)
}

func (driver *callerDriver) HandleWakes() (bool, error) {
	if driver.wakes.driverWork.consume() {
		if _, err := driver.session.ServiceDriverWork(0); err != nil {
			return false, err
		}
	}
	if !driver.retryAt.IsZero() && !time.Now().Before(driver.retryAt) {
		driver.retryAt = time.Time{}
		if err := driver.RequestFrame(true); err != nil {
			return false, err
		}
	}
	if !driver.wakes.frames.consume() {
		return false, nil
	}
	return driver.drainFrameResults()
}

// drainFrameResults drains every frame result and shows the newest rendered
// frame. A rendered frame that asks for another, as during a paint
// transition, demands it. Neither a target that was not ready nor a frame that
// missed the window causes a map-update event, so a retry follows after about
// one refresh. The retry is forced, because a frame that missed the window
// consumed its update.
func (driver *callerDriver) drainFrameResults() (bool, error) {
	batch, err := driver.session.DrainFrameResults()
	if err != nil || batch == nil {
		return false, err
	}
	defer batch.Close()
	count, err := batch.Count()
	if err != nil {
		return false, err
	}
	rendered, retry, repaint := false, false, false
	for i := uint(0); i < count; i++ {
		result, err := batch.Get(i)
		if err != nil {
			return false, err
		}
		switch result.Disposition {
		case maplibre.RenderResultRendered:
			rendered = true
			repaint = repaint || result.NeedsRepaint
		case maplibre.RenderResultTargetNotReady:
			retry = true
		}
	}
	shown := false
	if rendered {
		if shown, err = driver.present(); err != nil {
			return false, err
		}
		retry = retry || !shown
	}
	switch {
	case retry:
		driver.retryAt = time.Now().Add(retryDelay)
	case repaint:
		err = driver.RequestFrame(false)
	}
	return shown, err
}

func (driver *callerDriver) RetryAt() (time.Time, bool) {
	return driver.retryAt, !driver.retryAt.IsZero()
}

// Resize carries a new logical extent to the map through the attached
// session. A session resize needs every frame released first.
func (driver *callerDriver) Resize(v viewport) error {
	if err := driver.releaseFrames(); err != nil {
		return err
	}
	future, err := driver.session.Resize(v.extent())
	reportFailure(future, err, "render session resize")
	return nil
}

// replaceTarget gives the session a replacement target and waits for it.
// Frames that ran before the replacement drew into the outgoing target, so
// they are drained and shown while that target is still current. A failed
// replacement leaves it unknown which target the session holds, so the
// session detaches before the caller releases either one.
func (driver *callerDriver) replaceTarget(future *maplibre.Future[struct{}], err error) error {
	if err == nil {
		err = driver.await(future)
	}
	if err != nil {
		return errors.Join(err, driver.Close())
	}
	_, err = driver.drainFrameResults()
	return err
}

// Close releases held frames, detaches, and destroys the session. A failed
// detach abandons the session, which ends its graphics calls at once.
func (driver *callerDriver) Close() error {
	if driver.session == nil {
		return nil
	}
	err := driver.releaseFrames()
	future, detachErr := driver.session.Detach()
	if detachErr == nil {
		detachErr = driver.await(future)
	}
	if detachErr != nil {
		fmt.Printf("render session detach failed, abandoning: %v\n", detachErr)
		abandoned, abandonErr := driver.session.Abandon()
		if abandoned.QuarantinedResourceCount > 0 {
			fmt.Printf("render session abandon kept %d resource groups until exit\n", abandoned.QuarantinedResourceCount)
		}
		err = errors.Join(err, abandonErr)
	}
	err = errors.Join(err, driver.session.Close())
	driver.session = nil
	return err
}

// resizeMap carries the new logical extent to the map when the session
// cannot: a replaced target changes only the graphics resource.
func resizeMap(m *maplibre.MapHandle, v viewport) {
	future, err := m.Resize(maplibre.LogicalExtent{
		Width:       v.logicalWidth,
		Height:      v.logicalHeight,
		ScaleFactor: v.scaleFactor,
	})
	reportFailure(future, err, "map resize")
}

// reportFailure logs a command that fails. Nothing waits on its completion.
func reportFailure[T any](future *maplibre.Future[T], err error, operation string) {
	if err != nil {
		fmt.Printf("%s failed: %v\n", operation, err)
		return
	}
	go func() {
		if _, err := future.Await(context.Background()); err != nil {
			fmt.Printf("%s failed: %v\n", operation, err)
		}
	}()
}

func newOpenGLRenderTarget(context *openGLContext, v viewport, mode renderTargetMode, m *maplibre.MapHandle, eventType sdl.EventType) (renderTarget, error) {
	driver := newCallerDriver(eventType)
	switch mode {
	case modeOwnedTexture:
		return newOpenGLOwnedTextureTarget(context, v, m, driver)
	case modeBorrowedTexture:
		return newOpenGLBorrowedTextureTarget(context, v, m, driver)
	case modeNativeSurface:
		return newOpenGLSurfaceTarget(context, v, m, driver)
	default:
		return nil, fmt.Errorf("unsupported render target mode: %s", mode)
	}
}

// openGLFrameTarget composes a texture ring's frames. After a rendered result,
// it acquires every ready frame, keeps the newest, and composes it. It holds
// that frame until a newer one replaces it, so the session renders into the
// ring's other slot meanwhile.
type openGLFrameTarget struct {
	callerDriver
	compositor *openGLTextureCompositor
	held       *maplibre.AcquiredFrameHandle
}

func newOpenGLFrameTarget(compositor *openGLTextureCompositor, driver callerDriver) *openGLFrameTarget {
	target := &openGLFrameTarget{callerDriver: driver, compositor: compositor}
	target.present = target.drawFrame
	target.releaseFrames = target.releaseHeld
	return target
}

// openGLOwnedTextureTarget composes a session-owned texture ring.
type openGLOwnedTextureTarget struct {
	*openGLFrameTarget
}

func newOpenGLOwnedTextureTarget(context *openGLContext, v viewport, m *maplibre.MapHandle, driver callerDriver) (*openGLOwnedTextureTarget, error) {
	descriptor, err := context.descriptor(true)
	if err != nil {
		return nil, err
	}
	compositor, err := newOpenGLTextureCompositor(context, v)
	if err != nil {
		return nil, err
	}
	target := &openGLOwnedTextureTarget{newOpenGLFrameTarget(compositor, driver)}
	attachment, err := m.OpenglOwnedTextureAttach(
		maplibre.OpenglOwnedTextureDescriptor{Extent: v.extent(), Context: descriptor},
		target.attachOptions(textureRingDepth),
	)
	if err == nil {
		err = target.attach(attachment.Session, attachment.Completion)
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

// drawFrame composes the newest ready frame and releases the older ones.
// DrawTexture finishes its GPU reads, so each frame releases CPU-complete.
func (target *openGLFrameTarget) drawFrame() (bool, error) {
	var newest *maplibre.AcquiredFrameHandle
	for {
		frame, err := target.session.AcquireFrame()
		if err != nil {
			return false, errors.Join(err, releaseFrame(newest))
		}
		if frame == nil {
			break
		}
		if err := releaseFrame(newest); err != nil {
			return false, errors.Join(err, releaseFrame(frame))
		}
		newest = frame
	}
	if newest == nil {
		return target.held != nil, nil
	}
	drawErr := requireCPUCompleteProducer(newest)
	if drawErr == nil {
		drawErr = newest.WithOpenglTexture(func(info maplibre.OpenglTextureFrameView) error {
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
	releaseErr := target.releaseHeld()
	target.held = newest
	return drawErr == nil, errors.Join(drawErr, releaseErr)
}

func (target *openGLFrameTarget) releaseHeld() error {
	err := releaseFrame(target.held)
	target.held = nil
	return err
}

func releaseFrame(frame *maplibre.AcquiredFrameHandle) error {
	if frame == nil {
		return nil
	}
	return frame.Close(maplibre.GpuSync{Kind: maplibre.GpuSyncKindCpuComplete})
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

// openGLBorrowedTextureTarget renders into a ring of textures that the example
// owns, and composes its frames as a session-owned ring's.
type openGLBorrowedTextureTarget struct {
	*openGLFrameTarget
	mapRef *maplibre.MapHandle
	ring   [textureRingDepth]uint32
}

func newOpenGLBorrowedTextureTarget(context *openGLContext, v viewport, m *maplibre.MapHandle, driver callerDriver) (*openGLBorrowedTextureTarget, error) {
	compositor, err := newOpenGLTextureCompositor(context, v)
	if err != nil {
		return nil, err
	}
	target := &openGLBorrowedTextureTarget{openGLFrameTarget: newOpenGLFrameTarget(compositor, driver), mapRef: m}
	ring, descriptor, err := target.describe(v)
	if err == nil {
		target.ring = ring
		var attachment maplibre.OpenglBorrowedTextureAttachResult
		attachment, err = m.OpenglBorrowedTextureAttach(descriptor, target.attachOptions(textureRingDepth))
		if err == nil {
			err = target.attach(attachment.Session, attachment.Completion)
		}
	}
	if err != nil {
		target.deleteRing(target.ring)
		return nil, errors.Join(fmt.Errorf("OpenGL borrowed texture attach failed: %w", err), compositor.Close())
	}
	return target, nil
}

// describe allocates a ring at the viewport's size and describes it.
func (target *openGLBorrowedTextureTarget) describe(v viewport) ([textureRingDepth]uint32, maplibre.OpenglBorrowedTextureDescriptor, error) {
	var ring [textureRingDepth]uint32
	context, err := target.compositor.context.descriptor(true)
	if err != nil {
		return ring, maplibre.OpenglBorrowedTextureDescriptor{}, err
	}
	textures := make([]maplibre.OpenglBorrowedTexture, 0, textureRingDepth)
	for slot := range ring {
		texture, err := createBorrowedTexture(target.compositor.context, v)
		if err != nil {
			target.deleteRing(ring)
			return [textureRingDepth]uint32{}, maplibre.OpenglBorrowedTextureDescriptor{}, err
		}
		ring[slot] = texture
		textures = append(textures, maplibre.OpenglBorrowedTexture{Texture: texture})
	}
	return ring, maplibre.OpenglBorrowedTextureDescriptor{
		Extent:         v.extent(),
		PhysicalWidth:  v.physicalWidth,
		PhysicalHeight: v.physicalHeight,
		Context:        context,
		Textures:       textures,
		Target:         glTexture2D,
	}, nil
}

func (target *openGLBorrowedTextureTarget) deleteRing(ring [textureRingDepth]uint32) {
	if target.compositor.context.MakeCurrent() != nil {
		return
	}
	for _, texture := range ring {
		if texture != 0 {
			glDeleteTexture(texture)
		}
	}
}

func (target *openGLBorrowedTextureTarget) Close() error {
	err := target.callerDriver.Close()
	target.deleteRing(target.ring)
	return errors.Join(err, target.compositor.Close())
}

// Resize replaces the ring, because its owner sets its size. A replacement is
// refused while the host holds a frame, so the held one goes first, and the
// window keeps what it last presented. The outgoing ring stays alive until the
// replacement completes. A failed replacement leaves it unknown which ring the
// session holds, so the session detaches before either ring is released.
func (target *openGLBorrowedTextureTarget) Resize(v viewport) error {
	if err := target.releaseHeld(); err != nil {
		return err
	}
	if err := target.compositor.Resize(v); err != nil {
		return err
	}
	ring, descriptor, err := target.describe(v)
	if err != nil {
		return err
	}
	future, err := target.session.OpenglBorrowedTextureSetTarget(descriptor)
	if err != nil {
		target.deleteRing(ring)
		return fmt.Errorf("OpenGL borrowed texture set target failed: %w", err)
	}
	// A target replacement leaves the map's extent unchanged.
	resizeMap(target.mapRef, v)
	if err := target.await(future); err != nil {
		err = errors.Join(err, target.callerDriver.Close())
		target.deleteRing(ring)
		return fmt.Errorf("OpenGL borrowed texture set target failed: %w", err)
	}
	target.deleteRing(target.ring)
	target.ring = ring
	// A replacement publishes no map update, and a frame rendered before it
	// can no longer be acquired, so the new ring needs a forced frame.
	return target.RequestFrame(true)
}

// openGLSurfaceTarget renders into the window's EGL surface, and the session
// presents each frame.
type openGLSurfaceTarget struct {
	callerDriver
	context *openGLContext
	mapRef  *maplibre.MapHandle
}

func newOpenGLSurfaceTarget(context *openGLContext, v viewport, m *maplibre.MapHandle, driver callerDriver) (*openGLSurfaceTarget, error) {
	if err := context.refreshPlatformSurface(); err != nil {
		return nil, fmt.Errorf("OpenGL surface refresh failed: %w", err)
	}
	descriptor, err := context.descriptor(false)
	if err != nil {
		return nil, err
	}
	target := &openGLSurfaceTarget{callerDriver: driver, context: context, mapRef: m}
	target.presents = true
	attachment, err := m.OpenglSurfaceAttach(
		maplibre.OpenglSurfaceDescriptor{Extent: v.extent(), Context: descriptor, Surface: context.surface()},
		target.attachOptions(0),
	)
	if err == nil {
		err = target.attach(attachment.Session, attachment.Completion)
	}
	if err != nil {
		return nil, fmt.Errorf("OpenGL surface attach failed: %w", err)
	}
	return target, nil
}

// Resize resizes the session, or, when SDL returns a different EGL window
// surface for the resized window, replaces the session's target with it.
func (target *openGLSurfaceTarget) Resize(v viewport) error {
	outgoing := target.context.surface()
	if err := target.context.refreshPlatformSurface(); err != nil {
		// SDL may already have dropped the surface the session presents
		// through, so detach rather than leave it naming a dead surface.
		return errors.Join(err, target.callerDriver.Close())
	}
	if target.context.surface() == outgoing {
		return target.callerDriver.Resize(v)
	}
	descriptor, err := target.context.descriptor(false)
	if err != nil {
		return err
	}
	if err := target.replaceTarget(target.session.OpenglSurfaceSetTarget(maplibre.OpenglSurfaceDescriptor{
		Extent:  v.extent(),
		Context: descriptor,
		Surface: target.context.surface(),
	})); err != nil {
		return fmt.Errorf("OpenGL surface set target failed: %w", err)
	}
	resizeMap(target.mapRef, v)
	return target.RequestFrame(true)
}
