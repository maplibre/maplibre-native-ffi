// mod.zig — OpenGL (EGL) backend for zig-map.
//
// One render target mode is supported:
//
//   native_surface  — The SDL3 GL context IS the EGL context; its EGL handles
//                     are passed directly to mln_egl_surface_attach().  The
//                     session renders into the window surface and calls
//                     eglSwapBuffers after each frame.  This is the simplest
//                     and most efficient mode.
//
//   owned_texture / borrowed_texture — Not supported for OpenGL; returns
//                     BackendSetupFailed.
//
// Requires GLES 3.0 (Mesa llvmpipe on Ubuntu 24.04 supports 3.2).  The caller
// must have called setupGLAttributes() before SDL_CreateWindow so that SDL3
// requests the correct GL profile.

const std = @import("std");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const maplibre = @import("maplibre_native");
const render_target = @import("../../render_target.zig");
const types = @import("../../types.zig");

pub const OpenGLBackend = union(enum) {
    pub const window_flags = c.SDL_WINDOW_OPENGL;

    native_surface: OpenGLSurfaceBackend,

    /// Must be called after SDL_Init but before SDL_CreateWindow.
    pub fn setupGLAttributes() void {
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 0);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_ES);
    }

    pub fn init(
        _: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !OpenGLBackend {
        _ = viewport;
        return switch (mode) {
            .owned_texture => {
                std.debug.print(
                    "OpenGL owned-texture mode is not supported; " ++
                        "use native-surface\n",
                    .{},
                );
                return types.AppError.BackendSetupFailed;
            },
            .borrowed_texture => {
                std.debug.print(
                    "OpenGL borrowed-texture mode is not supported; " ++
                        "use native-surface\n",
                    .{},
                );
                return types.AppError.BackendSetupFailed;
            },
            .native_surface => .{
                .native_surface = try OpenGLSurfaceBackend.init(window),
            },
        };
    }

    pub fn deinit(self: *OpenGLBackend) void {
        switch (self.*) {
            .native_surface => |*backend| backend.deinit(),
        }
    }

    pub fn resize(_: *OpenGLBackend, _: types.Viewport) !void {
        switch (self.*) {
            .native_surface => {},
        }
    }

    pub fn needsRenderTargetReattachOnResize(self: *const OpenGLBackend) bool {
        return switch (self.*) {
            .native_surface => false,
        };
    }

    pub fn finishFrame(self: *OpenGLBackend) !void {
        switch (self.*) {
            .native_surface => {},
        }
    }

    pub fn attachRenderTarget(
        self: *OpenGLBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        return switch (self.*) {
            .native_surface => |*backend| backend.attachRenderTarget(map, viewport),
        };
    }

    pub fn drawTexture(
        _: *OpenGLBackend,
        _: *maplibre.RenderSessionHandle,
        _: types.Viewport,
    ) !bool {
        return switch (self.*) {
            .native_surface => unreachable,
        };
    }
};

// ── Native surface backend ────────────────────────────────────────────────────
//
// SDL3 creates the EGL context internally when SDL_GL_CreateContext is called.
// We query the EGL handles back from SDL3: SDL_EGL_GetCurrentDisplay() and
// SDL_EGL_GetWindowSurface() return real EGL types directly.  The EGL context
// handle is NOT obtainable as a real EGLContext from SDL_GL_GetCurrentContext()
// (that returns an opaque SDL wrapper); instead we call eglGetCurrentContext()
// directly from <EGL/egl.h> after SDL makes the context current.

const OpenGLSurfaceBackend = struct {
    gl_ctx: c.SDL_GLContext,
    window: *c.SDL_Window,

    fn init(window: *c.SDL_Window) !OpenGLSurfaceBackend {
        const gl_ctx = c.SDL_GL_CreateContext(window);
        if (gl_ctx == null) {
            std.debug.print(
                "SDL_GL_CreateContext failed: {s}\n",
                .{std.mem.span(c.SDL_GetError())},
            );
            return types.AppError.BackendSetupFailed;
        }
        return .{ .gl_ctx = gl_ctx, .window = window };
    }

    fn deinit(self: *OpenGLSurfaceBackend) void {
        _ = c.SDL_GL_DestroyContext(self.gl_ctx);
    }

    fn resize(_: *OpenGLSurfaceBackend, _: types.Viewport) !void {}

    fn finishFrame(_: *OpenGLSurfaceBackend) !void {}

    fn attachRenderTarget(
        self: *OpenGLSurfaceBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        // SDL3 uses EGL internally on Linux; retrieve the borrowed handles.
        // SDL_EGL_GetCurrentDisplay/GetWindowSurface return real EGL opaque
        // types, but SDL_GL_GetCurrentContext returns an opaque SDL wrapper
        // rather than the underlying EGLContext.  Use eglGetCurrentContext()
        // directly to obtain the true EGLContext handle.
        const egl_display = c.SDL_EGL_GetCurrentDisplay();
        const egl_context = c.eglGetCurrentContext();
        const egl_surface = c.SDL_EGL_GetWindowSurface(self.window);

        if (egl_display == null or
            egl_context == null or egl_surface == null)
        {
            std.debug.print(
                "EGL handles not available from SDL3 (is the GL context EGL-backed?): {s}\n",
                .{std.mem.span(c.SDL_GetError())},
            );
            return types.AppError.BackendSetupFailed;
        }

        const session = maplibre.attachEglSurface(map, .{
            .extent = render_target.extent(viewport),
            .display = .{ .ptr = egl_display.? },
            .context = .{ .ptr = egl_context.? },
            .surface = .{ .ptr = egl_surface.? },
        }) catch |err| {
            diagnostics.logError("EGL surface attach failed", err);
            return types.AppError.SurfaceAttachFailed;
        };
        return .{ .surface = session };
    }
};
