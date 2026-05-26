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
    const GLchar = u8;
    const GLboolean = u8;

    const COLOR_BUFFER_BIT: GLbitfield = 0x0000_4000;
    const COMPILE_STATUS: GLenum = 0x8b81;
    const CULL_FACE: GLenum = 0x0b44;
    const DEPTH_TEST: GLenum = 0x0b71;
    const FALSE: GLint = 0;
    const FRAMEBUFFER: GLenum = 0x8d40;
    const FRAGMENT_SHADER: GLenum = 0x8b30;
    const LINEAR: GLint = 0x2601;
    const LINK_STATUS: GLenum = 0x8b82;
    const MODELVIEW: GLenum = 0x1700;
    const NO_ERROR: GLenum = 0;
    const PROJECTION: GLenum = 0x1701;
    const QUADS: GLenum = 0x0007;
    const RGBA: GLenum = 0x1908;
    const SCISSOR_TEST: GLenum = 0x0c11;
    const TEXTURE0: GLenum = 0x84c0;
    const TEXTURE_2D: GLenum = 0x0de1;
    const TEXTURE_MAG_FILTER: GLenum = 0x2800;
    const TEXTURE_MIN_FILTER: GLenum = 0x2801;
    const TRIANGLES: GLenum = 0x0004;
    const UNSIGNED_BYTE: GLenum = 0x1401;
    const VERTEX_SHADER: GLenum = 0x8b31;

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

const gl_callconv = if (builtin.os.tag == .windows)
    std.builtin.CallingConvention.winapi
else
    std.builtin.CallingConvention.c;

const GLActiveTextureProc = *const fn (gl.GLenum) callconv(gl_callconv) void;
const GLAttachShaderProc = *const fn (gl.GLuint, gl.GLuint) callconv(gl_callconv) void;
const GLBindFramebufferProc = *const fn (gl.GLenum, gl.GLuint) callconv(gl_callconv) void;
const GLBindVertexArrayProc = *const fn (gl.GLuint) callconv(gl_callconv) void;
const GLCompileShaderProc = *const fn (gl.GLuint) callconv(gl_callconv) void;
const GLCreateProgramProc = *const fn () callconv(gl_callconv) gl.GLuint;
const GLCreateShaderProc = *const fn (gl.GLenum) callconv(gl_callconv) gl.GLuint;
const GLDeleteProgramProc = *const fn (gl.GLuint) callconv(gl_callconv) void;
const GLDeleteShaderProc = *const fn (gl.GLuint) callconv(gl_callconv) void;
const GLDeleteVertexArraysProc = *const fn (gl.GLsizei, *const gl.GLuint) callconv(gl_callconv) void;
const GLDrawArraysProc = *const fn (gl.GLenum, gl.GLint, gl.GLsizei) callconv(gl_callconv) void;
const GLGenVertexArraysProc = *const fn (gl.GLsizei, *gl.GLuint) callconv(gl_callconv) void;
const GLGetProgramInfoLogProc = *const fn (gl.GLuint, gl.GLsizei, *gl.GLsizei, [*]gl.GLchar) callconv(gl_callconv) void;
const GLGetProgramIvProc = *const fn (gl.GLuint, gl.GLenum, *gl.GLint) callconv(gl_callconv) void;
const GLGetShaderInfoLogProc = *const fn (gl.GLuint, gl.GLsizei, *gl.GLsizei, [*]gl.GLchar) callconv(gl_callconv) void;
const GLGetShaderIvProc = *const fn (gl.GLuint, gl.GLenum, *gl.GLint) callconv(gl_callconv) void;
const GLGetUniformLocationProc = *const fn (gl.GLuint, [*:0]const gl.GLchar) callconv(gl_callconv) gl.GLint;
const GLLinkProgramProc = *const fn (gl.GLuint) callconv(gl_callconv) void;
const GLShaderSourceProc = *const fn (gl.GLuint, gl.GLsizei, [*]const [*]const gl.GLchar, *const gl.GLint) callconv(gl_callconv) void;
const GLUniform1iProc = *const fn (gl.GLint, gl.GLint) callconv(gl_callconv) void;
const GLUseProgramProc = *const fn (gl.GLuint) callconv(gl_callconv) void;

