// compositor.zig — OpenGL ES 3.0 texture compositor for zig-map owned-texture mode.
//
// Reads pixel data from a maplibre-native-c owned texture session and blits it
// to the SDL3 window using a minimal full-screen-triangle shader pipeline.
//
// All GL function pointers are resolved at runtime via SDL_GL_GetProcAddress so
// that no GL headers need to be installed at build time.  We target GLES 3.0
// (Mesa llvmpipe supports it on Ubuntu 24.04) in order to use the gl_VertexID
// full-screen triangle trick without a VBO.
//
// Pixel data from mln_texture_read_premultiplied_rgba8 is in top-to-bottom row
// order; the vertex shader flips Y so the image displays right-way up.

const std = @import("std");

const c = @import("../../c.zig").c;
const maplibre = @import("maplibre_native");
const types = @import("../../types.zig");

// ── Minimal OpenGL ES 3.0 type aliases (no GL headers required) ──────────────

const GLuint = c_uint;
const GLenum = c_uint;
const GLint = c_int;
const GLsizei = c_int;
const GLchar = u8;

// ── GL constants ─────────────────────────────────────────────────────────────

const GL_TEXTURE_2D: GLenum = 0x0DE1;
const GL_RGBA: GLenum = 0x1908;
const GL_UNSIGNED_BYTE: GLenum = 0x1401;
const GL_TEXTURE_MIN_FILTER: GLenum = 0x2801;
const GL_TEXTURE_MAG_FILTER: GLenum = 0x2800;
const GL_LINEAR: GLenum = 0x2601;
const GL_VERTEX_SHADER: GLenum = 0x8B31;
const GL_FRAGMENT_SHADER: GLenum = 0x8B30;
const GL_COMPILE_STATUS: GLenum = 0x8B81;
const GL_LINK_STATUS: GLenum = 0x8B82;
const GL_TRIANGLES: GLenum = 0x0004;
const GL_COLOR_BUFFER_BIT: GLenum = 0x00004000;
const GL_TEXTURE0: GLenum = 0x84C0;

// ── GL function pointer types ────────────────────────────────────────────────

const PFN = struct {
    const GenTextures = *const fn (GLsizei, [*]GLuint) callconv(.c) void;
    const DeleteTextures = *const fn (GLsizei, [*c]const GLuint) callconv(.c) void;
    const BindTexture = *const fn (GLenum, GLuint) callconv(.c) void;
    const TexImage2D = *const fn (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, ?*const anyopaque) callconv(.c) void;
    const TexParameteri = *const fn (GLenum, GLenum, GLint) callconv(.c) void;
    const CreateShader = *const fn (GLenum) callconv(.c) GLuint;
    const ShaderSource = *const fn (GLuint, GLsizei, [*c]const [*c]const GLchar, [*c]const GLint) callconv(.c) void;
    const CompileShader = *const fn (GLuint) callconv(.c) void;
    const GetShaderiv = *const fn (GLuint, GLenum, *GLint) callconv(.c) void;
    const DeleteShader = *const fn (GLuint) callconv(.c) void;
    const CreateProgram = *const fn () callconv(.c) GLuint;
    const AttachShader = *const fn (GLuint, GLuint) callconv(.c) void;
    const LinkProgram = *const fn (GLuint) callconv(.c) void;
    const GetProgramiv = *const fn (GLuint, GLenum, *GLint) callconv(.c) void;
    const DeleteProgram = *const fn (GLuint) callconv(.c) void;
    const UseProgram = *const fn (GLuint) callconv(.c) void;
    const GetUniformLocation = *const fn (GLuint, [*c]const GLchar) callconv(.c) GLint;
    const Uniform1i = *const fn (GLint, GLint) callconv(.c) void;
    const ActiveTexture = *const fn (GLenum) callconv(.c) void;
    const Clear = *const fn (GLenum) callconv(.c) void;
    const Viewport = *const fn (GLint, GLint, GLsizei, GLsizei) callconv(.c) void;
    const DrawArrays = *const fn (GLenum, GLint, GLsizei) callconv(.c) void;
};

// ── Shaders ───────────────────────────────────────────────────────────────────
//
// Full-screen triangle using gl_VertexID (GLSL ES 3.00, no VBO required).
// The pixel buffer is top-to-bottom; Y is flipped so the image is right-side up.

const vert_glsl: [:0]const u8 =
    \\#version 300 es
    \\out vec2 v_texcoord;
    \\void main() {
    \\    float x = float((gl_VertexID & 1) * 4 - 1);
    \\    float y = float((gl_VertexID & 2) * 2 - 1);
    \\    v_texcoord = vec2(x * 0.5 + 0.5, 0.5 - y * 0.5);
    \\    gl_Position = vec4(x, y, 0.0, 1.0);
    \\}
