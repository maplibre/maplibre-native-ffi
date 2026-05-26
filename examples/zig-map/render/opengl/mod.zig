const std = @import("std");
const builtin = @import("builtin");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const maplibre = @import("maplibre_native");
const render_target = @import("../../render_target.zig");
const types = @import("../../types.zig");

pub const OpenGLBackend = if (builtin.os.tag == .windows) WindowsOpenGLBackend else LinuxTodoOpenGLBackend;

const gl = struct {
    const GLenum = c_uint;
    const GLint = c_int;
    const GLsizei = c_int;
    const GLuint = c_uint;
    const GLbitfield = c_uint;
    const GLfloat = f32;
    const GLdouble = f64;

    const COLOR_BUFFER_BIT: GLbitfield = 0x0000_4000;
    const LINEAR: GLint = 0x2601;
    const MODELVIEW: GLenum = 0x1700;
    const NO_ERROR: GLenum = 0;
    const PROJECTION: GLenum = 0x1701;
    const QUADS: GLenum = 0x0007;
    const RGBA: GLenum = 0x1908;
    const TEXTURE_2D: GLenum = 0x0de1;
    const TEXTURE_MAG_FILTER: GLenum = 0x2800;
    const TEXTURE_MIN_FILTER: GLenum = 0x2801;
    const UNSIGNED_BYTE: GLenum = 0x1401;

    extern "opengl32" fn glBegin(mode: GLenum) callconv(.winapi) void;
    extern "opengl32" fn glBindTexture(target: GLenum, texture: GLuint) callconv(.winapi) void;
    extern "opengl32" fn glClear(mask: GLbitfield) callconv(.winapi) void;
    extern "opengl32" fn glClearColor(red: GLfloat, green: GLfloat, blue: GLfloat, alpha: GLfloat) callconv(.winapi) void;
    extern "opengl32" fn glDeleteTextures(n: GLsizei, textures: *const GLuint) callconv(.winapi) void;
    extern "opengl32" fn glDisable(cap: GLenum) callconv(.winapi) void;
    extern "opengl32" fn glEnable(cap: GLenum) callconv(.winapi) void;
    extern "opengl32" fn glEnd() callconv(.winapi) void;
    extern "opengl32" fn glFinish() callconv(.winapi) void;
    extern "opengl32" fn glGenTextures(n: GLsizei, textures: *GLuint) callconv(.winapi) void;
    extern "opengl32" fn glGetError() callconv(.winapi) GLenum;
    extern "opengl32" fn glLoadIdentity() callconv(.winapi) void;
    extern "opengl32" fn glMatrixMode(mode: GLenum) callconv(.winapi) void;
    extern "opengl32" fn glOrtho(left: GLdouble, right: GLdouble, bottom: GLdouble, top: GLdouble, z_near: GLdouble, z_far: GLdouble) callconv(.winapi) void;
    extern "opengl32" fn glTexCoord2f(s: GLfloat, t: GLfloat) callconv(.winapi) void;
    extern "opengl32" fn glTexImage2D(target: GLenum, level: GLint, internal_format: GLint, width: GLsizei, height: GLsizei, border: GLint, format: GLenum, @"type": GLenum, pixels: ?*const anyopaque) callconv(.winapi) void;
    extern "opengl32" fn glTexParameteri(target: GLenum, pname: GLenum, param: GLint) callconv(.winapi) void;
    extern "opengl32" fn glVertex2f(x: GLfloat, y: GLfloat) callconv(.winapi) void;
    extern "opengl32" fn glViewport(x: GLint, y: GLint, width: GLsizei, height: GLsizei) callconv(.winapi) void;
};

const gl_texture_target = gl.TEXTURE_2D;
const gl_internal_format = gl.RGBA;
const gl_pixel_format = gl.RGBA;
const gl_pixel_type = gl.UNSIGNED_BYTE;

