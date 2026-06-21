const std = @import("std");
const builtin = @import("builtin");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const gl = @import("gl");
const maplibre = @import("maplibre_native");
const render_target = @import("../../render_target.zig");
const types = @import("../../types.zig");

pub const OpenGLRenderTarget = PlatformOpenGLRenderTarget;
const uses_egl = builtin.os.tag == .linux or builtin.os.tag == .macos;

fn GlProc(comptime name: []const u8) type {
    return @TypeOf(@field(@as(gl.ProcTable, undefined), name));
}

const OpenGLCompositorProcs = struct {
    ActiveTexture: GlProc("ActiveTexture"),
    AttachShader: GlProc("AttachShader"),
    BindFramebuffer: GlProc("BindFramebuffer"),
    BindTexture: GlProc("BindTexture"),
    BindVertexArray: GlProc("BindVertexArray"),
    Clear: GlProc("Clear"),
    ClearColor: GlProc("ClearColor"),
    CompileShader: GlProc("CompileShader"),
    CreateProgram: GlProc("CreateProgram"),
    CreateShader: GlProc("CreateShader"),
    DeleteProgram: GlProc("DeleteProgram"),
    DeleteShader: GlProc("DeleteShader"),
    DeleteTextures: GlProc("DeleteTextures"),
    DeleteVertexArrays: GlProc("DeleteVertexArrays"),
    Disable: GlProc("Disable"),
    DrawArrays: GlProc("DrawArrays"),
    Finish: GlProc("Finish"),
    GenTextures: GlProc("GenTextures"),
    GenVertexArrays: GlProc("GenVertexArrays"),
    GetError: GlProc("GetError"),
    GetProgramInfoLog: GlProc("GetProgramInfoLog"),
    GetProgramiv: GlProc("GetProgramiv"),
    GetShaderInfoLog: GlProc("GetShaderInfoLog"),
    GetShaderiv: GlProc("GetShaderiv"),
    GetUniformLocation: GlProc("GetUniformLocation"),
    LinkProgram: GlProc("LinkProgram"),
    ShaderSource: GlProc("ShaderSource"),
    TexImage2D: GlProc("TexImage2D"),
    TexParameteri: GlProc("TexParameteri"),
    Uniform1i: GlProc("Uniform1i"),
    UseProgram: GlProc("UseProgram"),
    Viewport: GlProc("Viewport"),
};

const required_gl_commands = .{
    "ActiveTexture",
    "AttachShader",
    "BindFramebuffer",
    "BindTexture",
    "BindVertexArray",
    "Clear",
    "ClearColor",
    "CompileShader",
    "CreateProgram",
    "CreateShader",
    "DeleteProgram",
    "DeleteShader",
    "DeleteTextures",
    "DeleteVertexArrays",
    "Disable",
    "DrawArrays",
    "Finish",
    "GenTextures",
    "GenVertexArrays",
    "GetError",
    "GetProgramInfoLog",
    "GetProgramiv",
    "GetShaderInfoLog",
    "GetShaderiv",
    "GetUniformLocation",
    "LinkProgram",
    "ShaderSource",
    "TexImage2D",
    "TexParameteri",
    "Uniform1i",
    "UseProgram",
    "Viewport",
};

const desktop_texture_vertex_shader =
    \\#version 130
    \\out vec2 out_uv;
    \\vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    \\vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
    \\void main() {
    \\  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    \\  out_uv = uvs[gl_VertexID];
    \\}
;

const desktop_texture_fragment_shader =
    \\#version 130
    \\uniform sampler2D map_texture;
    \\in vec2 out_uv;
    \\out vec4 out_color;
    \\void main() {
    \\  out_color = texture(map_texture, out_uv);
    \\}
;

