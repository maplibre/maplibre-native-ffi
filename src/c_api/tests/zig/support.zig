const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const testing = std.testing;

pub const c = @cImport({
    @cInclude("maplibre_native_c.h");
});

const vk = if (build_options.supports_vulkan) @cImport({
    @cInclude("vulkan/vulkan.h");
}) else struct {};

const egl = if (build_options.supports_opengl and builtin.os.tag == .linux) @cImport({
    @cInclude("EGL/egl.h");
}) else struct {};

// Zig 0.16 cannot translate the Windows SDK WGL headers reliably with MSVC, so
// keep this to the small ABI subset needed to create a shared context.
const wgl = if (build_options.supports_opengl and builtin.os.tag == .windows) struct {
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

extern "c" fn MTLCreateSystemDefaultDevice() ?*anyopaque;

pub fn createRuntime() !*c.mln_runtime {
    var runtime: ?*c.mln_runtime = null;
    var options = c.mln_runtime_options_default();
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_create(&options, &runtime));
    return runtime orelse error.RuntimeCreateFailed;
}

pub fn createMap(runtime: *c.mln_runtime) !*c.mln_map {
    var map: ?*c.mln_map = null;
    var options = c.mln_map_options_default();
    options.width = 512;
    options.height = 512;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_create(runtime, &options, &map));
    return map orelse error.MapCreateFailed;
}

pub fn destroyRuntime(runtime: *c.mln_runtime) void {
    testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_destroy(runtime)) catch @panic("runtime destroy failed");
}

pub fn destroyMap(map: *c.mln_map) void {
    testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_destroy(map)) catch @panic("map destroy failed");
}

pub const OwnedTextureAttachContext = if (build_options.supports_opengl) OpenGLAttachContext else if (build_options.supports_vulkan) VulkanAttachContext else if (build_options.supports_metal) MetalAttachContext else struct {};
pub const OwnedTextureDescriptor = if (build_options.supports_opengl) c.mln_opengl_owned_texture_descriptor else if (build_options.supports_vulkan) c.mln_vulkan_owned_texture_descriptor else if (build_options.supports_metal) c.mln_metal_owned_texture_descriptor else struct {};

pub fn ownedTextureDescriptor(context: *const OwnedTextureAttachContext) OwnedTextureDescriptor {
    var descriptor = defaultOwnedTextureDescriptor();
    configureOwnedTextureDescriptor(&descriptor, context);
    return descriptor;
}

pub fn defaultOwnedTextureDescriptor() OwnedTextureDescriptor {
    if (build_options.supports_opengl) {
        return c.mln_opengl_owned_texture_descriptor_default();
    } else if (build_options.supports_vulkan) {
        return c.mln_vulkan_owned_texture_descriptor_default();
    } else if (build_options.supports_metal) {
        return c.mln_metal_owned_texture_descriptor_default();
    } else {
        unreachable;
    }
}

pub fn configureOwnedTextureDescriptor(descriptor: *OwnedTextureDescriptor, context: *const OwnedTextureAttachContext) void {
    if (build_options.supports_opengl) {
        descriptor.context = context.descriptor();
    } else if (build_options.supports_vulkan) {
        descriptor.context = context.descriptor();
    } else if (build_options.supports_metal) {
        descriptor.context = context.descriptor();
    }
}

pub fn attachOwnedTextureSession(map: *c.mln_map, descriptor: *const OwnedTextureDescriptor) !*c.mln_render_session {
    var session: ?*c.mln_render_session = null;
    try testing.expectEqual(c.MLN_STATUS_OK, callOwnedTextureAttach(map, descriptor, &session));
    return session orelse error.SessionAttachFailed;
}

