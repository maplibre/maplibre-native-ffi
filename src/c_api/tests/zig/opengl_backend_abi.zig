// Raw C ABI/backend coverage: OpenGL descriptors expose provider unions,
// texture object names, size fields, and output-handle cases that bindings
// should wrap more safely.

const build_options = @import("build_options");
const builtin = @import("builtin");
const std = @import("std");
const testing = @import("std").testing;
const support = @import("support.zig");
const common = @import("render_session_abi.zig");
const c = support.c;

const wgl = if (builtin.os.tag == .windows) struct {
    const BOOL = c_int;
    const BYTE = u8;
    const DWORD = u32;
    const INT = c_int;
    const LPCSTR = [*:0]const u8;
    const UINT = u32;
    const WORD = u16;
    const WPARAM = usize;
    const LPARAM = isize;
    const LRESULT = isize;

    const HDC = *opaque {};
    const HGLRC = *opaque {};
    const HINSTANCE = *opaque {};
    const HWND = *opaque {};

    const WNDPROC = ?*const fn (HWND, UINT, WPARAM, LPARAM) callconv(.winapi) LRESULT;

    const WNDCLASSA = extern struct {
        style: UINT,
        lpfnWndProc: WNDPROC,
        cbClsExtra: INT,
        cbWndExtra: INT,
        hInstance: ?HINSTANCE,
        hIcon: ?*opaque {},
        hCursor: ?*opaque {},
        hbrBackground: ?*opaque {},
        lpszMenuName: ?LPCSTR,
        lpszClassName: ?LPCSTR,
    };

    const PIXELFORMATDESCRIPTOR = extern struct {
        nSize: WORD,
        nVersion: WORD,
        dwFlags: DWORD,
        iPixelType: BYTE,
        cColorBits: BYTE,
        cRedBits: BYTE,
        cRedShift: BYTE,
        cGreenBits: BYTE,
        cGreenShift: BYTE,
        cBlueBits: BYTE,
        cBlueShift: BYTE,
        cAlphaBits: BYTE,
        cAlphaShift: BYTE,
        cAccumBits: BYTE,
        cAccumRedBits: BYTE,
        cAccumGreenBits: BYTE,
        cAccumBlueBits: BYTE,
        cAccumAlphaBits: BYTE,
        cDepthBits: BYTE,
        cStencilBits: BYTE,
        cAuxBuffers: BYTE,
        iLayerType: BYTE,
        bReserved: BYTE,
        dwLayerMask: DWORD,
        dwVisibleMask: DWORD,
        dwDamageMask: DWORD,
    };

    const CS_OWNDC = 0x0020;
    const PFD_DOUBLEBUFFER = 0x00000001;
    const PFD_DRAW_TO_WINDOW = 0x00000004;
    const PFD_SUPPORT_OPENGL = 0x00000020;
    const PFD_TYPE_RGBA = 0;
    const PFD_MAIN_PLANE = 0;
    const WS_OVERLAPPEDWINDOW = 0x00cf0000;

    extern "kernel32" fn GetModuleHandleA(lpModuleName: ?LPCSTR) callconv(.winapi) ?HINSTANCE;
    extern "user32" fn RegisterClassA(lpWndClass: *const WNDCLASSA) callconv(.winapi) u16;
    extern "user32" fn CreateWindowExA(
        dwExStyle: DWORD,
        lpClassName: LPCSTR,
        lpWindowName: LPCSTR,
        dwStyle: DWORD,
        x: INT,
        y: INT,
        nWidth: INT,
        nHeight: INT,
        hWndParent: ?HWND,
        hMenu: ?*opaque {},
        hInstance: ?HINSTANCE,
        lpParam: ?*anyopaque,
    ) callconv(.winapi) ?HWND;
    extern "user32" fn DefWindowProcA(hWnd: HWND, msg: UINT, wParam: WPARAM, lParam: LPARAM) callconv(.winapi) LRESULT;
    extern "user32" fn DestroyWindow(hWnd: HWND) callconv(.winapi) BOOL;
    extern "user32" fn GetDC(hWnd: HWND) callconv(.winapi) ?HDC;
    extern "user32" fn ReleaseDC(hWnd: HWND, hDC: HDC) callconv(.winapi) INT;
    extern "gdi32" fn ChoosePixelFormat(hdc: HDC, ppfd: *const PIXELFORMATDESCRIPTOR) callconv(.winapi) INT;
    extern "gdi32" fn SetPixelFormat(hdc: HDC, format: INT, ppfd: *const PIXELFORMATDESCRIPTOR) callconv(.winapi) BOOL;
    extern "opengl32" fn wglCreateContext(hdc: HDC) callconv(.winapi) ?HGLRC;
    extern "opengl32" fn wglDeleteContext(hglrc: HGLRC) callconv(.winapi) BOOL;
    extern "opengl32" fn wglGetProcAddress(name: LPCSTR) callconv(.winapi) ?*anyopaque;
    extern "opengl32" fn wglMakeCurrent(hdc: ?HDC, hglrc: ?HGLRC) callconv(.winapi) BOOL;

    fn windowProc(hWnd: HWND, msg: UINT, wParam: WPARAM, lParam: LPARAM) callconv(.winapi) LRESULT {
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
} else struct {};

const fake_handle: *anyopaque = @ptrFromInt(1);

fn configureContext(context: *c.mln_opengl_context_descriptor) void {
    if (builtin.os.tag == .windows) {
        context.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WGL;
        context.data.wgl.size = @sizeOf(c.mln_wgl_context_descriptor);
        context.data.wgl.device_context = fake_handle;
        context.data.wgl.share_context = fake_handle;
    } else {
        context.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_EGL;
        context.data.egl.size = @sizeOf(c.mln_egl_context_descriptor);
        context.data.egl.display = fake_handle;
        context.data.egl.config = fake_handle;
        context.data.egl.share_context = fake_handle;
    }
}

fn shrinkContext(context: *c.mln_opengl_context_descriptor) void {
    if (builtin.os.tag == .windows) {
        context.data.wgl.size = @sizeOf(c.mln_wgl_context_descriptor) - 1;
    } else {
        context.data.egl.size = @sizeOf(c.mln_egl_context_descriptor) - 1;
    }
}

fn clearRequiredContextHandle(context: *c.mln_opengl_context_descriptor) void {
    if (builtin.os.tag == .windows) {
        context.data.wgl.share_context = null;
    } else {
        context.data.egl.share_context = null;
    }
}

const OpenGLOwnedTexture = struct {
    pub const descriptor_size = @sizeOf(c.mln_opengl_owned_texture_descriptor);

    pub fn descriptor() c.mln_opengl_owned_texture_descriptor {
        var value = c.mln_opengl_owned_texture_descriptor_default();
        configureContext(&value.context);
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_opengl_owned_texture_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_opengl_owned_texture_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_opengl_owned_texture_descriptor) void {
        clearRequiredContextHandle(&descriptor_ptr.context);
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_opengl_owned_texture_descriptor) void {
        opengl_backend_abi.shrinkContext(&descriptor_ptr.context);
    }
};

const OpenGLSurface = struct {
    pub const descriptor_size = @sizeOf(c.mln_opengl_surface_descriptor);

    pub fn descriptor() c.mln_opengl_surface_descriptor {
        var value = c.mln_opengl_surface_descriptor_default();
        configureContext(&value.context);
        value.surface = fake_handle;
        return value;
    }

    pub fn attach(map: ?*c.mln_map, descriptor_ptr: ?*const c.mln_opengl_surface_descriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
        return c.mln_opengl_surface_attach(map, descriptor_ptr, out_session);
    }

    pub fn clearRequiredHandle(descriptor_ptr: *c.mln_opengl_surface_descriptor) void {
        descriptor_ptr.surface = null;
    }

    pub fn shrinkContext(descriptor_ptr: *c.mln_opengl_surface_descriptor) void {
        opengl_backend_abi.shrinkContext(&descriptor_ptr.context);
    }
};

const opengl_backend_abi = @This();

const WglTestContext = if (builtin.os.tag == .windows) struct {
    window: wgl.HWND,
    device_context: wgl.HDC,
    share_context: wgl.HGLRC,

    pub fn init() !WglTestContext {
        const class_name = "MaplibreNativeCAbiWglTest";
        const module = wgl.GetModuleHandleA(null);
        if (module == null) return error.SkipZigTest;

        var window_class = std.mem.zeroes(wgl.WNDCLASSA);
        window_class.style = wgl.CS_OWNDC;
        window_class.lpfnWndProc = wgl.windowProc;
        window_class.hInstance = module;
        window_class.lpszClassName = class_name;
        _ = wgl.RegisterClassA(&window_class);

        const window = wgl.CreateWindowExA(
            0,
            class_name,
            class_name,
            wgl.WS_OVERLAPPEDWINDOW,
            0,
            0,
            8,
            8,
            null,
            null,
            module,
            null,
        ) orelse return error.SkipZigTest;
        errdefer _ = wgl.DestroyWindow(window);

        const device_context = wgl.GetDC(window) orelse return error.SkipZigTest;
        errdefer _ = wgl.ReleaseDC(window, device_context);

        var pixel_format_descriptor = std.mem.zeroes(wgl.PIXELFORMATDESCRIPTOR);
        pixel_format_descriptor.nSize = @intCast(@sizeOf(wgl.PIXELFORMATDESCRIPTOR));
        pixel_format_descriptor.nVersion = 1;
        pixel_format_descriptor.dwFlags = wgl.PFD_DRAW_TO_WINDOW | wgl.PFD_SUPPORT_OPENGL | wgl.PFD_DOUBLEBUFFER;
        pixel_format_descriptor.iPixelType = wgl.PFD_TYPE_RGBA;
        pixel_format_descriptor.cColorBits = 32;
        pixel_format_descriptor.cDepthBits = 24;
        pixel_format_descriptor.cStencilBits = 8;
        pixel_format_descriptor.iLayerType = wgl.PFD_MAIN_PLANE;

        const pixel_format = wgl.ChoosePixelFormat(device_context, &pixel_format_descriptor);
        if (pixel_format == 0) return error.SkipZigTest;
        if (wgl.SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor) == 0) return error.SkipZigTest;

        const share_context = wgl.wglCreateContext(device_context) orelse return error.SkipZigTest;
        errdefer _ = wgl.wglDeleteContext(share_context);
        if (wgl.wglMakeCurrent(device_context, share_context) == 0) return error.SkipZigTest;

        return .{
            .window = window,
            .device_context = device_context,
            .share_context = share_context,
        };
    }

    pub fn deinit(self: *WglTestContext) void {
        _ = wgl.wglMakeCurrent(null, null);
        _ = wgl.wglDeleteContext(self.share_context);
        _ = wgl.ReleaseDC(self.window, self.device_context);
        _ = wgl.DestroyWindow(self.window);
    }

    pub fn descriptor(self: *const WglTestContext) c.mln_opengl_context_descriptor {
        return .{
            .size = @sizeOf(c.mln_opengl_context_descriptor),
            .platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WGL,
            .data = .{
                .wgl = .{
                    .size = @sizeOf(c.mln_wgl_context_descriptor),
                    .device_context = @ptrCast(self.device_context),
                    .share_context = @ptrCast(self.share_context),
                    .get_proc_address = @ptrCast(@constCast(&wgl.wglGetProcAddress)),
                },
            },
        };
    }
} else struct {};

