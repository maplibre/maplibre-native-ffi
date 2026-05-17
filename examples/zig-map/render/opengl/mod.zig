// mod.zig — OpenGL (EGL) backend for zig-map.
//
// Two render target modes are supported:
//
//   native_surface  — The SDL3 GL context IS the EGL context; its EGL handles
//                     are passed directly to mln_egl_surface_attach().  The
//                     session renders into the window surface and calls
//                     eglSwapBuffers after each frame.  This is the simplest
//                     and most efficient mode.
//
//   owned_texture   — A maplibre-owned headless EGL context renders off-screen.
//                     A separate SDL3 GL context (the compositor) reads back
//                     the pixels and blits them to the window via a full-screen
//                     triangle shader.
//
//   borrowed_texture — Not supported for OpenGL; returns BackendSetupFailed.
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
const Compositor = @import("compositor.zig").Compositor;

pub const OpenGLBackend = union(enum) {
    pub const window_flags = c.SDL_WINDOW_OPENGL;

    owned_texture: OpenGLOwnedTextureBackend,
    native_surface: OpenGLSurfaceBackend,

    /// Must be called after SDL_Init but before SDL_CreateWindow.
    pub fn setupGLAttributes() void {
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_MINOR_VERSION, 0);
        _ = c.SDL_GL_SetAttribute(c.SDL_GL_CONTEXT_PROFILE_MASK, c.SDL_GL_CONTEXT_PROFILE_ES);
    }

    pub fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !OpenGLBackend {
        _ = viewport;
        return switch (mode) {
            .owned_texture => .{
                .owned_texture = try OpenGLOwnedTextureBackend.init(allocator, window),
            },
            .borrowed_texture => {
                std.debug.print(
                    "OpenGL borrowed-texture mode is not supported; " ++
                        "use owned-texture or native-surface\n",
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
            .owned_texture => |*backend| backend.deinit(),
            .native_surface => |*backend| backend.deinit(),
        }
    }

    pub fn resize(self: *OpenGLBackend, viewport: types.Viewport) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.resize(viewport),
            .native_surface => {},
        }
    }

    pub fn needsRenderTargetReattachOnResize(self: *const OpenGLBackend) bool {
        return switch (self.*) {
            .owned_texture, .native_surface => false,
        };
    }

    pub fn finishFrame(self: *OpenGLBackend) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.finishFrame(),
            .native_surface => {},
        }
    }

    pub fn attachRenderTarget(
        self: *OpenGLBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        return switch (self.*) {
            .owned_texture => |*backend| backend.attachRenderTarget(map, viewport),
            .native_surface => |*backend| backend.attachRenderTarget(map, viewport),
        };
    }

    pub fn drawTexture(
        self: *OpenGLBackend,
        texture: *c.mln_render_session,
        viewport: types.Viewport,
    ) !bool {
        return switch (self.*) {
            .owned_texture => |*backend| backend.drawTexture(texture, viewport),
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

        var descriptor = c.mln_egl_surface_descriptor_default();
        descriptor.width = viewport.logical_width;
        descriptor.height = viewport.logical_height;
        descriptor.scale_factor = viewport.scale_factor;
        descriptor.display = egl_display;
        descriptor.context = egl_context;
        descriptor.surface = egl_surface;

        const raw_map = try maplibre.native(map);
        var session: ?*c.mln_render_session = null;
        if (c.mln_egl_surface_attach(raw_map, &descriptor, &session) != c.MLN_STATUS_OK or
            session == null)
        {
            diagnostics.logAbiError("EGL surface attach failed");
            return types.AppError.SurfaceAttachFailed;
        }
        return .{ .surface = session.? };
    }
};

// ── Owned texture backend ─────────────────────────────────────────────────────
//
// maplibre renders into its own internal headless EGL context.  We blit the
// result to the window via a GL compositor that uploads the pixel readback as
// a texture and draws a full-screen triangle.

const OpenGLOwnedTextureBackend = struct {
    allocator: std.mem.Allocator,
    compositor: Compositor,

    fn init(allocator: std.mem.Allocator, window: *c.SDL_Window) !OpenGLOwnedTextureBackend {
        return .{
            .allocator = allocator,
            .compositor = try Compositor.init(allocator, window),
        };
    }

    fn deinit(self: *OpenGLOwnedTextureBackend) void {
        self.compositor.deinit(self.allocator);
    }

    fn resize(_: *OpenGLOwnedTextureBackend, _: types.Viewport) !void {}

    fn finishFrame(_: *OpenGLOwnedTextureBackend) !void {}

    fn attachRenderTarget(
        _: *OpenGLOwnedTextureBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        var descriptor = c.mln_owned_texture_descriptor_default();
        descriptor.extent.width = viewport.logical_width;
        descriptor.extent.height = viewport.logical_height;
        descriptor.extent.scale_factor = viewport.scale_factor;

        const raw_map = try maplibre.native(map);
        var texture: ?*c.mln_render_session = null;
        if (c.mln_owned_texture_attach(raw_map, &descriptor, &texture) != c.MLN_STATUS_OK or
            texture == null)
        {
            diagnostics.logAbiError("OpenGL owned texture attach failed");
            return types.AppError.TextureAttachFailed;
        }
        return .{ .texture = texture.? };
    }

    fn drawTexture(
        self: *OpenGLOwnedTextureBackend,
        texture: *c.mln_render_session,
        viewport: types.Viewport,
    ) !bool {
        return self.compositor.presentTexture(self.allocator, texture, viewport);
    }
};
