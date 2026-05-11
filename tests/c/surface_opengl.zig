const std = @import("std");
const build_options = @import("build_options");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

// ── EGL surface tests ────────────────────────────────────────────────────────

test "EGL surface unsupported backend validates arguments" {
    if (build_options.supports_opengl) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_egl_surface_descriptor_default();
    descriptor.display = @ptrFromInt(1);
    descriptor.config = @ptrFromInt(1);
    descriptor.context = @ptrFromInt(1);
    descriptor.surface = @ptrFromInt(1);

    // Non-null out_session is caught by argument validation before the
    // backend check, so the first call returns INVALID_ARGUMENT.
    var session: ?*c.mln_render_session = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &descriptor, &session));
    try testing.expect(session != null);

    // Null out_session reaches the backend check and returns UNSUPPORTED.
    session = null;
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_egl_surface_attach(map, &descriptor, &session));
    try testing.expectEqual(@as(?*c.mln_render_session, null), session);
}

test "EGL surface attach rejects invalid arguments" {
    if (!build_options.supports_opengl) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_egl_surface_descriptor_default();
    descriptor.display = @ptrFromInt(1);
    descriptor.config = @ptrFromInt(1);
    descriptor.context = @ptrFromInt(1);
    descriptor.surface = @ptrFromInt(1);

    var session: ?*c.mln_render_session = null;

    // Null map / descriptor / out_session
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(null, &descriptor, &session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, null, &session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &descriptor, null));

    // Non-null *session (already occupied)
    session = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &descriptor, &session));
    session = null;

    // Size field too small
    var small = descriptor;
    small.size = @sizeOf(c.mln_egl_surface_descriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &small, &session));

    // Zero width / height / bad scale_factor
    var bad = descriptor;
    bad.width = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.height = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.scale_factor = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &bad, &session));

    // Missing EGL handles
    bad = descriptor;
    bad.display = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.config = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.context = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.surface = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &bad, &session));

    // Default descriptor (all handles null) rejected
    const defaults = c.mln_egl_surface_descriptor_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_egl_surface_attach(map, &defaults, &session));
    try testing.expectEqual(@as(?*c.mln_render_session, null), session);
}

// ── WGL surface tests ────────────────────────────────────────────────────────

test "WGL surface unsupported backend validates arguments" {
    if (build_options.supports_opengl) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_wgl_surface_descriptor_default();
    descriptor.hdc = @ptrFromInt(1);
    descriptor.hglrc = @ptrFromInt(1);

    var session: ?*c.mln_render_session = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &descriptor, &session));
    try testing.expect(session != null);

    session = null;
    try testing.expectEqual(c.MLN_STATUS_UNSUPPORTED, c.mln_wgl_surface_attach(map, &descriptor, &session));
    try testing.expectEqual(@as(?*c.mln_render_session, null), session);
}

test "WGL surface attach rejects invalid arguments" {
    // WGL is only available on Windows OpenGL builds; skip everywhere else.
    if (!build_options.supports_opengl) return error.SkipZigTest;
    if (@import("builtin").os.tag != .windows) return error.SkipZigTest;

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_wgl_surface_descriptor_default();
    descriptor.hdc = @ptrFromInt(1);
    descriptor.hglrc = @ptrFromInt(1);

    var session: ?*c.mln_render_session = null;

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(null, &descriptor, &session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, null, &session));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &descriptor, null));

    session = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &descriptor, &session));
    session = null;

    var small = descriptor;
    small.size = @sizeOf(c.mln_wgl_surface_descriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &small, &session));

    var bad = descriptor;
    bad.width = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.height = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.scale_factor = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.hdc = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &bad, &session));

    bad = descriptor;
    bad.hglrc = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &bad, &session));

    const defaults = c.mln_wgl_surface_descriptor_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_wgl_surface_attach(map, &defaults, &session));
    try testing.expectEqual(@as(?*c.mln_render_session, null), session);
}
