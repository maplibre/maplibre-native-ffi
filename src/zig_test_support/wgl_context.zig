const builtin = @import("builtin");
const std = @import("std");
const testing = std.testing;

const gl = if (builtin.os.tag == .windows) @import("gl") else struct {};

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
    extern "kernel32" fn GetProcAddress(hModule: HINSTANCE, lpProcName: LPCSTR) callconv(.winapi) ?*anyopaque;
    extern "kernel32" fn LoadLibraryA(lpLibFileName: LPCSTR) callconv(.winapi) ?HINSTANCE;
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

    fn getProcAddress(name: [*:0]const u8) ?gl.PROC {
        if (wglGetProcAddress(name)) |proc| return @ptrCast(proc);
        const module = GetModuleHandleA("opengl32.dll") orelse LoadLibraryA("opengl32.dll") orelse return null;
        return @ptrCast(GetProcAddress(module, name) orelse return null);
    }
} else struct {};

fn GlProc(comptime name: []const u8) type {
    if (builtin.os.tag != .windows) return void;
    return @TypeOf(@field(@as(gl.ProcTable, undefined), name));
}

fn glProcName(comptime command: []const u8) [:0]const u8 {
    return "gl" ++ command;
}

const Procs = if (builtin.os.tag == .windows) struct {
    BindTexture: GlProc("BindTexture"),
    DeleteTextures: GlProc("DeleteTextures"),
    GenTextures: GlProc("GenTextures"),
    GetError: GlProc("GetError"),
    GetTexImage: GlProc("GetTexImage"),
    ReadBuffer: GlProc("ReadBuffer"),
    ReadPixels: GlProc("ReadPixels"),
    TexImage2D: GlProc("TexImage2D"),
    TexParameteri: GlProc("TexParameteri"),

    fn init() !Procs {
        var procs: Procs = undefined;
        inline for (.{
            "BindTexture",
            "DeleteTextures",
            "GenTextures",
            "GetError",
            "GetTexImage",
            "ReadBuffer",
            "ReadPixels",
            "TexImage2D",
            "TexParameteri",
        }) |command| {
            @field(procs, command) = @ptrCast(wgl.getProcAddress(glProcName(command)) orelse return error.WglUnavailable);
        }
        return procs;
    }
} else struct {};

pub const Context = if (builtin.os.tag == .windows) struct {
    window: wgl.HWND,
    device_context: wgl.HDC,
    share_context: wgl.HGLRC,
    procs: Procs,

    pub fn init() !Context {
        return initWithSize(8, 8);
    }

    pub fn initWithSize(width: u32, height: u32) !Context {
        return initWithClassName("MaplibreNativeWglTest", width, height);
    }

    pub fn initWithClassName(class_name: [:0]const u8, width: u32, height: u32) !Context {
        const module = wgl.GetModuleHandleA(null) orelse return error.WglUnavailable;

        var window_class = std.mem.zeroes(wgl.WNDCLASSA);
        window_class.style = wgl.CS_OWNDC;
        window_class.lpfnWndProc = wgl.windowProc;
        window_class.hInstance = module;
        window_class.lpszClassName = class_name;
        _ = wgl.RegisterClassA(&window_class);

        // WGL bootstraps through a drawable HWND/HDC here: the pixel format is
        // selected on a window DC before the shared context exists, so these
        // tests use a tiny hidden window instead of a headless pbuffer.
        const window = wgl.CreateWindowExA(
            0,
            class_name,
            class_name,
            wgl.WS_OVERLAPPEDWINDOW,
            0,
            0,
            @intCast(width),
            @intCast(height),
            null,
            null,
            module,
            null,
        ) orelse return error.WglUnavailable;
        errdefer _ = wgl.DestroyWindow(window);

        const device_context = wgl.GetDC(window) orelse return error.WglUnavailable;
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
        if (pixel_format == 0) return error.WglUnavailable;
        if (wgl.SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor) == 0) return error.WglUnavailable;

        const share_context = wgl.wglCreateContext(device_context) orelse return error.WglUnavailable;
        errdefer _ = wgl.wglDeleteContext(share_context);
        if (wgl.wglMakeCurrent(device_context, share_context) == 0) return error.WglUnavailable;

        return .{
            .window = window,
            .device_context = device_context,
            .share_context = share_context,
            .procs = try Procs.init(),
        };
    }

    pub fn deinit(self: *Context) void {
        _ = wgl.wglMakeCurrent(null, null);
        _ = wgl.wglDeleteContext(self.share_context);
        _ = wgl.ReleaseDC(self.window, self.device_context);
        _ = wgl.DestroyWindow(self.window);
    }

    pub fn makeCurrent(self: *const Context) !void {
        if (wgl.wglMakeCurrent(self.device_context, self.share_context) == 0) return error.WglUnavailable;
    }

    pub fn deviceContextPointer(self: *const Context) *anyopaque {
        return @ptrCast(self.device_context);
    }

    pub fn shareContextPointer(self: *const Context) *anyopaque {
        return @ptrCast(self.share_context);
    }

    pub fn getProcAddressPointer() *anyopaque {
        return @ptrCast(@constCast(&wgl.wglGetProcAddress));
    }

    pub fn createRgbaTexture(self: *const Context, width: u32, height: u32) !gl.uint {
        try self.makeCurrent();

        var texture: gl.uint = 0;
        self.procs.GenTextures(1, @ptrCast(&texture));
        if (texture == 0) return error.WglUnavailable;
        errdefer self.procs.DeleteTextures(1, @ptrCast(&texture));

        self.procs.BindTexture(gl.TEXTURE_2D, texture);
        self.procs.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        self.procs.TexParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        self.procs.TexImage2D(
            gl.TEXTURE_2D,
            0,
            gl.RGBA8,
            @intCast(width),
            @intCast(height),
            0,
            gl.RGBA,
            gl.UNSIGNED_BYTE,
            null,
        );
        self.procs.BindTexture(gl.TEXTURE_2D, 0);
        try testing.expectEqual(@as(gl.@"enum", gl.NO_ERROR), self.procs.GetError());
        return texture;
    }

    pub fn destroyTexture(self: *const Context, texture: gl.uint) void {
        self.procs.DeleteTextures(1, @ptrCast(&texture));
    }

    pub fn readRgbaTexture(self: *const Context, texture: gl.uint, pixels: []u8) !void {
        try self.makeCurrent();
        self.procs.BindTexture(gl.TEXTURE_2D, texture);
        self.procs.GetTexImage(gl.TEXTURE_2D, 0, gl.RGBA, gl.UNSIGNED_BYTE, pixels.ptr);
        self.procs.BindTexture(gl.TEXTURE_2D, 0);
        try testing.expectEqual(@as(gl.@"enum", gl.NO_ERROR), self.procs.GetError());
    }

    pub fn readSurfaceRgba(self: *const Context, width: u32, height: u32, pixels: []u8) !void {
        try self.makeCurrent();
        self.procs.ReadBuffer(gl.FRONT);
        self.procs.ReadPixels(0, 0, @intCast(width), @intCast(height), gl.RGBA, gl.UNSIGNED_BYTE, pixels.ptr);
        try testing.expectEqual(@as(gl.@"enum", gl.NO_ERROR), self.procs.GetError());
    }
} else struct {};