;

const frag_glsl: [:0]const u8 =
    \\#version 300 es
    \\precision mediump float;
    \\in vec2 v_texcoord;
    \\uniform sampler2D u_texture;
    \\out vec4 out_color;
    \\void main() {
    \\    out_color = texture(u_texture, v_texcoord);
    \\}
;

// ── Compositor ────────────────────────────────────────────────────────────────

pub const Compositor = struct {
    window: *c.SDL_Window,
    gl_ctx: c.SDL_GLContext,
    texture_id: GLuint,
    program: GLuint,
    texture_loc: GLint,
    pixel_buffer: []u8,

    // GL function pointers loaded at init time via SDL_GL_GetProcAddress.
    fn_genTextures: PFN.GenTextures,
    fn_deleteTextures: PFN.DeleteTextures,
    fn_bindTexture: PFN.BindTexture,
    fn_texImage2D: PFN.TexImage2D,
    fn_texParameteri: PFN.TexParameteri,
    fn_createShader: PFN.CreateShader,
    fn_shaderSource: PFN.ShaderSource,
    fn_compileShader: PFN.CompileShader,
    fn_getShaderiv: PFN.GetShaderiv,
    fn_deleteShader: PFN.DeleteShader,
    fn_createProgram: PFN.CreateProgram,
    fn_attachShader: PFN.AttachShader,
    fn_linkProgram: PFN.LinkProgram,
    fn_getProgramiv: PFN.GetProgramiv,
    fn_deleteProgram: PFN.DeleteProgram,
    fn_useProgram: PFN.UseProgram,
    fn_getUniformLocation: PFN.GetUniformLocation,
    fn_uniform1i: PFN.Uniform1i,
    fn_activeTexture: PFN.ActiveTexture,
    fn_clear: PFN.Clear,
    fn_viewport: PFN.Viewport,
    fn_drawArrays: PFN.DrawArrays,

    pub fn init(allocator: std.mem.Allocator, window: *c.SDL_Window) !Compositor {
        const gl_ctx = c.SDL_GL_CreateContext(window);
        if (gl_ctx == null) {
            std.debug.print("SDL_GL_CreateContext failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
            return types.AppError.BackendSetupFailed;
        }
        errdefer _ = c.SDL_GL_DestroyContext(gl_ctx);

        var self = Compositor{
            .window = window,
            .gl_ctx = gl_ctx,
            .texture_id = 0,
            .program = 0,
            .texture_loc = -1,
            .pixel_buffer = &.{},
            .fn_genTextures = try loadFn(PFN.GenTextures, "glGenTextures"),
            .fn_deleteTextures = try loadFn(PFN.DeleteTextures, "glDeleteTextures"),
            .fn_bindTexture = try loadFn(PFN.BindTexture, "glBindTexture"),
            .fn_texImage2D = try loadFn(PFN.TexImage2D, "glTexImage2D"),
            .fn_texParameteri = try loadFn(PFN.TexParameteri, "glTexParameteri"),
            .fn_createShader = try loadFn(PFN.CreateShader, "glCreateShader"),
            .fn_shaderSource = try loadFn(PFN.ShaderSource, "glShaderSource"),
            .fn_compileShader = try loadFn(PFN.CompileShader, "glCompileShader"),
            .fn_getShaderiv = try loadFn(PFN.GetShaderiv, "glGetShaderiv"),
            .fn_deleteShader = try loadFn(PFN.DeleteShader, "glDeleteShader"),
            .fn_createProgram = try loadFn(PFN.CreateProgram, "glCreateProgram"),
            .fn_attachShader = try loadFn(PFN.AttachShader, "glAttachShader"),
            .fn_linkProgram = try loadFn(PFN.LinkProgram, "glLinkProgram"),
            .fn_getProgramiv = try loadFn(PFN.GetProgramiv, "glGetProgramiv"),
            .fn_deleteProgram = try loadFn(PFN.DeleteProgram, "glDeleteProgram"),
            .fn_useProgram = try loadFn(PFN.UseProgram, "glUseProgram"),
            .fn_getUniformLocation = try loadFn(PFN.GetUniformLocation, "glGetUniformLocation"),
            .fn_uniform1i = try loadFn(PFN.Uniform1i, "glUniform1i"),
            .fn_activeTexture = try loadFn(PFN.ActiveTexture, "glActiveTexture"),
            .fn_clear = try loadFn(PFN.Clear, "glClear"),
            .fn_viewport = try loadFn(PFN.Viewport, "glViewport"),
            .fn_drawArrays = try loadFn(PFN.DrawArrays, "glDrawArrays"),
        };

        try self.buildProgram();
        errdefer self.fn_deleteProgram(self.program);

        self.fn_genTextures(1, @as([*]GLuint, @ptrCast(&self.texture_id)));

        self.pixel_buffer = try allocator.alloc(u8, 256 * 256 * 4);

        return self;
    }

    pub fn deinit(self: *Compositor, allocator: std.mem.Allocator) void {
        allocator.free(self.pixel_buffer);
        if (self.texture_id != 0) self.fn_deleteTextures(1, @ptrCast(&self.texture_id));
        if (self.program != 0) self.fn_deleteProgram(self.program);
        _ = c.SDL_GL_DestroyContext(self.gl_ctx);
        self.* = undefined;
    }

    /// Read pixels from an owned-texture session and blit them to the window.
    /// Returns true if a frame was drawn, false if the session has no new frame.
    pub fn presentTexture(
        self: *Compositor,
        allocator: std.mem.Allocator,
        texture: *maplibre.RenderSessionHandle,
        viewport: types.Viewport,
    ) !bool {
        // Probe the pixel buffer size without reading.
        const probe_info = texture.textureImageInfo() catch |err| switch (err) {
            error.InvalidState => return false,
            else => return types.AppError.BackendDrawFailed,
        };

        // Grow the pixel buffer if needed.
        if (probe_info.byte_length > self.pixel_buffer.len) {
            allocator.free(self.pixel_buffer);
            self.pixel_buffer = try allocator.alloc(u8, probe_info.byte_length);
        }

        // Read back the rendered pixels.
        const info = texture.readPremultipliedRgba8Into(self.pixel_buffer) catch
            return types.AppError.BackendDrawFailed;

        // Re-activate our display GL context (the headless EGL backend deactivated
        // all contexts after rendering, leaving no context current).
        if (!c.SDL_GL_MakeCurrent(self.window, self.gl_ctx)) {
            std.debug.print("SDL_GL_MakeCurrent failed: {s}\n", .{std.mem.span(c.SDL_GetError())});
            return types.AppError.BackendDrawFailed;
        }

        // Upload the pixels as a GL texture.
        self.fn_activeTexture(GL_TEXTURE0);
        self.fn_bindTexture(GL_TEXTURE_2D, self.texture_id);
        self.fn_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, @intCast(GL_LINEAR));
        self.fn_texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, @intCast(GL_LINEAR));
        self.fn_texImage2D(
            GL_TEXTURE_2D,
            0,
            @intCast(GL_RGBA),
            @intCast(info.width),
            @intCast(info.height),
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            self.pixel_buffer.ptr,
        );

        // Draw the full-screen triangle.
        self.fn_viewport(0, 0, @intCast(viewport.physical_width), @intCast(viewport.physical_height));
        self.fn_clear(GL_COLOR_BUFFER_BIT);
        self.fn_useProgram(self.program);
        self.fn_uniform1i(self.texture_loc, 0);
        self.fn_drawArrays(GL_TRIANGLES, 0, 3);

        _ = c.SDL_GL_SwapWindow(self.window);
        return true;
    }

    fn buildProgram(self: *Compositor) !void {
        const vert = try self.compileShader(GL_VERTEX_SHADER, vert_glsl);
        defer self.fn_deleteShader(vert);

        const frag = try self.compileShader(GL_FRAGMENT_SHADER, frag_glsl);
        defer self.fn_deleteShader(frag);

        const prog = self.fn_createProgram();
        self.fn_attachShader(prog, vert);
        self.fn_attachShader(prog, frag);
        self.fn_linkProgram(prog);

        var status: GLint = 0;
        self.fn_getProgramiv(prog, GL_LINK_STATUS, &status);
        if (status == 0) {
            std.debug.print("GL program link failed\n", .{});
            self.fn_deleteProgram(prog);
            return types.AppError.BackendSetupFailed;
        }

        self.program = prog;
        self.texture_loc = self.fn_getUniformLocation(prog, "u_texture");
    }

    fn compileShader(self: *Compositor, shader_type: GLenum, src: [:0]const u8) !GLuint {
        const shader = self.fn_createShader(shader_type);
        const src_ptr: [*c]const GLchar = src.ptr;
        self.fn_shaderSource(shader, 1, &src_ptr, null);
        self.fn_compileShader(shader);

        var status: GLint = 0;
        self.fn_getShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == 0) {
            std.debug.print("GL shader compile failed\n", .{});
            self.fn_deleteShader(shader);
            return types.AppError.BackendSetupFailed;
        }
        return shader;
    }
};

fn loadFn(comptime T: type, name: [:0]const u8) !T {
    const ptr = c.SDL_GL_GetProcAddress(name.ptr) orelse {
        std.debug.print("GL function not found: {s}\n", .{name});
        return types.AppError.BackendSetupFailed;
    };
    return @as(T, @ptrCast(@alignCast(ptr)));
}
