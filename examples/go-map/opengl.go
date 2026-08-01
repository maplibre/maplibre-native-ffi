package main

import (
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
	// Resize follows a resized host without losing the session: every mode
	// either resizes in place or hands the live session a replacement target.
	Resize(viewport) error
	FinishFrame() error
	RenderUpdate() (bool, error)
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
	session    *maplibre.RenderSessionHandle
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
	session, err := m.AttachOpenGLOwnedTexture(maplibre.OpenGLOwnedTextureDescriptor{Extent: v.extent(), Context: descriptor})
	if err != nil {
		_ = target.Close()
		return nil, fmt.Errorf("OpenGL texture attach failed: %w", err)
	}
	target.session = session
	return target, nil
}

func (target *openGLOwnedTextureTarget) Close() error {
	var result error
	if target.session != nil {
		result = errors.Join(result, target.session.Close())
		target.session = nil
	}
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
	return target.session.Resize(v.extent())
}

func (target *openGLOwnedTextureTarget) FinishFrame() error { return target.compositor.FinishFrame() }

func (target *openGLOwnedTextureTarget) RenderUpdate() (bool, error) {
	rendered, err := target.session.RenderUpdate()
	if err != nil {
		return false, fmt.Errorf("OpenGL texture render failed: %w", err)
	}
	if !rendered {
		return false, nil
	}
	frame, err := target.session.AcquireOpenGLTextureFrame()
	if err != nil {
		if errors.Is(err, maplibre.ErrInvalidState) {
			return false, nil
		}
		return false, fmt.Errorf("OpenGL texture acquire failed: %w", err)
	}
	defer func() { _ = frame.Close() }()
	texture, err := frame.Texture()
	if err != nil {
		return false, err
	}
	textureTarget, err := frame.Target()
	if err != nil {
		return false, err
	}
	return true, target.compositor.DrawTexture(textureTarget, texture)
}

type openGLBorrowedTextureTarget struct {
	compositor *openGLTextureCompositor
	session    *maplibre.RenderSessionHandle
	texture    uint32
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
	session, err := m.AttachOpenGLBorrowedTexture(maplibre.OpenGLBorrowedTextureDescriptor{
		Extent:         v.extent(),
		PhysicalWidth:  v.physicalWidth,
		PhysicalHeight: v.physicalHeight,
		Context:        descriptor,
		Texture:        texture,
		Target:         glTexture2D,
	})
	if err != nil {
		_ = target.Close()
		return nil, fmt.Errorf("OpenGL borrowed texture attach failed: %w", err)
	}
	target.session = session
	return target, nil
}

func (target *openGLBorrowedTextureTarget) Close() error {
	var result error
	if target.session != nil {
		result = errors.Join(result, target.session.Close())
		target.session = nil
	}
	if target.texture != 0 {
		result = errors.Join(result, target.compositor.context.MakeCurrent())
		glDeleteTexture(target.texture)
		target.texture = 0
	}
	if target.compositor != nil {
		result = errors.Join(result, target.compositor.Close())
		target.compositor = nil
	}
	return result
}

// Resize follows a resized host. A host-owned texture is sized by this example
// rather than by the session, so following a resize means creating one at the
// new size and handing it to the live session. The session keeps its renderer
// across the handover, which is what keeps the map from going cold every time
// the window changes size.
func (target *openGLBorrowedTextureTarget) Resize(v viewport) error {
	descriptor, err := target.compositor.context.descriptor(true)
	if err != nil {
		return err
	}
	replacement, err := createBorrowedTexture(target.compositor.context, v)
	if err != nil {
		return err
	}
	if err := target.session.SetOpenGLBorrowedTextureTarget(maplibre.OpenGLBorrowedTextureDescriptor{
		Extent:         v.extent(),
		PhysicalWidth:  v.physicalWidth,
		PhysicalHeight: v.physicalHeight,
		Context:        descriptor,
		Texture:        replacement,
		Target:         glTexture2D,
	}); err != nil {
		// A native error may mean the session took the replacement before
		// failing, and nothing here can tell that apart from a rejection that
		// came first, so detach before either texture is deleted.
		_ = target.session.Detach()
		glDeleteTexture(replacement)
		return fmt.Errorf("OpenGL borrowed texture set target failed: %w", err)
	}
	// Only once the session has taken the replacement, so a rejected one leaves
	// this target on the texture it already had.
	outgoing := target.texture
	target.texture = replacement
	if outgoing != 0 {
		glDeleteTexture(outgoing)
	}
	return target.compositor.Resize(v)
}

func (target *openGLBorrowedTextureTarget) FinishFrame() error {
	return target.compositor.FinishFrame()
}

func (target *openGLBorrowedTextureTarget) RenderUpdate() (bool, error) {
	rendered, err := target.session.RenderUpdate()
	if err != nil {
		return false, fmt.Errorf("OpenGL borrowed texture render failed: %w", err)
	}
	if !rendered {
		return false, nil
	}
	return true, target.compositor.DrawTexture(glTexture2D, target.texture)
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
	session *maplibre.RenderSessionHandle
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
	session, err := m.AttachOpenGLSurface(maplibre.OpenGLSurfaceDescriptor{Extent: v.extent(), Context: descriptor, Surface: context.surface()})
	if err != nil {
		_ = target.Close()
		return nil, fmt.Errorf("OpenGL surface attach failed: %w", err)
	}
	target.session = session
	return target, nil
}

func (target *openGLSurfaceTarget) Close() error {
	var result error
	if target.session != nil {
		result = errors.Join(result, target.session.Close())
		target.session = nil
	}
	return result
}

// Resize follows a resized host. SDL can hand back a different EGL window
// surface for the resized window, and the live session takes that replacement
// rather than being closed and attached again, so the map keeps its renderer.
func (target *openGLSurfaceTarget) Resize(v viewport) error {
	outgoing := target.context.surface()
	if err := target.context.refreshPlatformSurface(); err != nil {
		// SDL owns the surfaces and may already have dropped the one the
		// session presents through, so detach rather than leave it naming a
		// surface that is gone.
		_ = target.session.Detach()
		return err
	}
	if target.context.surface() == outgoing {
		return target.session.Resize(v.extent())
	}
	descriptor, err := target.context.descriptor(false)
	if err != nil {
		return err
	}
	if err := target.session.SetOpenGLSurfaceTarget(maplibre.OpenGLSurfaceDescriptor{
		Extent:  v.extent(),
		Context: descriptor,
		Surface: target.context.surface(),
	}); err != nil {
		// SDL owns both surfaces and already dropped the outgoing one, so on a
		// native error the session may be holding a surface that is gone.
		// Detaching is the only way to stop it naming either.
		_ = target.session.Detach()
		return fmt.Errorf("OpenGL surface set target failed: %w", err)
	}
	return nil
}

func (target *openGLSurfaceTarget) FinishFrame() error {
	if err := target.context.MakeCurrent(); err != nil {
		return err
	}
	glFinish()
	return nil
}

func (target *openGLSurfaceTarget) RenderUpdate() (bool, error) {
	rendered, err := target.session.RenderUpdate()
	if err != nil {
		return false, fmt.Errorf("OpenGL surface render failed: %w", err)
	}
	return rendered, nil
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