const gles_texture_vertex_shader =
    \\#version 300 es
    \\out vec2 out_uv;
    \\const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    \\const vec2 uvs[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
    \\void main() {
    \\  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    \\  out_uv = uvs[gl_VertexID];
    \\}
;

const gles_texture_fragment_shader =
    \\#version 300 es
    \\precision mediump float;
    \\uniform sampler2D map_texture;
    \\in vec2 out_uv;
    \\out vec4 out_color;
    \\void main() {
    \\  out_color = texture(map_texture, out_uv);
    \\}
;

fn loadGlProc(comptime T: type, name: [:0]const u8) !T {
    const proc = c.SDL_GL_GetProcAddress(name.ptr) orelse {
        std.debug.print("SDL_GL_GetProcAddress failed for {s}: {s}\n", .{ name, std.mem.span(c.SDL_GetError()) });
        return types.AppError.BackendSetupFailed;
    };
    return @ptrCast(proc);
}

fn glProcName(comptime command: []const u8) [:0]const u8 {
    return "gl" ++ command;
}

fn loadOpenGLCompositorProcs() !OpenGLCompositorProcs {
    var procs: OpenGLCompositorProcs = undefined;
    inline for (required_gl_commands) |command| {
        @field(procs, command) = try loadGlProc(@TypeOf(@field(procs, command)), glProcName(command));
    }
    return procs;
}

fn createTextureProgram(procs: OpenGLCompositorProcs) !gl.uint {
    const vertex_source = if (uses_egl) gles_texture_vertex_shader else desktop_texture_vertex_shader;
    const fragment_source = if (uses_egl) gles_texture_fragment_shader else desktop_texture_fragment_shader;

    const vertex = try compileShader(procs, gl.VERTEX_SHADER, vertex_source, "texture vertex shader");
    defer procs.DeleteShader(vertex);
    const fragment = try compileShader(procs, gl.FRAGMENT_SHADER, fragment_source, "texture fragment shader");
    defer procs.DeleteShader(fragment);

    const program = procs.CreateProgram();
    if (program == 0) return types.AppError.BackendSetupFailed;
    procs.AttachShader(program, vertex);
    procs.AttachShader(program, fragment);
    procs.LinkProgram(program);
    var linked: gl.int = 0;
    procs.GetProgramiv(program, gl.LINK_STATUS, @ptrCast(&linked));
    if (linked == gl.FALSE) {
        logProgramInfoLog(procs, program, "OpenGL compositor program link failed");
        procs.DeleteProgram(program);
        return types.AppError.BackendSetupFailed;
    }
    return program;
}

fn compileShader(
    procs: OpenGLCompositorProcs,
    kind: gl.@"enum",
    source: []const u8,
    name: []const u8,
) !gl.uint {
    const shader = procs.CreateShader(kind);
    if (shader == 0) return types.AppError.BackendSetupFailed;
    errdefer procs.DeleteShader(shader);

    const sources = [_][*]const gl.char{source.ptr};
    const lengths = [_]gl.int{@intCast(source.len)};
    procs.ShaderSource(shader, 1, sources[0..].ptr, lengths[0..].ptr);
    procs.CompileShader(shader);
    var compiled: gl.int = 0;
    procs.GetShaderiv(shader, gl.COMPILE_STATUS, @ptrCast(&compiled));
    if (compiled == gl.FALSE) {
        logShaderInfoLog(procs, shader, name);
        return types.AppError.BackendSetupFailed;
    }
    return shader;
}

fn logShaderInfoLog(procs: OpenGLCompositorProcs, shader: gl.uint, name: []const u8) void {
    var buffer: [1024]gl.char = undefined;
    var length: gl.sizei = 0;
    procs.GetShaderInfoLog(shader, @intCast(buffer.len), &length, buffer[0..].ptr);
    const log = buffer[0..@min(@as(usize, @intCast(length)), buffer.len)];
    std.debug.print("OpenGL compositor {s} compile failed: {s}\n", .{ name, log });
}

fn logProgramInfoLog(procs: OpenGLCompositorProcs, program: gl.uint, message: []const u8) void {
    var buffer: [1024]gl.char = undefined;
    var length: gl.sizei = 0;
    procs.GetProgramInfoLog(program, @intCast(buffer.len), &length, buffer[0..].ptr);
    const log = buffer[0..@min(@as(usize, @intCast(length)), buffer.len)];
    std.debug.print("{s}: {s}\n", .{ message, log });
}

const gl_texture_target: gl.@"enum" = gl.TEXTURE_2D;
const gl_internal_format: gl.int = gl.RGBA;
const gl_pixel_format: gl.@"enum" = gl.RGBA;
const gl_pixel_type: gl.@"enum" = gl.UNSIGNED_BYTE;

const PlatformOpenGLRenderTarget = union(enum) {
    pub const window_flags = c.SDL_WINDOW_OPENGL;

    owned_texture: OpenGLOwnedTextureBackend,
    borrowed_texture: OpenGLBorrowedTextureBackend,
    native_surface: OpenGLSurfaceBackend,

    pub fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
        map: *maplibre.MapHandle,
    ) !PlatformOpenGLRenderTarget {
        _ = allocator;
        return switch (mode) {
            .owned_texture => .{ .owned_texture = try OpenGLOwnedTextureBackend.init(window, viewport, map) },
            .borrowed_texture => .{ .borrowed_texture = try OpenGLBorrowedTextureBackend.init(window, viewport, map) },
            .native_surface => .{ .native_surface = try OpenGLSurfaceBackend.init(window, viewport, map) },
        };
    }

    pub fn deinit(self: *PlatformOpenGLRenderTarget) void {
        switch (self.*) {
            .owned_texture => |*backend| backend.deinit(),
            .borrowed_texture => |*backend| backend.deinit(),
            .native_surface => |*backend| backend.deinit(),
        }
    }

    pub fn resize(self: *PlatformOpenGLRenderTarget, viewport: types.Viewport) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.resize(viewport),
            .borrowed_texture => |*backend| try backend.resize(viewport),
            .native_surface => |*backend| try backend.resize(viewport),
        }
    }

    pub fn needsReattachOnResize(self: *const PlatformOpenGLRenderTarget) bool {
        return switch (self.*) {
            .owned_texture, .native_surface => false,
            .borrowed_texture => true,
        };
    }

    pub fn finishFrame(self: *PlatformOpenGLRenderTarget) !void {
        switch (self.*) {
            .owned_texture => |*backend| try backend.finishFrame(),
            .borrowed_texture => |*backend| try backend.finishFrame(),
            .native_surface => |*backend| try backend.finishFrame(),
        }
    }

    pub fn renderUpdate(
        self: *PlatformOpenGLRenderTarget,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        return switch (self.*) {
            .owned_texture => |*backend| backend.renderUpdate(diagnostic_store, viewport),
            .borrowed_texture => |*backend| backend.renderUpdate(diagnostic_store, viewport),
            .native_surface => |*backend| backend.renderUpdate(diagnostic_store),
        };
    }
};

