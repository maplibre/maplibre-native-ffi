const std = @import("std");
const builtin = @import("builtin");

const c = @import("../../c.zig").c;
const diagnostics = @import("../../diagnostics.zig");
const gl = @import("gl");
const maplibre = @import("maplibre_native_ffi");
const render_target = @import("../../render_target.zig");
const types = @import("../../types.zig");

pub const OpenGlRenderTarget = PlatformOpenGlRenderTarget;
const uses_egl = builtin.os.tag == .linux or builtin.os.tag == .macos;

fn GlProc(comptime name: []const u8) type {
    return @TypeOf(@field(@as(gl.ProcTable, undefined), name));
}

const OpenGlCompositorProcs = struct {
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

fn loadOpenGlCompositorProcs() !OpenGlCompositorProcs {
    var procs: OpenGlCompositorProcs = undefined;
    inline for (required_gl_commands) |command| {
        @field(procs, command) = try loadGlProc(@TypeOf(@field(procs, command)), glProcName(command));
    }
    return procs;
}

fn createTextureProgram(procs: OpenGlCompositorProcs) !gl.uint {
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
    procs: OpenGlCompositorProcs,
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

fn logShaderInfoLog(procs: OpenGlCompositorProcs, shader: gl.uint, name: []const u8) void {
    var buffer: [1024]gl.char = undefined;
    var length: gl.sizei = 0;
    procs.GetShaderInfoLog(shader, @intCast(buffer.len), &length, buffer[0..].ptr);
    const log = buffer[0..@min(@as(usize, @intCast(length)), buffer.len)];
    std.debug.print("OpenGL compositor {s} compile failed: {s}\n", .{ name, log });
}

fn logProgramInfoLog(procs: OpenGlCompositorProcs, program: gl.uint, message: []const u8) void {
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

const PlatformOpenGlRenderTarget = union(types.RenderTargetMode) {
    pub const window_flags = c.SDL_WINDOW_OPENGL;

    owned_texture: OpenGlOwnedTextureBackend,
    borrowed_texture: OpenGlBorrowedTextureBackend,
    native_surface: OpenGlSurfaceBackend,

    pub fn init(
        allocator: std.mem.Allocator,
        window: *c.SDL_Window,
        viewport: types.Viewport,
        mode: types.RenderTargetMode,
    ) !PlatformOpenGlRenderTarget {
        _ = allocator;
        return switch (mode) {
            .owned_texture => .{ .owned_texture = .{ .compositor = try OpenGlTextureCompositor.init(window, viewport) } },
            .borrowed_texture => .{ .borrowed_texture = try OpenGlBorrowedTextureBackend.init(window, viewport) },
            .native_surface => .{ .native_surface = .{ .context = try OpenGlContext.init(window) } },
        };
    }

    /// Attaches the render session. An OpenGL target on the host's WGL or
    /// EGL context renders where that context is current, so the render loop
    /// services a caller driver.
    pub fn attach(self: *PlatformOpenGlRenderTarget, map: *maplibre.Map, viewport: types.Viewport) !void {
        switch (self.*) {
            inline else => |*backend| try backend.attach(map, viewport, render_target.attachOptions(self.*, .caller_graphics_thread)),
        }
    }

    /// Releases any held frame, then detaches the session.
    pub fn detach(self: *PlatformOpenGlRenderTarget) void {
        switch (self.*) {
            .owned_texture => |*backend| render_target.releaseFrame(&backend.held),
            .borrowed_texture => |*backend| render_target.releaseFrame(&backend.held),
            .native_surface => {},
        }
        self.session().deinit();
    }

    pub fn deinit(self: *PlatformOpenGlRenderTarget) void {
        switch (self.*) {
            inline else => |*backend| backend.deinit(),
        }
    }

    pub fn session(self: *PlatformOpenGlRenderTarget) *render_target.Session {
        return switch (self.*) {
            inline else => |*backend| &backend.session,
        };
    }

    /// Starts the session resize or target replacement a new viewport needs.
    pub fn resize(self: *PlatformOpenGlRenderTarget, viewport: types.Viewport) !void {
        switch (self.*) {
            inline else => |*backend| try backend.resize(viewport),
        }
    }

    /// Follows completed borrowed-ring replacements.
    pub fn retireReplaced(self: *PlatformOpenGlRenderTarget) !void {
        if (self.* == .borrowed_texture) try self.borrowed_texture.retireReplaced();
    }

    /// Shows the newest rendered frame, reporting false when no frame reached
    /// the window.
    pub fn present(self: *PlatformOpenGlRenderTarget, viewport: types.Viewport) !bool {
        _ = viewport;
        return switch (self.*) {
            .owned_texture => |*backend| presentAcquired(&backend.session, &backend.compositor, &backend.held),
            .borrowed_texture => |*backend| presentAcquired(&backend.session, &backend.compositor, &backend.held),
            // The driver already presented the frame.
            .native_surface => true,
        };
    }
};

const OpenGlContext = struct {
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

    fn init(window: *c.SDL_Window) !OpenGlContext {
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

    fn deinit(self: *OpenGlContext) void {
        _ = c.SDL_GL_MakeCurrent(self.window, null);
        _ = c.SDL_GL_DestroyContext(self.context);
        self.context = null;
    }

    fn makeCurrent(self: *const OpenGlContext) !void {
        if (!c.SDL_GL_MakeCurrent(self.window, self.context)) {
            logSdlError("SDL_GL_MakeCurrent failed");
            return types.AppError.BackendSetupFailed;
        }
    }

    fn swapWindow(self: *const OpenGlContext) !void {
        if (!c.SDL_GL_SwapWindow(self.window)) {
            logSdlError("SDL_GL_SwapWindow failed");
            return types.AppError.BackendDrawFailed;
        }
    }

    fn descriptor(self: *const OpenGlContext) maplibre.OpenGlContextDescriptor {
        return switch (self.platform) {
            .wgl => |wgl| .{ .ownership = .shared, .data = .{ .wgl = .{
                .device_context = (@ptrCast(wgl.device_context)),
                .share_context = (@ptrCast(self.context)),
                .get_proc_address = null,
            } } },
            .egl => |egl| .{ .ownership = .shared, .data = .{ .egl = .{
                .display = (@ptrCast(egl.display)),
                .config = (@ptrCast(egl.config)),
                .share_context = (@ptrCast(self.context)),
                .get_proc_address = null,
            } } },
        };
    }

    /// Re-reads the platform surface for this window and returns whether the
    /// handle changed. SDL can hand back a different EGL window surface after a
    /// resize, and a session still presenting through the old one draws nowhere.
    fn refreshPlatformSurface(self: *OpenGlContext) !bool {
        const previous = self.surface();
        self.platform = try platformContext(self.window);
        return !std.meta.eql(self.surface(), previous);
    }

    fn surface(self: *const OpenGlContext) ?*anyopaque {
        return switch (self.platform) {
            .wgl => |wgl| (@ptrCast(wgl.device_context)),
            .egl => |egl| (@ptrCast(egl.surface)),
        };
    }
};

fn platformContext(window: *c.SDL_Window) !OpenGlContext.Platform {
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

const OpenGlTextureCompositor = struct {
    context: OpenGlContext,
    viewport: types.Viewport,
    procs: OpenGlCompositorProcs,
    program: gl.uint,
    vertex_array: gl.uint,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGlTextureCompositor {
        var context = try OpenGlContext.init(window);
        errdefer context.deinit();
        const procs = try loadOpenGlCompositorProcs();
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

    fn deinit(self: *OpenGlTextureCompositor) void {
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

    fn resize(self: *OpenGlTextureCompositor, viewport: types.Viewport) void {
        self.viewport = viewport;
    }

    /// Samples texture into the window and presents it. The sampling finishes
    /// before this returns, so the caller may hand the texture back to the
    /// session.
    fn drawTexture(self: *OpenGlTextureCompositor, texture: gl.uint) !bool {
        try self.context.makeCurrent();
        try self.drawTextureQuad(texture);
        try self.context.swapWindow();
        self.procs.Finish();
        return true;
    }

    fn drawTextureQuad(self: *OpenGlTextureCompositor, texture: gl.uint) !void {
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

const OpenGlOwnedTextureBackend = struct {
    compositor: OpenGlTextureCompositor,
    session: render_target.Session = .{},
    /// The newest frame, held until a newer one replaces it.
    held: ?maplibre.AcquiredFrame = null,

    fn deinit(self: *OpenGlOwnedTextureBackend) void {
        render_target.releaseFrame(&self.held);
        self.session.deinit();
        self.compositor.deinit();
    }

    fn attach(self: *OpenGlOwnedTextureBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const attachment = map.attachOpenGlOwnedTexture(.{
            .extent = render_target.extent(viewport),
            .context = self.compositor.context.descriptor(),
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("OpenGL texture attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(attachment, options, .owned_texture);
    }

    fn resize(self: *OpenGlOwnedTextureBackend, viewport: types.Viewport) !void {
        self.compositor.resize(viewport);
        // A session resizes only while the host holds none of its frames.
        render_target.releaseFrame(&self.held);
        try self.session.resize(viewport);
    }
};

/// Acquires the newest frame into `held` and samples its texture into the
/// window. Both texture modes hand their frames over this way.
fn presentAcquired(session: *render_target.Session, compositor: *OpenGlTextureCompositor, held: *?maplibre.AcquiredFrame) !bool {
    // Without a new frame, the window keeps the one it already shows.
    if (!try session.acquireNewest(held)) return true;
    const frame = held.*.?;
    const Context = struct {
        compositor: *OpenGlTextureCompositor,
        frame: maplibre.AcquiredFrame,
        fn producer(context: @This(), sync: maplibre.GpuSync) anyerror!bool {
            if (sync.kind != .cpu_complete) return types.AppError.BackendDrawFailed;
            return context.frame.getOpenGlTexture(context, draw, null);
        }
        fn draw(context: @This(), info: maplibre.OpenGlTextureFrame) anyerror!bool {
            return context.compositor.drawTexture(info.texture);
        }
    };
    return frame.getProducerSync(Context{ .compositor = compositor, .frame = frame }, Context.producer, null);
}

const BorrowedTexture = struct {
    texture: gl.uint,

    fn init(context: *const OpenGlContext, procs: OpenGlCompositorProcs, viewport: types.Viewport) !BorrowedTexture {
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

    fn deinit(self: *BorrowedTexture, context: *const OpenGlContext, procs: OpenGlCompositorProcs) void {
        if (self.texture == 0) return;
        context.makeCurrent() catch {};
        procs.DeleteTextures(1, @ptrCast(&self.texture));
        self.texture = 0;
    }
};

/// The textures of a borrowed ring, one per slot.
const OpenGlRing = struct {
    textures: [render_target.borrowed_ring_depth]BorrowedTexture,

    fn init(context: *const OpenGlContext, procs: OpenGlCompositorProcs, viewport: types.Viewport) !OpenGlRing {
        var ring: OpenGlRing = undefined;
        var created: usize = 0;
        errdefer for (ring.textures[0..created]) |*texture| texture.deinit(context, procs);
        while (created < ring.textures.len) : (created += 1) {
            ring.textures[created] = try BorrowedTexture.init(context, procs, viewport);
        }
        return ring;
    }

    fn deinit(self: *OpenGlRing, context: *const OpenGlContext, procs: OpenGlCompositorProcs) void {
        for (&self.textures) |*texture| texture.deinit(context, procs);
    }

    fn entries(self: OpenGlRing) [render_target.borrowed_ring_depth]maplibre.OpenGlBorrowedTexture {
        var result: [render_target.borrowed_ring_depth]maplibre.OpenGlBorrowedTexture = undefined;
        for (self.textures, &result) |texture, *entry| entry.* = .{ .texture = texture.texture };
        return result;
    }
};

const OpenGlBorrowedTextureBackend = struct {
    compositor: OpenGlTextureCompositor,
    session: render_target.Session = .{},
    /// The newest frame, held until a newer one replaces it.
    held: ?maplibre.AcquiredFrame = null,
    /// The ring the session renders into.
    ring: OpenGlRing,
    replacements: render_target.Replacements(OpenGlRing) = .{},

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGlBorrowedTextureBackend {
        var compositor = try OpenGlTextureCompositor.init(window, viewport);
        errdefer compositor.deinit();
        return .{
            .ring = try OpenGlRing.init(&compositor.context, compositor.procs, viewport),
            .compositor = compositor,
        };
    }

    fn deinit(self: *OpenGlBorrowedTextureBackend) void {
        render_target.releaseFrame(&self.held);
        self.session.deinit();
        while (self.replacements.takeAny()) |ring| {
            var retired = ring;
            retired.deinit(&self.compositor.context, self.compositor.procs);
        }
        self.replacements.deinit();
        self.ring.deinit(&self.compositor.context, self.compositor.procs);
        self.compositor.deinit();
    }

    fn attach(self: *OpenGlBorrowedTextureBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const entries = self.ring.entries();
        const attachment = map.attachOpenGlBorrowedTexture(std.heap.smp_allocator, .{
            .extent = render_target.extent(viewport),
            .physical_width = viewport.physical_width,
            .physical_height = viewport.physical_height,
            .context = self.compositor.context.descriptor(),
            .textures = &entries,
            .target = gl_texture_target,
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("OpenGL borrowed texture attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(attachment, options, .borrowed_texture);
    }

    /// Follows a resized window: allocates a ring at the new size and hands it
    /// to the live session, which stays attached. A replacement is refused
    /// while the host holds a frame, so the held one goes first; the window
    /// keeps showing what it last presented.
    fn resize(self: *OpenGlBorrowedTextureBackend, viewport: types.Viewport) !void {
        self.compositor.resize(viewport);
        render_target.releaseFrame(&self.held);
        try self.replaceRing(viewport);
    }

    fn replaceRing(self: *OpenGlBorrowedTextureBackend, viewport: types.Viewport) !void {
        var replacement = try OpenGlRing.init(&self.compositor.context, self.compositor.procs, viewport);
        errdefer replacement.deinit(&self.compositor.context, self.compositor.procs);
        var diagnostic: maplibre.Diagnostic = .{};
        const entries = replacement.entries();
        var completion = (self.session.handle.?).setOpenGlBorrowedTextureTarget(std.heap.smp_allocator, .{
            .extent = render_target.extent(viewport),
            .physical_width = viewport.physical_width,
            .physical_height = viewport.physical_height,
            .context = self.compositor.context.descriptor(),
            .textures = &entries,
            .target = gl_texture_target,
        }, &diagnostic) catch |err| {
            diagnostics.logError("OpenGL borrowed texture set target failed", err, &diagnostic);
            return types.AppError.ResizeFailed;
        };
        self.replacements.push(completion, self.ring) catch |err| {
            completion.deinit();
            return err;
        };
        self.ring = replacement;
    }

    /// Destroys each ring that a completed replacement retired.
    fn retireReplaced(self: *OpenGlBorrowedTextureBackend) !void {
        while (try self.replacements.takeCompleted(&self.session)) |ring| {
            var retired = ring;
            retired.deinit(&self.compositor.context, self.compositor.procs);
        }
    }
};

const OpenGlSurfaceBackend = struct {
    context: OpenGlContext,
    session: render_target.Session = .{},

    fn deinit(self: *OpenGlSurfaceBackend) void {
        self.session.deinit();
        self.context.deinit();
    }

    fn attach(self: *OpenGlSurfaceBackend, map: *maplibre.Map, viewport: types.Viewport, options: maplibre.RenderSessionAttachOptions) !void {
        var diagnostic: maplibre.Diagnostic = .{};
        const attachment = map.attachOpenGlSurface(.{
            .extent = render_target.extent(viewport),
            .context = self.context.descriptor(),
            .surface = self.context.surface(),
        }, options, &diagnostic) catch |err| {
            diagnostics.logError("OpenGL surface attach failed", err, &diagnostic);
            return types.AppError.AttachFailed;
        };
        self.session = try render_target.Session.attach(attachment, options, .native_surface);
    }

    /// Follows a resized window. When SDL hands back a different EGL window
    /// surface, the live session takes the replacement rather than being closed
    /// and attached again.
    fn resize(self: *OpenGlSurfaceBackend, viewport: types.Viewport) !void {
        // A failure unwinds straight to shutdown, which detaches the session
        // before anything presents through the surface SDL may have dropped.
        const replaced = self.context.refreshPlatformSurface() catch |err| {
            diagnostics.logError("OpenGL surface refresh failed", err, null);
            return types.AppError.ResizeFailed;
        };
        if (!replaced) return self.session.resize(viewport);
        var diagnostic: maplibre.Diagnostic = .{};
        var completion = (self.session.handle.?).setOpenGlSurfaceTarget(.{
            .extent = render_target.extent(viewport),
            .context = self.context.descriptor(),
            .surface = self.context.surface(),
        }, &diagnostic) catch |err| {
            diagnostics.logError("OpenGL surface set target failed", err, &diagnostic);
            return types.AppError.ResizeFailed;
        };
        completion.deinit();
    }
};

fn checkGlError(procs: OpenGlCompositorProcs, operation: []const u8) !void {
    const gl_error = procs.GetError();
    if (gl_error == gl.NO_ERROR) return;
    std.debug.print("{s} failed with OpenGL error 0x{x}\n", .{ operation, gl_error });
    return types.AppError.BackendSetupFailed;
}

fn clearGlErrors(procs: OpenGlCompositorProcs) void {
    while (procs.GetError() != gl.NO_ERROR) {}
}

fn logSdlError(message: []const u8) void {
    const err = c.SDL_GetError();
    const details = if (err == null) "" else std.mem.span(err);
    std.debug.print("{s}: {s}\n", .{ message, details });
}