const WindowsOpenGLBackend = union(enum) {
    pub const window_flags = c.SDL_WINDOW_OPENGL;

    owned_texture: OpenGLOwnedTextureBackend,
    borrowed_texture: OpenGLBorrowedTextureBackend,
    native_surface: OpenGLSurfaceBackend,

    pub fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !WindowsOpenGLBackend {
        _ = allocator;
        return switch (mode) {
            .owned_texture => .{ .owned_texture = try OpenGLOwnedTextureBackend.init(window, viewport) },
            .borrowed_texture => .{ .borrowed_texture = try OpenGLBorrowedTextureBackend.init(window, viewport) },
            .native_surface => .{ .native_surface = try OpenGLSurfaceBackend.init(window, viewport) },
        };
    }

    pub fn deinit(self: *WindowsOpenGLBackend) void {
        switch (self.*) {
            .owned_texture => |*backend| backend.deinit(),
            .borrowed_texture => |*backend| backend.deinit(),
            .native_surface => |*backend| backend.deinit(),
        }
    }

    pub fn resize(self: *WindowsOpenGLBackend, viewport: types.Viewport) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.resize(viewport),
            .borrowed_texture => |*backend| try backend.resize(viewport),
            .native_surface => |*backend| try backend.resize(viewport),
        }
    }

    pub fn needsRenderTargetReattachOnResize(self: *const WindowsOpenGLBackend) bool {
        return switch (self.*) {
            .owned_texture, .native_surface => false,
            .borrowed_texture => true,
        };
    }

    pub fn finishFrame(self: *WindowsOpenGLBackend) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.finishFrame(),
            .borrowed_texture => |*backend| try backend.finishFrame(),
            .native_surface => |*backend| try backend.finishFrame(),
        }
    }

    pub fn attachRenderTarget(
        self: *WindowsOpenGLBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        return switch (self.*) {
            .owned_texture => |*backend| backend.attachRenderTarget(map, viewport),
            .borrowed_texture => |*backend| backend.attachRenderTarget(map, viewport),
            .native_surface => |*backend| backend.attachRenderTarget(map, viewport),
        };
    }

    pub fn drawTexture(
        self: *WindowsOpenGLBackend,
        texture: *maplibre.RenderSessionHandle,
        viewport: types.Viewport,
    ) !bool {
        return switch (self.*) {
            .owned_texture => |*backend| backend.drawTexture(texture, viewport),
            .borrowed_texture => |*backend| backend.drawTexture(texture, viewport),
            .native_surface => unreachable,
        };
    }
};

const OpenGLContext = struct {
    window: *c.SDL_Window,
    context: c.SDL_GLContext,
    device_context: *anyopaque,

    fn init(window: *c.SDL_Window) !OpenGLContext {
        const context = c.SDL_GL_CreateContext(window) orelse {
            logSdlError("SDL_GL_CreateContext failed");
            return types.AppError.BackendSetupFailed;
        };
        errdefer _ = c.SDL_GL_DestroyContext(context);
        if (!c.SDL_GL_MakeCurrent(window, context)) {
            logSdlError("SDL_GL_MakeCurrent failed");
            return types.AppError.BackendSetupFailed;
        }
        const properties = c.SDL_GetWindowProperties(window);
        if (properties == 0) {
            logSdlError("SDL_GetWindowProperties failed");
            return types.AppError.BackendSetupFailed;
        }
        const device_context = c.SDL_GetPointerProperty(
            properties,
            c.SDL_PROP_WINDOW_WIN32_HDC_POINTER,
            null,
        ) orelse return types.AppError.BackendSetupFailed;
        return .{
            .window = window,
            .context = context,
            .device_context = device_context,
        };
    }

    fn deinit(self: *OpenGLContext) void {
        _ = c.SDL_GL_MakeCurrent(self.window, null);
        _ = c.SDL_GL_DestroyContext(self.context);
        self.context = null;
    }

    fn makeCurrent(self: *const OpenGLContext) !void {
        if (!c.SDL_GL_MakeCurrent(self.window, self.context)) {
            logSdlError("SDL_GL_MakeCurrent failed");
            return types.AppError.BackendSetupFailed;
        }
    }

    fn swapWindow(self: *const OpenGLContext) !void {
        if (!c.SDL_GL_SwapWindow(self.window)) {
            logSdlError("SDL_GL_SwapWindow failed");
            return types.AppError.BackendDrawFailed;
        }
    }

    fn descriptor(self: *const OpenGLContext) maplibre.OpenGLContextDescriptor {
        return .{ .wgl = .{
            .device_context = .{ .ptr = @ptrCast(self.device_context) },
            .share_context = .{ .ptr = @ptrCast(self.context) },
            .get_proc_address = null,
        } };
    }

    fn surface(self: *const OpenGLContext) maplibre.NativePointer {
        return .{ .ptr = @ptrCast(self.device_context) };
    }
};

const OpenGLTextureCompositor = struct {
    context: OpenGLContext,
    viewport: types.Viewport,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGLTextureCompositor {
        return .{
            .context = try OpenGLContext.init(window),
            .viewport = viewport,
        };
    }

    fn deinit(self: *OpenGLTextureCompositor) void {
        self.context.makeCurrent() catch {};
        gl.glFinish();
        self.context.deinit();
    }

    fn resize(self: *OpenGLTextureCompositor, viewport: types.Viewport) !void {
        try self.context.makeCurrent();
        self.viewport = viewport;
    }

    fn finishFrame(self: *OpenGLTextureCompositor) !void {
        try self.context.makeCurrent();
        gl.glFinish();
    }

    fn drawTexture(self: *OpenGLTextureCompositor, texture: gl.GLuint) !bool {
        try self.context.makeCurrent();
        drawTextureQuad(texture, self.viewport);
        try self.context.swapWindow();
        return true;
    }
};