const OpenGLContext = struct {
    const Platform = union(enum) {
        wgl: struct {
            device_context: *anyopaque,
        },
        egl: struct {
            display: *anyopaque,
            config: *anyopaque,
            surface: *anyopaque,
        },
    };

    window: *c.SDL_Window,
    context: c.SDL_GLContext,
    platform: Platform,

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
        const platform = try platformContext(window);
        return .{
            .window = window,
            .context = context,
            .platform = platform,
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
        return switch (self.platform) {
            .wgl => |wgl| .{ .wgl = .{
                .device_context = maplibre.NativePointer.fromPtr(@ptrCast(wgl.device_context)),
                .share_context = maplibre.NativePointer.fromPtr(@ptrCast(self.context)),
                .get_proc_address = null,
            } },
            .egl => |egl| .{ .egl = .{
                .display = maplibre.NativePointer.fromPtr(@ptrCast(egl.display)),
                .config = maplibre.NativePointer.fromPtr(@ptrCast(egl.config)),
                .share_context = maplibre.NativePointer.fromPtr(@ptrCast(self.context)),
                .get_proc_address = null,
            } },
        };
    }

    fn surface(self: *const OpenGLContext) maplibre.NativePointer {
        return switch (self.platform) {
            .wgl => |wgl| maplibre.NativePointer.fromPtr(@ptrCast(wgl.device_context)),
            .egl => |egl| maplibre.NativePointer.fromPtr(@ptrCast(egl.surface)),
        };
    }
};

fn platformContext(window: *c.SDL_Window) !OpenGLContext.Platform {
    switch (builtin.os.tag) {
        .windows => {
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
            return .{ .wgl = .{ .device_context = device_context } };
        },
        .linux, .macos => {
            const display = c.SDL_EGL_GetCurrentDisplay() orelse {
                logSdlError("SDL_EGL_GetCurrentDisplay failed");
                return types.AppError.BackendSetupFailed;
            };
            const config = c.SDL_EGL_GetCurrentConfig() orelse {
                logSdlError("SDL_EGL_GetCurrentConfig failed");
                return types.AppError.BackendSetupFailed;
            };
            const surface = c.SDL_EGL_GetWindowSurface(window) orelse {
                logSdlError("SDL_EGL_GetWindowSurface failed");
                return types.AppError.BackendSetupFailed;
            };
            return .{ .egl = .{
                .display = display,
                .config = config,
                .surface = surface,
            } };
        },
        else => return types.AppError.BackendSetupFailed,
    }
}