test "OpenGL default descriptors initialize ABI sizes" {
    const owned = c.mln_opengl_owned_texture_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_owned_texture_descriptor)), owned.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), owned.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_context_descriptor)), owned.context.size);

    const borrowed = c.mln_opengl_borrowed_texture_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_borrowed_texture_descriptor)), borrowed.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), borrowed.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_context_descriptor)), borrowed.context.size);

    const surface = c.mln_opengl_surface_descriptor_default();
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_surface_descriptor)), surface.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_render_target_extent)), surface.extent.size);
    try testing.expectEqual(@as(u32, @sizeOf(c.mln_opengl_context_descriptor)), surface.context.size);
}

test "OpenGL provider mask matches OpenGL build platform" {
    const mask = c.mln_opengl_supported_context_provider_mask();
    if (!build_options.supports_opengl) {
        try testing.expectEqual(@as(u32, 0), mask);
    } else if (builtin.os.tag == .windows) {
        try testing.expect((mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL) != 0);
    } else {
        try testing.expect((mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL) != 0);
    }
}

test "OpenGL owned texture attach rejects unsafe raw inputs" {
    try common.expectAttachRejectsUnsafeInputs(OpenGLOwnedTexture);
}

test "OpenGL surface attach rejects unsafe raw inputs" {
    try common.expectAttachRejectsUnsafeInputs(OpenGLSurface);
}