const OpenGLOwnedTextureBackend = struct {
    compositor: OpenGLTextureCompositor,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGLOwnedTextureBackend {
        return .{ .compositor = try OpenGLTextureCompositor.init(window, viewport) };
    }

    fn deinit(self: *OpenGLOwnedTextureBackend) void {
        self.compositor.deinit();
    }

    fn resize(self: *OpenGLOwnedTextureBackend, viewport: types.Viewport) !void {
        try self.compositor.resize(viewport);
    }

    fn finishFrame(self: *OpenGLOwnedTextureBackend) !void {
        try self.compositor.finishFrame();
    }

    fn attachRenderTarget(
        self: *OpenGLOwnedTextureBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const texture = maplibre.attachOpenGLOwnedTexture(map, .{
            .extent = render_target.extent(viewport),
            .context = self.compositor.context.descriptor(),
        }) catch |err| {
            diagnostics.logError("OpenGL texture attach failed", err);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
    }

    fn drawTexture(
        self: *OpenGLOwnedTextureBackend,
        texture: *maplibre.RenderSessionHandle,
        _: types.Viewport,
    ) !bool {
        var frame = texture.acquireOpenGLOwnedTextureFrame() catch |err| switch (err) {
            error.InvalidState => return false,
            else => {
                diagnostics.logError("OpenGL texture acquire failed", err);
                return types.AppError.BackendDrawFailed;
            },
        };
        defer frame.release() catch |err| diagnostics.logError("OpenGL texture release failed", err);

        const info = try frame.info();
        return try self.compositor.drawTexture(info.texture);
    }
};

const BorrowedTexture = struct {
    texture: gl.GLuint,

    fn init(context: *const OpenGLContext, viewport: types.Viewport) !BorrowedTexture {
        try context.makeCurrent();
        var texture: gl.GLuint = 0;
        gl.glGenTextures(1, &texture);
        gl.glBindTexture(gl_texture_target, texture);
        gl.glTexParameteri(gl_texture_target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.glTexParameteri(gl_texture_target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.glTexImage2D(
            gl_texture_target,
            0,
            @intCast(gl_internal_format),
            @intCast(viewport.physical_width),
            @intCast(viewport.physical_height),
            0,
            gl_pixel_format,
            gl_pixel_type,
            null,
        );
        gl.glBindTexture(gl_texture_target, 0);
        try checkGlError("create OpenGL borrowed texture");
        return .{ .texture = texture };
    }

    fn deinit(self: *BorrowedTexture, context: *const OpenGLContext) void {
        if (self.texture == 0) return;
        context.makeCurrent() catch {};
        gl.glDeleteTextures(1, &self.texture);
        self.texture = 0;
    }
};

const OpenGLBorrowedTextureBackend = struct {
    compositor: OpenGLTextureCompositor,
    borrowed_texture: BorrowedTexture,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGLBorrowedTextureBackend {
        var compositor = try OpenGLTextureCompositor.init(window, viewport);
        errdefer compositor.deinit();
        return .{
            .borrowed_texture = try BorrowedTexture.init(&compositor.context, viewport),
            .compositor = compositor,
        };
    }

    fn deinit(self: *OpenGLBorrowedTextureBackend) void {
        self.borrowed_texture.deinit(&self.compositor.context);
        self.compositor.deinit();
    }

    fn resize(self: *OpenGLBorrowedTextureBackend, viewport: types.Viewport) !void {
        var borrowed_texture = try BorrowedTexture.init(&self.compositor.context, viewport);
        errdefer borrowed_texture.deinit(&self.compositor.context);
        self.borrowed_texture.deinit(&self.compositor.context);
        self.borrowed_texture = borrowed_texture;
        try self.compositor.resize(viewport);
    }

    fn finishFrame(self: *OpenGLBorrowedTextureBackend) !void {
        try self.compositor.finishFrame();
    }

    fn attachRenderTarget(
        self: *OpenGLBorrowedTextureBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const texture = maplibre.attachOpenGLBorrowedTexture(map, .{
            .extent = render_target.extent(viewport),
            .context = self.compositor.context.descriptor(),
            .texture = self.borrowed_texture.texture,
            .target = gl_texture_target,
        }) catch |err| {
            diagnostics.logError("OpenGL borrowed texture attach failed", err);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
    }

    fn drawTexture(
        self: *OpenGLBorrowedTextureBackend,
        texture: *maplibre.RenderSessionHandle,
        _: types.Viewport,
    ) !bool {
        _ = texture;
        return try self.compositor.drawTexture(self.borrowed_texture.texture);
    }
};

const OpenGLSurfaceBackend = struct {
    context: OpenGLContext,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGLSurfaceBackend {
        _ = viewport;
        return .{ .context = try OpenGLContext.init(window) };
    }

    fn deinit(self: *OpenGLSurfaceBackend) void {
        self.context.deinit();
    }

    fn resize(_: *OpenGLSurfaceBackend, _: types.Viewport) !void {}

    fn finishFrame(self: *OpenGLSurfaceBackend) !void {
        try self.context.makeCurrent();
        gl.glFinish();
    }

    fn attachRenderTarget(
        self: *OpenGLSurfaceBackend,
        map: *maplibre.MapHandle,
        viewport: types.Viewport,
    ) !render_target.Session {
        const surface = maplibre.attachOpenGLSurface(map, .{
            .extent = render_target.extent(viewport),
            .context = self.context.descriptor(),
            .surface = self.context.surface(),
        }) catch |err| {
            diagnostics.logError("OpenGL surface attach failed", err);
            return types.AppError.SurfaceAttachFailed;
        };
        return .{ .surface = surface };
    }
};

fn drawTextureQuad(texture: gl.GLuint, viewport: types.Viewport) void {
    gl.glViewport(0, 0, @intCast(viewport.physical_width), @intCast(viewport.physical_height));
    gl.glClearColor(0.08, 0.09, 0.11, 1.0);
    gl.glClear(gl.COLOR_BUFFER_BIT);
    gl.glMatrixMode(gl.PROJECTION);
    gl.glLoadIdentity();
    gl.glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    gl.glMatrixMode(gl.MODELVIEW);
    gl.glLoadIdentity();
    gl.glEnable(gl_texture_target);
    gl.glBindTexture(gl_texture_target, texture);
    gl.glTexParameteri(gl_texture_target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.glTexParameteri(gl_texture_target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.glBegin(gl.QUADS);
    gl.glTexCoord2f(0.0, 0.0);
    gl.glVertex2f(-1.0, -1.0);
    gl.glTexCoord2f(1.0, 0.0);
    gl.glVertex2f(1.0, -1.0);
    gl.glTexCoord2f(1.0, 1.0);
    gl.glVertex2f(1.0, 1.0);
    gl.glTexCoord2f(0.0, 1.0);
    gl.glVertex2f(-1.0, 1.0);
    gl.glEnd();
    gl.glBindTexture(gl_texture_target, 0);
    gl.glDisable(gl_texture_target);
}

fn checkGlError(operation: []const u8) !void {
    const gl_error = gl.glGetError();
    if (gl_error == gl.NO_ERROR) return;
    std.debug.print("{s} failed with OpenGL error 0x{x}\n", .{ operation, gl_error });
    return types.AppError.BackendSetupFailed;
}

fn logSdlError(message: []const u8) void {
    const err = c.SDL_GetError();
    const details = if (err == null) "" else std.mem.span(err);
    std.debug.print("{s}: {s}\n", .{ message, details });
}

const LinuxTodoOpenGLBackend = struct {
    pub const window_flags = c.SDL_WINDOW_OPENGL;

    pub fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !LinuxTodoOpenGLBackend {
        _ = allocator;
        _ = window;
        _ = viewport;
        _ = mode;
        // TODO(linux): Add EGL/SDL context and surface support for zig-map
        // on a Linux machine with the Mesa/llvmpipe environment available.
        return types.AppError.BackendSetupFailed;
    }

    pub fn deinit(_: *LinuxTodoOpenGLBackend) void {}
    pub fn resize(_: *LinuxTodoOpenGLBackend, _: types.Viewport) !void {}
    pub fn needsRenderTargetReattachOnResize(_: *const LinuxTodoOpenGLBackend) bool {
        return false;
    }
    pub fn finishFrame(_: *LinuxTodoOpenGLBackend) !void {}
    pub fn attachRenderTarget(
        _: *LinuxTodoOpenGLBackend,
        _: *maplibre.MapHandle,
        _: types.Viewport,
    ) !render_target.Session {
        return types.AppError.BackendSetupFailed;
    }
    pub fn drawTexture(
        _: *LinuxTodoOpenGLBackend,
        _: *maplibre.RenderSessionHandle,
        _: types.Viewport,
    ) !bool {
        return types.AppError.BackendDrawFailed;
    }
};