const OpenGLCompositorProcs = struct {
    active_texture: GLActiveTextureProc,
    attach_shader: GLAttachShaderProc,
    bind_framebuffer: GLBindFramebufferProc,
    bind_vertex_array: GLBindVertexArrayProc,
    compile_shader: GLCompileShaderProc,
    create_program: GLCreateProgramProc,
    create_shader: GLCreateShaderProc,
    delete_program: GLDeleteProgramProc,
    delete_shader: GLDeleteShaderProc,
    delete_vertex_arrays: GLDeleteVertexArraysProc,
    draw_arrays: GLDrawArraysProc,
    gen_vertex_arrays: GLGenVertexArraysProc,
    get_program_info_log: GLGetProgramInfoLogProc,
    get_program_iv: GLGetProgramIvProc,
    get_shader_info_log: GLGetShaderInfoLogProc,
    get_shader_iv: GLGetShaderIvProc,
    get_uniform_location: GLGetUniformLocationProc,
    link_program: GLLinkProgramProc,
    shader_source: GLShaderSourceProc,
    uniform_1i: GLUniform1iProc,
    use_program: GLUseProgramProc,

    fn load() !OpenGLCompositorProcs {
        return .{
            .active_texture = try loadGlProc(GLActiveTextureProc, "glActiveTexture"),
            .attach_shader = try loadGlProc(GLAttachShaderProc, "glAttachShader"),
            .bind_framebuffer = try loadGlProc(GLBindFramebufferProc, "glBindFramebuffer"),
            .bind_vertex_array = try loadGlProc(GLBindVertexArrayProc, "glBindVertexArray"),
            .compile_shader = try loadGlProc(GLCompileShaderProc, "glCompileShader"),
            .create_program = try loadGlProc(GLCreateProgramProc, "glCreateProgram"),
            .create_shader = try loadGlProc(GLCreateShaderProc, "glCreateShader"),
            .delete_program = try loadGlProc(GLDeleteProgramProc, "glDeleteProgram"),
            .delete_shader = try loadGlProc(GLDeleteShaderProc, "glDeleteShader"),
            .delete_vertex_arrays = try loadGlProc(GLDeleteVertexArraysProc, "glDeleteVertexArrays"),
            .draw_arrays = try loadGlProc(GLDrawArraysProc, "glDrawArrays"),
            .gen_vertex_arrays = try loadGlProc(GLGenVertexArraysProc, "glGenVertexArrays"),
            .get_program_info_log = try loadGlProc(GLGetProgramInfoLogProc, "glGetProgramInfoLog"),
            .get_program_iv = try loadGlProc(GLGetProgramIvProc, "glGetProgramiv"),
            .get_shader_info_log = try loadGlProc(GLGetShaderInfoLogProc, "glGetShaderInfoLog"),
            .get_shader_iv = try loadGlProc(GLGetShaderIvProc, "glGetShaderiv"),
            .get_uniform_location = try loadGlProc(GLGetUniformLocationProc, "glGetUniformLocation"),
            .link_program = try loadGlProc(GLLinkProgramProc, "glLinkProgram"),
            .shader_source = try loadGlProc(GLShaderSourceProc, "glShaderSource"),
            .uniform_1i = try loadGlProc(GLUniform1iProc, "glUniform1i"),
            .use_program = try loadGlProc(GLUseProgramProc, "glUseProgram"),
        };
    }
};

const texture_vertex_shader =
    \\#version 130
    \\out vec2 out_uv;
    \\vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    \\vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
    \\void main() {
    \\  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
    \\  out_uv = uvs[gl_VertexID];
    \\}
;

const texture_fragment_shader =
    \\#version 130
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

fn createTextureProgram(procs: OpenGLCompositorProcs) !gl.GLuint {
    const vertex = try compileShader(procs, gl.VERTEX_SHADER, texture_vertex_shader, "texture vertex shader");
    defer procs.delete_shader(vertex);
    const fragment = try compileShader(procs, gl.FRAGMENT_SHADER, texture_fragment_shader, "texture fragment shader");
    defer procs.delete_shader(fragment);

    const program = procs.create_program();
    if (program == 0) return types.AppError.BackendSetupFailed;
    procs.attach_shader(program, vertex);
    procs.attach_shader(program, fragment);
    procs.link_program(program);
    var linked: gl.GLint = 0;
    procs.get_program_iv(program, gl.LINK_STATUS, &linked);
    if (linked == gl.FALSE) {
        logProgramInfoLog(procs, program, "OpenGL compositor program link failed");
        procs.delete_program(program);
        return types.AppError.BackendSetupFailed;
    }
    return program;
}