pub fn callOwnedTextureAttach(map: ?*c.mln_map, descriptor: ?*const OwnedTextureDescriptor, out_session: ?*?*c.mln_render_session) c.mln_status {
    if (build_options.supports_opengl) {
        return c.mln_opengl_owned_texture_attach(map, descriptor, out_session);
    } else if (build_options.supports_vulkan) {
        return c.mln_vulkan_owned_texture_attach(map, descriptor, out_session);
    } else if (build_options.supports_metal) {
        return c.mln_metal_owned_texture_attach(map, descriptor, out_session);
    } else {
        unreachable;
    }
}

const OpenGLAttachContext = if (build_options.supports_opengl and builtin.os.tag == .windows) struct {
    window: wgl.HWND,
    device_context: wgl.HDC,
    share_context: wgl.HGLRC,

    pub fn init() !OpenGLAttachContext {
        const class_name = "MaplibreNativeCAbiSupportWgl";
        const module = wgl.GetModuleHandleA(null) orelse return error.SkipZigTest;

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

    pub fn deinit(self: *OpenGLAttachContext) void {
        _ = wgl.wglMakeCurrent(null, null);
        _ = wgl.wglDeleteContext(self.share_context);
        _ = wgl.ReleaseDC(self.window, self.device_context);
        _ = wgl.DestroyWindow(self.window);
    }

    pub fn descriptor(self: *const OpenGLAttachContext) c.mln_opengl_context_descriptor {
        return .{
            .size = @sizeOf(c.mln_opengl_context_descriptor),
            .platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WGL,
            .data = .{ .wgl = .{
                .size = @sizeOf(c.mln_wgl_context_descriptor),
                .device_context = @ptrCast(self.device_context),
                .share_context = @ptrCast(self.share_context),
                .get_proc_address = @ptrCast(@constCast(&wgl.wglGetProcAddress)),
            } },
        };
    }
} else if (build_options.supports_opengl and builtin.os.tag == .linux) struct {
    display: egl.EGLDisplay,
    config: egl.EGLConfig,
    surface: egl.EGLSurface,
    share_context: egl.EGLContext,

    pub fn init() !@This() {
        const display = egl.eglGetDisplay(egl.EGL_DEFAULT_DISPLAY);
        if (display == egl.EGL_NO_DISPLAY) return error.SkipZigTest;
        errdefer _ = egl.eglTerminate(display);

        var major: egl.EGLint = 0;
        var minor: egl.EGLint = 0;
        if (egl.eglInitialize(display, &major, &minor) == egl.EGL_FALSE) return error.SkipZigTest;
        if (egl.eglBindAPI(egl.EGL_OPENGL_ES_API) == egl.EGL_FALSE) return error.SkipZigTest;

        const config_attributes = [_]egl.EGLint{
            egl.EGL_SURFACE_TYPE,    egl.EGL_PBUFFER_BIT,
            egl.EGL_RENDERABLE_TYPE, egl.EGL_OPENGL_ES3_BIT,
            egl.EGL_RED_SIZE,        8,
            egl.EGL_GREEN_SIZE,      8,
            egl.EGL_BLUE_SIZE,       8,
            egl.EGL_ALPHA_SIZE,      8,
            egl.EGL_DEPTH_SIZE,      24,
            egl.EGL_STENCIL_SIZE,    8,
            egl.EGL_NONE,
        };
        var config: egl.EGLConfig = null;
        var config_count: egl.EGLint = 0;
        if (egl.eglChooseConfig(display, &config_attributes, &config, 1, &config_count) == egl.EGL_FALSE or
            config_count == 0 or config == null)
        {
            return error.SkipZigTest;
        }

        const context_attributes = [_]egl.EGLint{
            egl.EGL_CONTEXT_CLIENT_VERSION, 3,
            egl.EGL_NONE,
        };
        const share_context = egl.eglCreateContext(display, config, egl.EGL_NO_CONTEXT, &context_attributes);
        if (share_context == egl.EGL_NO_CONTEXT) return error.SkipZigTest;
        errdefer _ = egl.eglDestroyContext(display, share_context);

        const surface_attributes = [_]egl.EGLint{
            egl.EGL_WIDTH,  8,
            egl.EGL_HEIGHT, 8,
            egl.EGL_NONE,
        };
        const surface = egl.eglCreatePbufferSurface(display, config, &surface_attributes);
        if (surface == egl.EGL_NO_SURFACE) return error.SkipZigTest;
        errdefer _ = egl.eglDestroySurface(display, surface);

        if (egl.eglMakeCurrent(display, surface, surface, share_context) == egl.EGL_FALSE) return error.SkipZigTest;
        // TODO(linux): Validate this EGL helper on the Linux Mesa/llvmpipe
        // environment before depending on it for non-Windows OpenGL CI signal.
        return .{
            .display = display,
            .config = config,
            .surface = surface,
            .share_context = share_context,
        };
    }

    pub fn deinit(self: *@This()) void {
        _ = egl.eglMakeCurrent(self.display, egl.EGL_NO_SURFACE, egl.EGL_NO_SURFACE, egl.EGL_NO_CONTEXT);
        _ = egl.eglDestroySurface(self.display, self.surface);
        _ = egl.eglDestroyContext(self.display, self.share_context);
        _ = egl.eglTerminate(self.display);
    }

    pub fn descriptor(self: *const @This()) c.mln_opengl_context_descriptor {
        return .{
            .size = @sizeOf(c.mln_opengl_context_descriptor),
            .platform = c.MLN_OPENGL_CONTEXT_PLATFORM_EGL,
            .data = .{ .egl = .{
                .size = @sizeOf(c.mln_egl_context_descriptor),
                .display = @ptrCast(self.display.?),
                .config = @ptrCast(self.config.?),
                .share_context = @ptrCast(self.share_context.?),
                .get_proc_address = null,
            } },
        };
    }
} else struct {};

const MetalAttachContext = struct {
    device: *anyopaque,

    pub fn init() !MetalAttachContext {
        return .{ .device = MTLCreateSystemDefaultDevice() orelse return error.SkipZigTest };
    }

    pub fn deinit(_: *MetalAttachContext) void {}

    pub fn descriptor(self: *const MetalAttachContext) c.mln_metal_context_descriptor {
        return .{
            .size = @sizeOf(c.mln_metal_context_descriptor),
            .device = self.device,
        };
    }
};

const VulkanAttachContext = if (build_options.supports_vulkan) struct {
    dispatch: VulkanDispatch,
    instance: vk.VkInstance,
    physical_device: vk.VkPhysicalDevice,
    device: vk.VkDevice,
    queue: vk.VkQueue,
    queue_family_index: u32,

    pub fn init() !VulkanAttachContext {
        var dispatch = try VulkanDispatch.init();
        errdefer dispatch.deinit();

        var app_info = std.mem.zeroes(vk.VkApplicationInfo);
        app_info.sType = vk.VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "maplibre-native-c-api-tests";
        app_info.applicationVersion = 1;
        app_info.pEngineName = "maplibre-native-c-api-tests";
        app_info.engineVersion = 1;
        app_info.apiVersion = vk.VK_API_VERSION_1_1;

        var instance_info = std.mem.zeroes(vk.VkInstanceCreateInfo);
        instance_info.sType = vk.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_info.pApplicationInfo = &app_info;
        if (builtin.os.tag == .macos) {
            const instance_extensions = [_][*c]const u8{vk.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};
            instance_info.flags = vk.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            instance_info.enabledExtensionCount = instance_extensions.len;
            instance_info.ppEnabledExtensionNames = &instance_extensions;
        }

        var instance: vk.VkInstance = null;
        try expectVk(dispatch.create_instance.?(&instance_info, null, &instance));
        dispatch.loadInstanceFunctions(instance);
        errdefer dispatch.destroy_instance.?(instance, null);

        var physical_device_count: u32 = 0;
        try expectVk(dispatch.enumerate_physical_devices.?(instance, &physical_device_count, null));
        try testing.expect(physical_device_count != 0);

        const physical_devices = try testing.allocator.alloc(vk.VkPhysicalDevice, physical_device_count);
        defer testing.allocator.free(physical_devices);
        try expectVk(dispatch.enumerate_physical_devices.?(instance, &physical_device_count, physical_devices.ptr));

        for (physical_devices) |physical_device| {
            var queue_family_count: u32 = 0;
            dispatch.get_physical_device_queue_family_properties.?(physical_device, &queue_family_count, null);
            if (queue_family_count == 0) continue;

            const queue_families = try testing.allocator.alloc(vk.VkQueueFamilyProperties, queue_family_count);
            defer testing.allocator.free(queue_families);
            dispatch.get_physical_device_queue_family_properties.?(physical_device, &queue_family_count, queue_families.ptr);

            for (queue_families, 0..) |queue_family, index| {
                if ((queue_family.queueFlags & vk.VK_QUEUE_GRAPHICS_BIT) == 0 or queue_family.queueCount == 0) continue;

                var priority: f32 = 1.0;
                var queue_info = std.mem.zeroes(vk.VkDeviceQueueCreateInfo);
                queue_info.sType = vk.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info.queueFamilyIndex = @intCast(index);
                queue_info.queueCount = 1;
                queue_info.pQueuePriorities = &priority;

                var supported_features = std.mem.zeroes(vk.VkPhysicalDeviceFeatures);
                dispatch.get_physical_device_features.?(physical_device, &supported_features);
                var features = std.mem.zeroes(vk.VkPhysicalDeviceFeatures);
                features.samplerAnisotropy = supported_features.samplerAnisotropy;
                features.wideLines = supported_features.wideLines;

                const portability_subset_extensions = [_][*c]const u8{"VK_KHR_portability_subset"};
                const enabled_device_extensions = if (try hasDeviceExtension(&dispatch, physical_device, "VK_KHR_portability_subset"))
                    portability_subset_extensions[0..]
                else
                    portability_subset_extensions[0..0];

                var device_info = std.mem.zeroes(vk.VkDeviceCreateInfo);
                device_info.sType = vk.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                device_info.queueCreateInfoCount = 1;
                device_info.pQueueCreateInfos = &queue_info;
                device_info.enabledExtensionCount = @intCast(enabled_device_extensions.len);
                device_info.ppEnabledExtensionNames = enabled_device_extensions.ptr;
                device_info.pEnabledFeatures = &features;

                var device: vk.VkDevice = null;
                if (dispatch.create_device.?(physical_device, &device_info, null, &device) != vk.VK_SUCCESS) continue;
                dispatch.loadDeviceFunctions(device);

                var queue: vk.VkQueue = null;
                dispatch.get_device_queue.?(device, @intCast(index), 0, &queue);
                return .{
                    .dispatch = dispatch,
                    .instance = instance,
                    .physical_device = physical_device,
                    .device = device,
                    .queue = queue,
                    .queue_family_index = @intCast(index),
                };
            }
        }

        return error.NoUsableVulkanGraphicsQueue;
    }

    pub fn deinit(self: *VulkanAttachContext) void {
        if (self.device != null) {
            _ = self.dispatch.device_wait_idle.?(self.device);
            self.dispatch.destroy_device.?(self.device, null);
            self.device = null;
        }
        if (self.instance != null) {
            self.dispatch.destroy_instance.?(self.instance, null);
            self.instance = null;
        }
        self.dispatch.deinit();
    }

    pub fn descriptor(self: *const VulkanAttachContext) c.mln_vulkan_context_descriptor {
        return .{
            .size = @sizeOf(c.mln_vulkan_context_descriptor),
            .instance = @ptrCast(self.instance.?),
            .physical_device = @ptrCast(self.physical_device.?),
            .device = @ptrCast(self.device.?),
            .graphics_queue = @ptrCast(self.queue.?),
            .graphics_queue_family_index = self.queue_family_index,
            .get_instance_proc_addr = nativeFunctionPointer(self.dispatch.get_instance_proc_addr),
            .get_device_proc_addr = nativeFunctionPointer(self.dispatch.get_device_proc_addr),
        };
    }
} else struct {};

const VulkanDispatch = if (build_options.supports_vulkan) struct {
    get_instance_proc_addr: vk.PFN_vkGetInstanceProcAddr,
    get_device_proc_addr: vk.PFN_vkGetDeviceProcAddr,
    create_instance: vk.PFN_vkCreateInstance,
    destroy_instance: vk.PFN_vkDestroyInstance = null,
    enumerate_physical_devices: vk.PFN_vkEnumeratePhysicalDevices = null,
    get_physical_device_queue_family_properties: vk.PFN_vkGetPhysicalDeviceQueueFamilyProperties = null,
    get_physical_device_features: vk.PFN_vkGetPhysicalDeviceFeatures = null,
    enumerate_device_extension_properties: vk.PFN_vkEnumerateDeviceExtensionProperties = null,
    create_device: vk.PFN_vkCreateDevice = null,
    destroy_device: vk.PFN_vkDestroyDevice = null,
    device_wait_idle: vk.PFN_vkDeviceWaitIdle = null,
    get_device_queue: vk.PFN_vkGetDeviceQueue = null,

    fn init() !VulkanDispatch {
        return .{
            .get_instance_proc_addr = vk.vkGetInstanceProcAddr,
            .get_device_proc_addr = vk.vkGetDeviceProcAddr,
            .create_instance = vk.vkCreateInstance,
            .destroy_instance = vk.vkDestroyInstance,
            .enumerate_physical_devices = vk.vkEnumeratePhysicalDevices,
            .get_physical_device_queue_family_properties = vk.vkGetPhysicalDeviceQueueFamilyProperties,
            .get_physical_device_features = vk.vkGetPhysicalDeviceFeatures,
            .enumerate_device_extension_properties = vk.vkEnumerateDeviceExtensionProperties,
            .create_device = vk.vkCreateDevice,
            .destroy_device = vk.vkDestroyDevice,
            .device_wait_idle = vk.vkDeviceWaitIdle,
            .get_device_queue = vk.vkGetDeviceQueue,
        };
    }

    fn deinit(_: *VulkanDispatch) void {}

    fn loadInstanceFunctions(_: *VulkanDispatch, _: vk.VkInstance) void {}

    fn loadDeviceFunctions(_: *VulkanDispatch, _: vk.VkDevice) void {}
} else struct {};

fn nativeFunctionPointer(function: anytype) *anyopaque {
    return @ptrFromInt(@intFromPtr(function.?));
}

fn hasDeviceExtension(dispatch: *const VulkanDispatch, physical_device: if (build_options.supports_vulkan) vk.VkPhysicalDevice else ?*anyopaque, name: [*c]const u8) !bool {
    if (!build_options.supports_vulkan) return false;

    var count: u32 = 0;
    try expectVk(dispatch.enumerate_device_extension_properties.?(physical_device, null, &count, null));

    var properties_buffer: [256]vk.VkExtensionProperties = undefined;
    if (count > properties_buffer.len) count = properties_buffer.len;
    try expectVk(dispatch.enumerate_device_extension_properties.?(physical_device, null, &count, &properties_buffer));

    const expected = std.mem.span(name);
    for (properties_buffer[0..count]) |property| {
        if (std.mem.eql(u8, std.mem.span(@as([*:0]const u8, @ptrCast(&property.extensionName))), expected)) return true;
    }
    return false;
}

fn expectVk(result: if (build_options.supports_vulkan) vk.VkResult else i32) !void {
    if (build_options.supports_vulkan) try testing.expectEqual(vk.VK_SUCCESS, result);
}
