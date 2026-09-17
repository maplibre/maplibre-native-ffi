package main

import (
	"context"
	"errors"
	"fmt"
	"runtime"
	"unsafe"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

const glTexture2D = 0x0DE1

type renderTarget interface {
	Close() error
	// Resize keeps the session attached, either resizing the target in place or
	// handing the session a replacement. It starts the ordered submission and
	// returns; PollPending drives it from the render loop.
	Resize(viewport) error
	// PollPending services caller-driver work, releases anything a completed
	// target replacement retired, and reports whether an ordered submission is
	// still outstanding. The render loop holds frame demand back while one is.
	PollPending() (bool, error)
	FinishFrame() error
	// DriveFrame submits demand and services the caller graphics-thread driver.
	DriveFrame() (frameOutcome, error)
}

// frameOutcome carries one frame demand's outcome: whether the session
// rendered the demand, and whether the map asked for another frame while it
// rendered this one.
type frameOutcome struct {
	rendered     bool
	needsRepaint bool
}

// callerDriver is an attached caller-graphics-thread session plus the
// monotonic demand tokens that tie each frame result back to the demand that
// produced it, and the one ordered submission that can be outstanding.
type callerDriver struct {
	session *maplibre.RenderSessionHandle
	// mapRef is the map this session renders. Target replacement changes only
	// the graphics resource, so those paths carry the extent to the map
	// directly.
	mapRef    *maplibre.MapHandle
	nextToken uint64
	pending   *maplibre.Future[struct{}]
}

// Attach services driver work until the attachment resolves.
func (driver *callerDriver) Attach(m *maplibre.MapHandle, session *maplibre.RenderSessionHandle, attached *maplibre.Future[struct{}]) error {
	driver.mapRef = m
	driver.session = session
	driver.pending = attached
	return driver.AwaitPending()
}

// ResizeMap carries the new logical extent to the map on the paths where the
// session cannot: a caller-owned texture the host sizes, and a replaced surface
// target. Both change only the graphics resource.
func (driver *callerDriver) ResizeMap(v viewport) error {
	_, err := driver.mapRef.Resize(maplibre.LogicalExtent{
		Width:       v.logicalWidth,
		Height:      v.logicalHeight,
		ScaleFactor: v.scaleFactor,
	})
	return err
}

// BeginPending takes ownership of the future one ordered submission returned,
// so the render loop can drive it instead of blocking the caller.
func (driver *callerDriver) BeginPending(future *maplibre.Future[struct{}]) {
	driver.pending = future
}

// PollPending services driver work and reports whether the outstanding
// submission is still pending.
func (driver *callerDriver) PollPending() (bool, error) {
	if driver.pending == nil {
		return false, nil
	}
	select {
	case <-driver.pending.Done():
		future := driver.pending
		driver.pending = nil
		_, err := future.Await(context.Background())
		return false, err
	default:
	}
	if _, err := driver.session.ServiceDriverWork(0); err != nil {
		driver.pending = nil
		return false, err
	}
	return true, nil
}

// AwaitPending services driver work until the outstanding submission
// completes. Startup and shutdown block here; the render loop polls instead.
func (driver *callerDriver) AwaitPending() error {
	for {
		pending, err := driver.PollPending()
		if err != nil || !pending {
			return err
		}
	}
}

// Resize carries a new logical extent to the map through the attached session,
// which is the only extent authority while it stays attached.
func (driver *callerDriver) Resize(v viewport) error {
	future, err := driver.session.Resize(v.extent())
	if err != nil {
		return err
	}
	driver.BeginPending(future)
	return nil
}

// Close detaches on the graphics thread, then destroys the session.
func (driver *callerDriver) Close() error {
	if driver.session == nil {
		return nil
	}
	session := driver.session
	// The outstanding submission owns the pending slot the detach needs, so it
	// finishes first.
	err := driver.AwaitPending()
	if err == nil {
		var future *maplibre.Future[struct{}]
		if future, err = session.Detach(); err == nil {
			driver.BeginPending(future)
			err = driver.AwaitPending()
		}
	}
	driver.session = nil
	driver.pending = nil
	if err != nil {
		_, abandonErr := session.Abandon()
		err = errors.Join(err, abandonErr)
	}
	return errors.Join(err, session.Close())
}

// RenderFrame submits one demand, services driver work, and reports the
// outcome of the result carrying this demand's token.
func (driver *callerDriver) RenderFrame(present bool) (frameOutcome, error) {
	driver.nextToken++
	token := driver.nextToken
	demand := maplibre.NewFrameDemand()
	if present {
		demand.Flags |= maplibre.FrameDemandPresent
	}
	demand.Token = token
	if err := driver.session.RequestFrame(demand); err != nil {
		return frameOutcome{}, err
	}
	if _, err := driver.session.ServiceDriverWork(0); err != nil {
		return frameOutcome{}, err
	}
	results, err := driver.session.DrainFrameResults()
	if err != nil {
		return frameOutcome{}, err
	}
	for _, result := range results {
		if result.Token != token {
			continue
		}
		return frameOutcome{
			rendered:     result.Disposition == maplibre.RenderResultRendered,
			needsRepaint: result.NeedsRepaint,
		}, nil
	}
	return frameOutcome{}, nil
}

// AcquireFrame leases the rendered frame, reporting nil while the ring holds
// none.
func (driver *callerDriver) AcquireFrame() (*maplibre.AcquiredFrame, error) {
	frame, err := driver.session.AcquireFrame()
	if errors.Is(err, maplibre.ErrNotReady) {
		return nil, nil
	}
	return frame, err
}

func requireCPUCompleteProducer(frame *maplibre.AcquiredFrame) error {
	producer, err := frame.ProducerSync()
	if err != nil {
		return err
	}
	if producer.Kind != maplibre.GPUSyncCPUComplete {
		return fmt.Errorf("go-map cannot wait on producer synchronization kind %d", producer.Kind)
	}
	return nil
}

type openGLContext struct {
	window   *sdl.Window
	context  sdl.GLContext
	platform openGLPlatform
}

type openGLPlatform struct {
	wgl *openGLWGLPlatform
	egl *openGLEGLPlatform
}

type openGLWGLPlatform struct {
	deviceContext unsafe.Pointer
}

type openGLEGLPlatform struct {
	display       unsafe.Pointer
	windowConfig  unsafe.Pointer
	pbufferConfig unsafe.Pointer
	surface       unsafe.Pointer
}

func usesEGL() bool {
	return runtime.GOOS == "linux" || runtime.GOOS == "darwin"
}

func newOpenGLContext(window *sdl.Window) (*openGLContext, error) {
	context, err := sdl.GL_CreateContext(window)
	if err != nil {
		return nil, fmt.Errorf("SDL_GL_CreateContext failed: %w", err)
	}
	ctx := &openGLContext{window: window, context: context}
	if err := ctx.MakeCurrent(); err != nil {
		_ = sdl.GL_DestroyContext(context)
		return nil, err
	}
	platform, err := platformOpenGLContext(window, context)
	if err != nil {
		_ = ctx.Close()
		return nil, err
	}
	ctx.platform = platform
	if err := glLoad(); err != nil {
		_ = ctx.Close()
		return nil, err
	}
	return ctx, nil
}

func platformOpenGLContext(window *sdl.Window, context sdl.GLContext) (openGLPlatform, error) {
	if runtime.GOOS == "windows" {
		properties, err := window.Properties()
		if err != nil {
			return openGLPlatform{}, fmt.Errorf("SDL_GetWindowProperties failed: %w", err)
		}
		deviceContext := properties.Pointer(sdl.PropWindowWin32HDCPointer, nil)
		if deviceContext == nil {
			return openGLPlatform{}, errors.New("SDL Win32 HDC property is not available")
		}
		return openGLPlatform{wgl: &openGLWGLPlatform{deviceContext: deviceContext}}, nil
	}

	display, err := sdl.EGL_GetCurrentDisplay()
	if err != nil {
		return openGLPlatform{}, fmt.Errorf("SDL_EGL_GetCurrentDisplay failed: %w", err)
	}
	config, err := sdl.EGL_GetCurrentConfig()
	if err != nil {
		return openGLPlatform{}, fmt.Errorf("SDL_EGL_GetCurrentConfig failed: %w", err)
	}
	surface, err := sdl.EGL_GetWindowSurface(window)
	if err != nil {
		return openGLPlatform{}, fmt.Errorf("SDL_EGL_GetWindowSurface failed: %w", err)
	}
	pbufferConfig := eglPbufferConfig(unsafe.Pointer(display), unsafe.Pointer(config))
	_ = context
	return openGLPlatform{egl: &openGLEGLPlatform{
		display:       unsafe.Pointer(display),
		windowConfig:  unsafe.Pointer(config),
		pbufferConfig: pbufferConfig,
		surface:       unsafe.Pointer(surface),
	}}, nil
}

func (ctx *openGLContext) MakeCurrent() error {
	if err := sdl.GL_MakeCurrent(ctx.window, ctx.context); err != nil {
		return fmt.Errorf("SDL_GL_MakeCurrent failed: %w", err)
	}
	return nil
}

func (ctx *openGLContext) SwapWindow() error {
	if err := sdl.GL_SwapWindow(ctx.window); err != nil {
		return fmt.Errorf("SDL_GL_SwapWindow failed: %w", err)
	}
	return nil
}

func (ctx *openGLContext) Close() error {
	_ = sdl.GL_MakeCurrent(ctx.window, nil)
	if ctx.context == nil {
		return nil
	}
	err := sdl.GL_DestroyContext(ctx.context)
	ctx.context = nil
	return err
}

func (ctx *openGLContext) descriptor(texture bool) (maplibre.OpenGLContextDescriptor, error) {
	if ctx.platform.wgl != nil {
		return maplibre.OpenGLContextDescriptor{WGL: &maplibre.WGLContextDescriptor{
			DeviceContext:  nativePointer(ctx.platform.wgl.deviceContext),
			ShareContext:   nativePointer(unsafe.Pointer(ctx.context)),
			GetProcAddress: 0,
		}}, nil
	}
	config := ctx.platform.egl.windowConfig
	if texture {
		config = ctx.platform.egl.pbufferConfig
		if config == nil {
			return maplibre.OpenGLContextDescriptor{}, errors.New("no EGL config compatible with the current context supports pbuffer surfaces")
		}
	}
	return maplibre.OpenGLContextDescriptor{EGL: &maplibre.EGLContextDescriptor{
		Display:        nativePointer(ctx.platform.egl.display),
		Config:         nativePointer(config),
		ShareContext:   nativePointer(unsafe.Pointer(ctx.context)),
		GetProcAddress: 0,
	}}, nil
}

func (ctx *openGLContext) surface() maplibre.NativePointer {
	if ctx.platform.wgl != nil {
		return nativePointer(ctx.platform.wgl.deviceContext)
	}
	return nativePointer(ctx.platform.egl.surface)
}

func (ctx *openGLContext) refreshPlatformSurface() error {
	if ctx.platform.egl == nil {
		return nil
	}
	if err := ctx.MakeCurrent(); err != nil {
		return err
	}
	platform, err := platformOpenGLContext(ctx.window, ctx.context)
	if err != nil {
		return err
	}
	ctx.platform.egl = platform.egl
	return nil
}

func nativePointer(value unsafe.Pointer) maplibre.NativePointer {
	return maplibre.NativePointer(uintptr(value))
}

func newOpenGLRenderTarget(context *openGLContext, v viewport, mode renderTargetMode, m *maplibre.MapHandle) (renderTarget, error) {
	switch mode {
	case modeOwnedTexture:
		return newOpenGLOwnedTextureTarget(context, v, m)
	case modeBorrowedTexture:
		return newOpenGLBorrowedTextureTarget(context, v, m)
	case modeNativeSurface:
		return newOpenGLSurfaceTarget(context, v, m)
	default:
		return nil, fmt.Errorf("unsupported render target mode: %s", mode)
	}
}

type openGLTextureCompositor struct {
	context *openGLContext
	program uint32
	vao     uint32
	view    viewport
}

func newOpenGLTextureCompositor(context *openGLContext, v viewport) (*openGLTextureCompositor, error) {
	compositor := &openGLTextureCompositor{context: context, view: v}
	program, err := createTextureProgram()
	if err != nil {
		_ = compositor.Close()
		return nil, err
	}
	compositor.program = program
	compositor.vao = glGenVertexArray()
	if compositor.vao == 0 {
		_ = compositor.Close()
		return nil, errors.New("glGenVertexArrays returned 0")
	}
	glUseProgram(compositor.program)
	if sampler := glGetUniformLocation(compositor.program, "map_texture"); sampler >= 0 {
		glUniform1i(sampler, 0)
	}
	glUseProgram(0)
	if err := checkGLError("initialize OpenGL texture compositor"); err != nil {
		_ = compositor.Close()
		return nil, err
	}
	return compositor, nil
}

func (compositor *openGLTextureCompositor) Close() error {
	var result error
	if compositor.context != nil {
		result = errors.Join(result, compositor.context.MakeCurrent())
	}
	glFinish()
	if compositor.vao != 0 {
		glDeleteVertexArray(compositor.vao)
		compositor.vao = 0
	}
	if compositor.program != 0 {
		glDeleteProgram(compositor.program)
		compositor.program = 0
	}
	return result
}

func (compositor *openGLTextureCompositor) Resize(v viewport) error {
	if err := compositor.context.MakeCurrent(); err != nil {
		return err
	}
	compositor.view = v
	return nil
}

func (compositor *openGLTextureCompositor) FinishFrame() error {
	if err := compositor.context.MakeCurrent(); err != nil {
		return err
	}
	glFinish()
	return nil
}

func (compositor *openGLTextureCompositor) DrawTexture(target uint32, texture uint32) error {
	if err := compositor.context.MakeCurrent(); err != nil {
		return err
	}
	clearGLErrors()
	glBindFramebuffer(0x8D40, 0)
	glDisable(0x0B44)
	glDisable(0x0B71)
	glDisable(0x0C11)
	glViewport(0, 0, int32(compositor.view.physicalWidth), int32(compositor.view.physicalHeight))
	glClearColor(0.08, 0.09, 0.11, 1)
	glClear(0x00004000)
	glUseProgram(compositor.program)
	glBindVertexArray(compositor.vao)
	glActiveTexture(0x84C0)
	glBindTexture(target, texture)
	glTexParameteri(target, 0x2801, 0x2601)
	glTexParameteri(target, 0x2800, 0x2601)
	glDrawArrays(0x0004, 0, 3)
	glBindTexture(target, 0)
	glBindVertexArray(0)
	glUseProgram(0)
	if err := checkGLError("draw OpenGL texture"); err != nil {
		return err
	}
	return compositor.context.SwapWindow()
}

type openGLOwnedTextureTarget struct {
	compositor *openGLTextureCompositor
	driver     callerDriver
}

func newOpenGLOwnedTextureTarget(context *openGLContext, v viewport, m *maplibre.MapHandle) (*openGLOwnedTextureTarget, error) {
	compositor, err := newOpenGLTextureCompositor(context, v)
	if err != nil {
		return nil, err
	}
	target := &openGLOwnedTextureTarget{compositor: compositor}
	descriptor, err := compositor.context.descriptor(true)
	if err != nil {
		_ = target.Close()
		return nil, err
	}
	options := maplibre.NewRenderSessionAttachOptions()
	options.Driver = maplibre.RenderDriverCallerGraphicsThread
	options.RequestedTextureRingDepth = 2
	session, operation, err := m.AttachOpenGLOwnedTexture(
		maplibre.OpenGLOwnedTextureDescriptor{
			Extent:  v.extent(),
			Context: descriptor,
		},
		options,
	)
	if err == nil {
		err = target.driver.Attach(m, session, operation)
	}
	if err != nil {
		_ = target.Close()
		return nil, fmt.Errorf("OpenGL texture attach failed: %w", err)
	}
	return target, nil
}

func (target *openGLOwnedTextureTarget) Close() error {
	result := target.driver.Close()
	if target.compositor != nil {
		result = errors.Join(result, target.compositor.Close())
		target.compositor = nil
	}
	return result
}

func (target *openGLOwnedTextureTarget) Resize(v viewport) error {
	if err := target.compositor.Resize(v); err != nil {
		return err
	}
	return target.driver.Resize(v)
}

func (target *openGLOwnedTextureTarget) PollPending() (bool, error) {
	return target.driver.PollPending()
}

func (target *openGLOwnedTextureTarget) FinishFrame() error {
	return target.compositor.FinishFrame()
}

func (target *openGLOwnedTextureTarget) DriveFrame() (frameOutcome, error) {
	outcome, err := target.driver.RenderFrame(false)
	if err != nil {
		return frameOutcome{}, fmt.Errorf("OpenGL texture render failed: %w", err)
	}
	if !outcome.rendered {
		return outcome, nil
	}
	frame, err := target.driver.AcquireFrame()
	if err != nil {
		return frameOutcome{}, err
	}
	if frame == nil {
		outcome.rendered = false
		return outcome, nil
	}
	accessErr := requireCPUCompleteProducer(frame)
	var drawErr error
	if accessErr == nil {
		var info maplibre.OpenGLOwnedTextureFrameInfo
		info, accessErr = frame.OpenGLTexture()
		if accessErr == nil {
			drawErr = target.compositor.DrawTexture(info.Target, info.Texture)
		}
	}
	releaseErr := frame.Release(maplibre.GPUSync{
		Kind: maplibre.GPUSyncCPUComplete,
	})
	outcome.rendered = accessErr == nil && drawErr == nil
	return outcome, errors.Join(accessErr, drawErr, releaseErr)
}

type openGLBorrowedTextureTarget struct {
	compositor *openGLTextureCompositor
	driver     callerDriver
	texture    uint32
	// retiredTexture is the texture the session still renders into until the
	// pending target replacement completes.
	retiredTexture uint32
}

func newOpenGLBorrowedTextureTarget(context *openGLContext, v viewport, m *maplibre.MapHandle) (*openGLBorrowedTextureTarget, error) {
	compositor, err := newOpenGLTextureCompositor(context, v)
	if err != nil {
		return nil, err
	}
	target := &openGLBorrowedTextureTarget{compositor: compositor}
	texture, err := createBorrowedTexture(compositor.context, v)
	if err != nil {
		_ = target.Close()
		return nil, err
	}
	target.texture = texture
	descriptor, err := compositor.context.descriptor(true)
	if err != nil {
		_ = target.Close()
		return nil, err
	}
	options := maplibre.NewRenderSessionAttachOptions()
	options.Driver = maplibre.RenderDriverCallerGraphicsThread
	session, operation, err := m.AttachOpenGLBorrowedTexture(maplibre.OpenGLBorrowedTextureDescriptor{
		Extent:         v.extent(),
		PhysicalWidth:  v.physicalWidth,
		PhysicalHeight: v.physicalHeight,
		Context:        descriptor,
		Texture:        texture,
		Target:         glTexture2D,
	}, options)
	if err == nil {
		err = target.driver.Attach(m, session, operation)
	}
	if err != nil {
		_ = target.Close()
		return nil, fmt.Errorf("OpenGL borrowed texture attach failed: %w", err)
	}
	return target, nil
}

func (target *openGLBorrowedTextureTarget) Close() error {
	result := target.driver.Close()
	if target.texture != 0 || target.retiredTexture != 0 {
		result = errors.Join(result, target.compositor.context.MakeCurrent())
		if target.texture != 0 {
			glDeleteTexture(target.texture)
			target.texture = 0
		}
		target.releaseRetiredTexture()
	}
	if target.compositor != nil {
		result = errors.Join(result, target.compositor.Close())
		target.compositor = nil
	}
	return result
}

func (target *openGLBorrowedTextureTarget) PollPending() (bool, error) {
	pending, err := target.driver.PollPending()
	if !pending {
		target.releaseRetiredTexture()
	}
	return pending, err
}

func (target *openGLBorrowedTextureTarget) releaseRetiredTexture() {
	if target.retiredTexture == 0 {
		return
	}
	glDeleteTexture(target.retiredTexture)
	target.retiredTexture = 0
}

// Resize hands the live session a texture at the new size; the session keeps
// its renderer across the handover.
func (target *openGLBorrowedTextureTarget) Resize(v viewport) error {
	descriptor, err := target.compositor.context.descriptor(true)
	if err != nil {
		return err
	}
	replacement, err := createBorrowedTexture(target.compositor.context, v)
	if err != nil {
		return err
	}
	operation, err := target.driver.session.SetOpenGLBorrowedTextureTarget(maplibre.OpenGLBorrowedTextureDescriptor{
		Extent:         v.extent(),
		PhysicalWidth:  v.physicalWidth,
		PhysicalHeight: v.physicalHeight,
		Context:        descriptor,
		Texture:        replacement,
		Target:         glTexture2D,
	})
	if err != nil {
		glDeleteTexture(replacement)
		return fmt.Errorf("OpenGL borrowed texture set target failed: %w", err)
	}
	target.driver.BeginPending(operation)
	// The session keeps rendering into the outgoing texture until the
	// replacement commits, so it outlives this call.
	target.retiredTexture = target.texture
	target.texture = replacement
	if err := target.driver.ResizeMap(v); err != nil {
		return err
	}
	return target.compositor.Resize(v)
}

func (target *openGLBorrowedTextureTarget) FinishFrame() error {
	return target.compositor.FinishFrame()
}

func (target *openGLBorrowedTextureTarget) DriveFrame() (frameOutcome, error) {
	outcome, err := target.driver.RenderFrame(false)
	if err != nil {
		return frameOutcome{}, fmt.Errorf("OpenGL borrowed texture render failed: %w", err)
	}
	if !outcome.rendered {
		return outcome, nil
	}
	return outcome, target.compositor.DrawTexture(glTexture2D, target.texture)
}

func createBorrowedTexture(context *openGLContext, v viewport) (uint32, error) {
	if err := context.MakeCurrent(); err != nil {
		return 0, err
	}
	texture := glGenTexture()
	glBindTexture(glTexture2D, texture)
	glTexParameteri(glTexture2D, 0x2801, 0x2601)
	glTexParameteri(glTexture2D, 0x2800, 0x2601)
	glTexImage2D(glTexture2D, 0, 0x1908, int32(v.physicalWidth), int32(v.physicalHeight), 0, 0x1908, 0x1401)
	glBindTexture(glTexture2D, 0)
	if err := checkGLError("create OpenGL borrowed texture"); err != nil {
		if texture != 0 {
			glDeleteTexture(texture)
		}
		return 0, err
	}
	return texture, nil
}

type openGLSurfaceTarget struct {
	context *openGLContext
	driver  callerDriver
}

func newOpenGLSurfaceTarget(context *openGLContext, v viewport, m *maplibre.MapHandle) (*openGLSurfaceTarget, error) {
	if err := context.refreshPlatformSurface(); err != nil {
		return nil, fmt.Errorf("OpenGL surface refresh failed: %w", err)
	}
	target := &openGLSurfaceTarget{context: context}
	descriptor, err := context.descriptor(false)
	if err != nil {
		return nil, err
	}
	options := maplibre.NewRenderSessionAttachOptions()
	options.Driver = maplibre.RenderDriverCallerGraphicsThread
	session, operation, err := m.AttachOpenGLSurface(
		maplibre.OpenGLSurfaceDescriptor{
			Extent:  v.extent(),
			Context: descriptor,
			Surface: context.surface(),
		},
		options,
	)
	if err == nil {
		err = target.driver.Attach(m, session, operation)
	}
	if err != nil {
		_ = target.Close()
		return nil, fmt.Errorf("OpenGL surface attach failed: %w", err)
	}
	return target, nil
}

func (target *openGLSurfaceTarget) Close() error {
	return target.driver.Close()
}

func (target *openGLSurfaceTarget) PollPending() (bool, error) {
	return target.driver.PollPending()
}

// Resize handles SDL returning a different EGL window surface for the resized
// window by handing the live session the replacement.
func (target *openGLSurfaceTarget) Resize(v viewport) error {
	outgoing := target.context.surface()
	if err := target.context.refreshPlatformSurface(); err != nil {
		// SDL may already have dropped the surface the session presents
		// through, so detach rather than leave it naming a dead surface.
		_ = target.driver.Close()
		return err
	}
	if target.context.surface() == outgoing {
		return target.driver.Resize(v)
	}
	descriptor, err := target.context.descriptor(false)
	if err != nil {
		return err
	}
	operation, err := target.driver.session.SetOpenGLSurfaceTarget(maplibre.OpenGLSurfaceDescriptor{
		Extent:  v.extent(),
		Context: descriptor,
		Surface: target.context.surface(),
	})
	if err != nil {
		_ = target.driver.Close()
		return fmt.Errorf("OpenGL surface set target failed: %w", err)
	}
	target.driver.BeginPending(operation)
	return target.driver.ResizeMap(v)
}

func (target *openGLSurfaceTarget) FinishFrame() error {
	if err := target.context.MakeCurrent(); err != nil {
		return err
	}
	glFinish()
	return nil
}

func (target *openGLSurfaceTarget) DriveFrame() (frameOutcome, error) {
	outcome, err := target.driver.RenderFrame(true)
	if err != nil {
		return frameOutcome{}, fmt.Errorf("OpenGL surface render failed: %w", err)
	}
	return outcome, nil
}

func createTextureProgram() (uint32, error) {
	vertexSource := desktopTextureVertexShader
	fragmentSource := desktopTextureFragmentShader
	if usesEGL() {
		vertexSource = glesTextureVertexShader
		fragmentSource = glesTextureFragmentShader
	}
	vertex, err := compileShader(0x8B31, vertexSource, "texture vertex shader")
	if err != nil {
		return 0, err
	}
	defer glDeleteShader(vertex)
	fragment, err := compileShader(0x8B30, fragmentSource, "texture fragment shader")
	if err != nil {
		return 0, err
	}
	defer glDeleteShader(fragment)
	program := glCreateProgram()
	if program == 0 {
		return 0, errors.New("glCreateProgram returned 0")
	}
	glAttachShader(program, vertex)
	glAttachShader(program, fragment)
	glLinkProgram(program)
	if glGetProgramiv(program, 0x8B82) == 0 {
		log := glGetProgramInfoLog(program)
		glDeleteProgram(program)
		return 0, fmt.Errorf("OpenGL compositor program link failed: %s", log)
	}
	return program, nil
}

func compileShader(kind uint32, source string, name string) (uint32, error) {
	shader := glCreateShader(kind)
	if shader == 0 {
		return 0, errors.New("glCreateShader returned 0")
	}
	glShaderSource(shader, source)
	glCompileShader(shader)
	if glGetShaderiv(shader, 0x8B81) == 0 {
		log := glGetShaderInfoLog(shader)
		glDeleteShader(shader)
		return 0, fmt.Errorf("OpenGL compositor %s compile failed: %s", name, log)
	}
	return shader, nil
}

func checkGLError(operation string) error {
	if glError := glGetError(); glError != 0 {
		return fmt.Errorf("%s failed with OpenGL error 0x%x", operation, glError)
	}
	return nil
}

func clearGLErrors() {
	for glGetError() != 0 {
	}
}

const desktopTextureVertexShader = `#version 130
out vec2 out_uv;
vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
void main() {
  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
  out_uv = uvs[gl_VertexID];
}`

const desktopTextureFragmentShader = `#version 130
uniform sampler2D map_texture;
in vec2 out_uv;
out vec4 out_color;
void main() {
  out_color = texture(map_texture, out_uv);
}`

const glesTextureVertexShader = `#version 300 es
out vec2 out_uv;
const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
const vec2 uvs[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
void main() {
  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
  out_uv = uvs[gl_VertexID];
}`

const glesTextureFragmentShader = `#version 300 es
precision mediump float;
uniform sampler2D map_texture;
in vec2 out_uv;
out vec4 out_color;
void main() {
  out_color = texture(map_texture, out_uv);
}`