const OpenGLTextureCompositor = struct {
    context: OpenGLContext,
    viewport: types.Viewport,
    procs: OpenGLCompositorProcs,
    program: gl.uint,
    vertex_array: gl.uint,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGLTextureCompositor {
        var context = try OpenGLContext.init(window);
        errdefer context.deinit();
        const procs = try loadOpenGLCompositorProcs();
        const program = try createTextureProgram(procs);
        errdefer procs.DeleteProgram(program);
        var vertex_array: gl.uint = 0;
        procs.GenVertexArrays(1, @ptrCast(&vertex_array));
        if (vertex_array == 0) return types.AppError.BackendSetupFailed;
        errdefer if (vertex_array != 0) procs.DeleteVertexArrays(1, @ptrCast(&vertex_array));
        procs.UseProgram(program);
        const sampler = procs.GetUniformLocation(program, "map_texture");
        if (sampler >= 0) procs.Uniform1i(sampler, 0);
        procs.UseProgram(0);
        try checkGlError(procs, "initialize OpenGL texture compositor");
        return .{
            .context = context,
            .viewport = viewport,
            .procs = procs,
            .program = program,
            .vertex_array = vertex_array,
        };
    }

    fn deinit(self: *OpenGLTextureCompositor) void {
        self.context.makeCurrent() catch {};
        self.procs.Finish();
        if (self.vertex_array != 0) {
            self.procs.DeleteVertexArrays(1, @ptrCast(&self.vertex_array));
            self.vertex_array = 0;
        }
        if (self.program != 0) {
            self.procs.DeleteProgram(self.program);
            self.program = 0;
        }
        self.context.deinit();
    }

    fn resize(self: *OpenGLTextureCompositor, viewport: types.Viewport) !void {
        try self.context.makeCurrent();
        self.viewport = viewport;
    }

    fn finishFrame(self: *OpenGLTextureCompositor) !void {
        try self.context.makeCurrent();
        self.procs.Finish();
    }

    fn drawTexture(self: *OpenGLTextureCompositor, texture: gl.uint) !bool {
        try self.context.makeCurrent();
        try self.drawTextureQuad(texture);
        try self.context.swapWindow();
        return true;
    }

    fn drawTextureQuad(self: *OpenGLTextureCompositor, texture: gl.uint) !void {
        clearGlErrors(self.procs);
        self.procs.BindFramebuffer(gl.FRAMEBUFFER, 0);
        self.procs.Disable(gl.CULL_FACE);
        self.procs.Disable(gl.DEPTH_TEST);
        self.procs.Disable(gl.SCISSOR_TEST);
        self.procs.Viewport(0, 0, @intCast(self.viewport.physical_width), @intCast(self.viewport.physical_height));
        self.procs.ClearColor(0.08, 0.09, 0.11, 1.0);
        self.procs.Clear(gl.COLOR_BUFFER_BIT);
        self.procs.UseProgram(self.program);
        self.procs.BindVertexArray(self.vertex_array);
        self.procs.ActiveTexture(gl.TEXTURE0);
        self.procs.BindTexture(gl_texture_target, texture);
        self.procs.TexParameteri(gl_texture_target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        self.procs.TexParameteri(gl_texture_target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        self.procs.DrawArrays(gl.TRIANGLES, 0, 3);
        self.procs.BindTexture(gl_texture_target, 0);
        self.procs.BindVertexArray(0);
        self.procs.UseProgram(0);
        try checkGlError(self.procs, "draw OpenGL texture");
    }
};

const OpenGLOwnedTextureBackend = struct {
    compositor: OpenGLTextureCompositor,
    session: render_target.Session,

    fn init(
        window: *c.SDL_Window,
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !OpenGLOwnedTextureBackend {
        var self = OpenGLOwnedTextureBackend{
            .compositor = try OpenGLTextureCompositor.init(window, viewport),
            .session = .none,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
    }

    fn deinit(self: *OpenGLOwnedTextureBackend) void {
        self.session.deinit();
        self.compositor.deinit();
    }

    fn resize(self: *OpenGLOwnedTextureBackend, viewport: types.Viewport) !void {
        try self.compositor.resize(viewport);
        try self.session.resize(viewport, null);
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
            diagnostics.logError("OpenGL texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
    }

    fn renderUpdate(
        self: *OpenGLOwnedTextureBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        _ = viewport;
        if (!try self.session.renderUpdate(diagnostic_store)) return false;
        const texture = switch (self.session) {
            .texture => |*texture| texture,
            else => return false,
        };
        var frame = texture.acquireOpenGLOwnedTextureFrame() catch |err| switch (err) {
            error.InvalidState => return false,
            else => {
                diagnostics.logError("OpenGL texture acquire failed", err, null);
                return types.AppError.BackendDrawFailed;
            },
        };
        defer frame.release() catch |err| diagnostics.logError("OpenGL texture release failed", err, null);

        const info = try frame.info();
        return try self.compositor.drawTexture(info.texture);
    }
};

const BorrowedTexture = struct {
    texture: gl.uint,

    fn init(context: *const OpenGLContext, procs: OpenGLCompositorProcs, viewport: types.Viewport) !BorrowedTexture {
        try context.makeCurrent();
        var texture: gl.uint = 0;
        procs.GenTextures(1, @ptrCast(&texture));
        procs.BindTexture(gl_texture_target, texture);
        procs.TexParameteri(gl_texture_target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        procs.TexParameteri(gl_texture_target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        procs.TexImage2D(
            gl_texture_target,
            0,
            gl_internal_format,
            @intCast(viewport.physical_width),
            @intCast(viewport.physical_height),
            0,
            gl_pixel_format,
            gl_pixel_type,
            null,
        );
        procs.BindTexture(gl_texture_target, 0);
        try checkGlError(procs, "create OpenGL borrowed texture");
        return .{ .texture = texture };
    }

    fn deinit(self: *BorrowedTexture, context: *const OpenGLContext, procs: OpenGLCompositorProcs) void {
        if (self.texture == 0) return;
        context.makeCurrent() catch {};
        procs.DeleteTextures(1, @ptrCast(&self.texture));
        self.texture = 0;
    }
};

const OpenGLBorrowedTextureBackend = struct {
    compositor: OpenGLTextureCompositor,
    session: render_target.Session,
    borrowed_texture: BorrowedTexture,

    fn init(
        window: *c.SDL_Window,
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !OpenGLBorrowedTextureBackend {
        var compositor = try OpenGLTextureCompositor.init(window, viewport);
        errdefer compositor.deinit();
        var self = OpenGLBorrowedTextureBackend{
            .borrowed_texture = try BorrowedTexture.init(&compositor.context, compositor.procs, viewport),
            .session = .none,
            .compositor = compositor,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
    }

    fn deinit(self: *OpenGLBorrowedTextureBackend) void {
        self.session.deinit();
        self.borrowed_texture.deinit(&self.compositor.context, self.compositor.procs);
        self.compositor.deinit();
    }

    fn resize(_: *OpenGLBorrowedTextureBackend, _: types.Viewport) !void {
        return types.AppError.TextureResizeFailed;
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
            diagnostics.logError("OpenGL borrowed texture attach failed", err, null);
            return types.AppError.TextureAttachFailed;
        };
        return .{ .texture = texture };
    }

    fn renderUpdate(
        self: *OpenGLBorrowedTextureBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
        viewport: types.Viewport,
    ) !bool {
        _ = viewport;
        if (!try self.session.renderUpdate(diagnostic_store)) return false;
        return try self.compositor.drawTexture(self.borrowed_texture.texture);
    }
};

const OpenGLSurfaceBackend = struct {
    context: OpenGLContext,
    procs: OpenGLCompositorProcs,
    session: render_target.Session,

    fn init(
        window: *c.SDL_Window,
        viewport: types.Viewport,
        map: *maplibre.MapHandle,
    ) !OpenGLSurfaceBackend {
        var context = try OpenGLContext.init(window);
        errdefer context.deinit();
        var self = OpenGLSurfaceBackend{
            .context = context,
            .procs = try loadOpenGLCompositorProcs(),
            .session = .none,
        };
        errdefer self.deinit();
        self.session = try self.attachRenderTarget(map, viewport);
        return self;
    }

    fn deinit(self: *OpenGLSurfaceBackend) void {
        self.session.deinit();
        self.context.deinit();
    }

    fn resize(self: *OpenGLSurfaceBackend, viewport: types.Viewport) !void {
        try self.session.resize(viewport, null);
    }

    fn finishFrame(self: *OpenGLSurfaceBackend) !void {
        try self.context.makeCurrent();
        self.procs.Finish();
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
            diagnostics.logError("OpenGL surface attach failed", err, null);
            return types.AppError.SurfaceAttachFailed;
        };
        return .{ .surface = surface };
    }

    fn renderUpdate(
        self: *OpenGLSurfaceBackend,
        diagnostic_store: ?*const maplibre.DiagnosticStore,
    ) !bool {
        return try self.session.renderUpdate(diagnostic_store);
    }
};

fn checkGlError(procs: OpenGLCompositorProcs, operation: []const u8) !void {
    const gl_error = procs.GetError();
    if (gl_error == gl.NO_ERROR) return;
    std.debug.print("{s} failed with OpenGL error 0x{x}\n", .{ operation, gl_error });
    return types.AppError.BackendSetupFailed;
}

fn clearGlErrors(procs: OpenGLCompositorProcs) void {
    while (procs.GetError() != gl.NO_ERROR) {}
}

fn logSdlError(message: []const u8) void {
    const err = c.SDL_GetError();
    const details = if (err == null) "" else std.mem.span(err);
    std.debug.print("{s}: {s}\n", .{ message, details });
}
