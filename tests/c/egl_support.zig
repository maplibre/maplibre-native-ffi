const build_options = @import("build_options");
const builtin = @import("builtin");

/// Holds the EGL display, context, and pbuffer surface created by
/// mln_test_egl_create. Layout must match mln_test_egl_context in
/// egl_support_linux.c.
pub const EglContext = extern struct {
    egl_lib: ?*anyopaque,
    display: ?*anyopaque,
    context: ?*anyopaque,
    surface: ?*anyopaque,
    pfn_destroy_context: ?*anyopaque,
    pfn_destroy_surface: ?*anyopaque,
    pfn_terminate: ?*anyopaque,
};

extern "c" fn mln_test_egl_create(width: u32, height: u32, out: *EglContext) bool;
extern "c" fn mln_test_egl_destroy(ctx: *EglContext) void;

/// Creates a real EGL pbuffer context of the given size.
/// Returns `error.SkipZigTest` on non-OpenGL builds or non-Linux platforms.
/// Returns `error.EglContextUnavailable` if EGL initialisation fails
/// (e.g. no suitable driver), allowing the test to be skipped gracefully.
pub fn create(width: u32, height: u32) !EglContext {
    if (!build_options.supports_opengl) return error.SkipZigTest;
    if (builtin.os.tag != .linux) return error.SkipZigTest;
    var ctx: EglContext = undefined;
    if (!mln_test_egl_create(width, height, &ctx)) {
        return error.EglContextUnavailable;
    }
    return ctx;
}

pub fn destroy(ctx: *EglContext) void {
    mln_test_egl_destroy(ctx);
}