test "OpenGL borrowed texture rejects unsafe raw descriptors" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_opengl_borrowed_texture_descriptor_default();
    configureContext(&descriptor.context);
    descriptor.texture = 1;
    descriptor.target = 0x0de1;

    var texture: ?*c.mln_render_session = null;
    var invalid_extent_descriptor = descriptor;
    invalid_extent_descriptor.extent.size = @sizeOf(c.mln_render_target_extent) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_opengl_borrowed_texture_attach(map, &invalid_extent_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    var invalid_context_descriptor = descriptor;
    invalid_context_descriptor.context.size = @sizeOf(c.mln_opengl_context_descriptor) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_opengl_borrowed_texture_attach(map, &invalid_context_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);

    var missing_texture_descriptor = descriptor;
    missing_texture_descriptor.texture = 0;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_opengl_borrowed_texture_attach(map, &missing_texture_descriptor, &texture));
    try testing.expectEqual(@as(?*c.mln_render_session, null), texture);
}

test "OpenGL WGL owned texture attaches through raw C ABI" {
    if (!build_options.supports_opengl or builtin.os.tag != .windows) return error.SkipZigTest;

    var wgl_context = try WglTestContext.init();
    defer wgl_context.deinit();

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    var descriptor = c.mln_opengl_owned_texture_descriptor_default();
    descriptor.extent.width = 256;
    descriptor.extent.height = 256;
    descriptor.extent.scale_factor = 1.0;
    descriptor.context = wgl_context.descriptor();

    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_opengl_owned_texture_attach(map, &descriptor, &session));
    const handle = session orelse return error.SessionAttachFailed;
    defer testing.expectEqual(c.MLN_STATUS_OK, c.mln_render_session_destroy(handle)) catch @panic("render session destroy failed");

    var image_info = c.mln_texture_image_info_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_texture_read_premultiplied_rgba8(handle, null, 0, &image_info));

    var frame = c.mln_opengl_owned_texture_frame{
        .size = @sizeOf(c.mln_opengl_owned_texture_frame),
        .generation = 0,
        .width = 0,
        .height = 0,
        .scale_factor = 0.0,
        .frame_id = 0,
        .texture = 0,
        .target = 0,
        .internal_format = 0,
        .format = 0,
        .type = 0,
    };
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_opengl_owned_texture_acquire_frame(handle, &frame));
    try testing.expectEqual(c.MLN_STATUS_INVALID_STATE, c.mln_opengl_owned_texture_release_frame(handle, &frame));
}