fn compileShader(
    procs: OpenGLCompositorProcs,
    kind: gl.GLenum,
    source: []const u8,
    name: []const u8,
) !gl.GLuint {
    const shader = procs.create_shader(kind);
    if (shader == 0) return types.AppError.BackendSetupFailed;
    errdefer procs.delete_shader(shader);

    const sources = [_][*]const gl.GLchar{source.ptr};
    const lengths = [_]gl.GLint{@intCast(source.len)};
    procs.shader_source(shader, 1, sources[0..].ptr, &lengths[0]);
    procs.compile_shader(shader);
    var compiled: gl.GLint = 0;
    procs.get_shader_iv(shader, gl.COMPILE_STATUS, &compiled);
    if (compiled == gl.FALSE) {
        logShaderInfoLog(procs, shader, name);
        return types.AppError.BackendSetupFailed;
    }
    return shader;
}

fn logShaderInfoLog(procs: OpenGLCompositorProcs, shader: gl.GLuint, name: []const u8) void {
    var buffer: [1024]gl.GLchar = undefined;
    var length: gl.GLsizei = 0;
    procs.get_shader_info_log(shader, @intCast(buffer.len), &length, buffer[0..].ptr);
    const log = buffer[0..@min(@as(usize, @intCast(length)), buffer.len)];
    std.debug.print("OpenGL compositor {s} compile failed: {s}\n", .{ name, log });
}

fn logProgramInfoLog(procs: OpenGLCompositorProcs, program: gl.GLuint, message: []const u8) void {
    var buffer: [1024]gl.GLchar = undefined;
    var length: gl.GLsizei = 0;
    procs.get_program_info_log(program, @intCast(buffer.len), &length, buffer[0..].ptr);
    const log = buffer[0..@min(@as(usize, @intCast(length)), buffer.len)];
    std.debug.print("{s}: {s}\n", .{ message, log });
}

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
    procs: OpenGLCompositorProcs,
    program: gl.GLuint,
    vertex_array: gl.GLuint,

    fn init(window: *c.SDL_Window, viewport: types.Viewport) !OpenGLTextureCompositor {
        var context = try OpenGLContext.init(window);
        errdefer context.deinit();
        const procs = try OpenGLCompositorProcs.load();
        const program = try createTextureProgram(procs);
        errdefer procs.delete_program(program);
        var vertex_array: gl.GLuint = 0;
        procs.gen_vertex_arrays(1, &vertex_array);
        if (vertex_array == 0) return types.AppError.BackendSetupFailed;
        errdefer if (vertex_array != 0) procs.delete_vertex_arrays(1, &vertex_array);
        procs.use_program(program);
        const sampler = procs.get_uniform_location(program, "map_texture");
        if (sampler >= 0) procs.uniform_1i(sampler, 0);
        procs.use_program(0);
        try checkGlError("initialize OpenGL texture compositor");
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
        gl.glFinish();
        if (self.vertex_array != 0) {
            self.procs.delete_vertex_arrays(1, &self.vertex_array);
            self.vertex_array = 0;
        }
        if (self.program != 0) {
            self.procs.delete_program(self.program);
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
        gl.glFinish();
    }

    fn drawTexture(self: *OpenGLTextureCompositor, texture: gl.GLuint) !bool {
        try self.context.makeCurrent();
        try self.drawTextureQuad(texture);
        try self.context.swapWindow();
        return true;
    }

    fn drawTextureQuad(self: *OpenGLTextureCompositor, texture: gl.GLuint) !void {
        clearGlErrors();
        self.procs.bind_framebuffer(gl.FRAMEBUFFER, 0);
        gl.glDisable(gl.CULL_FACE);
        gl.glDisable(gl.DEPTH_TEST);
        gl.glDisable(gl.SCISSOR_TEST);
        gl.glViewport(0, 0, @intCast(self.viewport.physical_width), @intCast(self.viewport.physical_height));
        gl.glClearColor(0.08, 0.09, 0.11, 1.0);
        gl.glClear(gl.COLOR_BUFFER_BIT);
        self.procs.use_program(self.program);
        self.procs.bind_vertex_array(self.vertex_array);
        self.procs.active_texture(gl.TEXTURE0);
        gl.glBindTexture(gl_texture_target, texture);
        gl.glTexParameteri(gl_texture_target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.glTexParameteri(gl_texture_target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        self.procs.draw_arrays(gl.TRIANGLES, 0, 3);
        gl.glBindTexture(gl_texture_target, 0);
        self.procs.bind_vertex_array(0);
        self.procs.use_program(0);
        try checkGlError("draw OpenGL texture");
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

fn clearGlErrors() void {
    while (gl.glGetError() != gl.NO_ERROR) {}
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
