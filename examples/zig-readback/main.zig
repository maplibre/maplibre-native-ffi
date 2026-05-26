const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const maplibre = @import("maplibre_native");

extern "c" fn MTLCreateSystemDefaultDevice() ?*anyopaque;

const vk = if (build_options.supports_vulkan) @cImport({
    @cInclude("vulkan/vulkan.h");
}) else struct {};

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

const width = 512;
const height = 512;
const style_url = "https://tiles.openfreemap.org/styles/bright";

pub fn main(init_args: std.process.Init) !void {
    const allocator = init_args.gpa;
    var args = try std.process.Args.Iterator.initAllocator(init_args.minimal.args, allocator);
    defer args.deinit();
    _ = args.skip();
    const output_path = args.next() orelse "map.ppm";

    try maplibre.setAsyncLogSeverityMask(.none, null);
    defer maplibre.setAsyncLogSeverityMask(.default, null) catch {};
    try logAndValidateRenderBackend();

    var diagnostic_store = maplibre.DiagnosticStore.init(allocator);
    defer diagnostic_store.deinit();

    var runtime = try maplibre.RuntimeHandle.create(allocator, .{ .cache_path = ":memory:" }, &diagnostic_store);
    defer runtime.close() catch {};

    var map = try maplibre.MapHandle.create(&runtime, .{
        .width = width,
        .height = height,
        .scale_factor = 1.0,
        .mode = .static,
    });
    defer map.close() catch {};

    var texture_target = try OwnedTextureTarget.attach(&map, .{
        .extent = .{ .width = width, .height = height, .scale_factor = 1.0 },
    });
    defer texture_target.close() catch {};
    const texture = &texture_target.session;

    try setInitialCamera(&map);
    try map.setStyleUrl(allocator, style_url);
    try map.requestStillImage();
    renderTexture(init_args.io, &runtime, &map, texture) catch |err| {
        logLatestDiagnostic(&diagnostic_store);
        return err;
    };

    var image = texture.readPremultipliedRgba8(allocator) catch |err| {
        logLatestDiagnostic(&diagnostic_store);
        return err;
    };
    defer image.deinit();

    try writePpm(init_args.io, allocator, output_path, image.data, image.info);
    std.debug.print("wrote {s} ({d}x{d})\n", .{ output_path, image.info.width, image.info.height });
}

fn logLatestDiagnostic(diagnostic_store: *const maplibre.DiagnosticStore) void {
    const diagnostic = diagnostic_store.get() orelse return;
    std.debug.print("native diagnostic", .{});
    if (diagnostic.raw_status) |raw_status| std.debug.print(" ({d})", .{raw_status});
    std.debug.print(": {s}\n", .{diagnostic.message});
}

fn logAndValidateRenderBackend() !void {
    const support = maplibre.supportedRenderBackends();
    std.debug.print("native render backends: {s}\n", .{renderBackendSupportLabel(support)});
    if (build_options.supports_metal and !support.metal) return error.NativeRenderBackendMismatch;
    if (build_options.supports_opengl and !support.opengl) return error.NativeRenderBackendMismatch;
    if (build_options.supports_vulkan and !support.vulkan) return error.NativeRenderBackendMismatch;
}

fn renderBackendSupportLabel(support: maplibre.RenderBackendSupport) []const u8 {
    if (support.metal and support.opengl and support.vulkan) return "metal,opengl,vulkan";
    if (support.metal and support.opengl) return "metal,opengl";
    if (support.metal and support.vulkan) return "metal,vulkan";
    if (support.opengl and support.vulkan) return "opengl,vulkan";
    if (support.metal) return "metal";
    if (support.opengl) return "opengl";
    if (support.vulkan) return "vulkan";
    return "none";
}

const OwnedTextureDescriptor = struct {
    extent: maplibre.RenderTargetExtent,
};

const OwnedTextureContext = if (build_options.supports_opengl) OpenGLAttachContext else if (build_options.supports_vulkan) VulkanAttachContext else if (build_options.supports_metal) struct {
    device: *anyopaque,

    fn init() !@This() {
        return .{ .device = MTLCreateSystemDefaultDevice() orelse return error.MetalDeviceUnavailable };
    }

    fn deinit(_: *@This()) void {}

    fn descriptor(self: *const @This()) maplibre.MetalContextDescriptor {
        return .{ .device = .{ .ptr = self.device } };
    }
} else struct {};

const OwnedTextureTarget = struct {
    context: OwnedTextureContext,
    session: maplibre.RenderSessionHandle,
    context_active: bool = true,

    fn attach(map: *maplibre.MapHandle, descriptor: OwnedTextureDescriptor) !OwnedTextureTarget {
        var context = try OwnedTextureContext.init();
        errdefer context.deinit();

        var session = if (build_options.supports_opengl)
            try maplibre.attachOpenGLOwnedTexture(map, .{
                .extent = descriptor.extent,
                .context = context.descriptor(),
            })
        else if (build_options.supports_vulkan)
            try maplibre.attachVulkanOwnedTexture(map, .{
                .extent = descriptor.extent,
                .context = context.descriptor(),
            })
        else if (build_options.supports_metal)
            try maplibre.attachMetalOwnedTexture(map, .{
                .extent = descriptor.extent,
                .context = context.descriptor(),
            })
        else
            unreachable;
        errdefer session.close() catch {};

        return .{ .context = context, .session = session };
    }

    fn close(self: *OwnedTextureTarget) !void {
        if (!self.context_active) return;
        defer {
            self.context.deinit();
            self.context_active = false;
        }
        try self.session.close();
    }
};

const OpenGLAttachContext = if (build_options.supports_opengl and builtin.os.tag == .windows) struct {
    window: wgl.HWND,
    device_context: wgl.HDC,
    share_context: wgl.HGLRC,

    fn init() !OpenGLAttachContext {
        const class_name = "MaplibreNativeZigReadbackWgl";
        const module = wgl.GetModuleHandleA(null) orelse return error.WglUnavailable;

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
        };
    }

    fn deinit(self: *OpenGLAttachContext) void {
        _ = wgl.wglMakeCurrent(null, null);
        _ = wgl.wglDeleteContext(self.share_context);
        _ = wgl.ReleaseDC(self.window, self.device_context);
        _ = wgl.DestroyWindow(self.window);
    }

    fn descriptor(self: *const OpenGLAttachContext) maplibre.OpenGLContextDescriptor {
        return .{ .wgl = .{
            .device_context = .{ .ptr = @ptrCast(self.device_context) },
            .share_context = .{ .ptr = @ptrCast(self.share_context) },
            .get_proc_address = .{ .ptr = @ptrCast(@constCast(&wgl.wglGetProcAddress)) },
        } };
    }
} else if (build_options.supports_opengl and builtin.os.tag == .linux) struct {
    fn init() !@This() {
        // TODO(linux): create an EGL readback context here after validating the
        // Mesa/llvmpipe config selection on a Linux workstation.
        return error.EglReadbackContextTodo;
    }

    fn deinit(_: *@This()) void {}

    fn descriptor(_: *const @This()) maplibre.OpenGLContextDescriptor {
        unreachable;
    }
} else struct {};

const VulkanAttachContext = if (build_options.supports_vulkan) struct {
    dispatch: VulkanDispatch,
    instance: vk.VkInstance,
    physical_device: vk.VkPhysicalDevice,
    device: vk.VkDevice,
    queue: vk.VkQueue,
    queue_family_index: u32,

    fn init() !VulkanAttachContext {
        var dispatch = try VulkanDispatch.init();
        errdefer dispatch.deinit();

        var app_info = std.mem.zeroes(vk.VkApplicationInfo);
        app_info.sType = vk.VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "maplibre-native-zig-readback";
        app_info.applicationVersion = 1;
        app_info.pEngineName = "maplibre-native-zig-readback";
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
        if (physical_device_count == 0) return error.NoVulkanPhysicalDevice;

        var physical_devices_buffer: [16]vk.VkPhysicalDevice = undefined;
        if (physical_device_count > physical_devices_buffer.len) physical_device_count = physical_devices_buffer.len;
        try expectVk(dispatch.enumerate_physical_devices.?(instance, &physical_device_count, &physical_devices_buffer));

        for (physical_devices_buffer[0..physical_device_count]) |physical_device| {
            var queue_family_count: u32 = 0;
            dispatch.get_physical_device_queue_family_properties.?(physical_device, &queue_family_count, null);
            if (queue_family_count == 0) continue;

            var queue_families_buffer: [32]vk.VkQueueFamilyProperties = undefined;
            if (queue_family_count > queue_families_buffer.len) queue_family_count = queue_families_buffer.len;
            dispatch.get_physical_device_queue_family_properties.?(physical_device, &queue_family_count, &queue_families_buffer);

            for (queue_families_buffer[0..queue_family_count], 0..) |queue_family, index| {
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

    fn deinit(self: *VulkanAttachContext) void {
        self.dispatch.destroy_device.?(self.device, null);
        self.dispatch.destroy_instance.?(self.instance, null);
        self.dispatch.deinit();
    }

    fn descriptor(self: *const VulkanAttachContext) maplibre.VulkanContextDescriptor {
        return .{
            .instance = .{ .ptr = @ptrCast(self.instance.?) },
            .physical_device = .{ .ptr = @ptrCast(self.physical_device.?) },
            .device = .{ .ptr = @ptrCast(self.device.?) },
            .graphics_queue = .{ .ptr = @ptrCast(self.queue.?) },
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
            .get_device_queue = vk.vkGetDeviceQueue,
        };
    }

    fn deinit(_: *VulkanDispatch) void {}

    fn loadInstanceFunctions(_: *VulkanDispatch, _: vk.VkInstance) void {}

    fn loadDeviceFunctions(_: *VulkanDispatch, _: vk.VkDevice) void {}
} else struct {};

fn nativeFunctionPointer(function: anytype) maplibre.NativePointer {
    return .{ .ptr = @ptrFromInt(@intFromPtr(function.?)) };
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
    if (build_options.supports_vulkan and result != vk.VK_SUCCESS) return error.VulkanCallFailed;
}

fn setInitialCamera(map: *maplibre.MapHandle) !void {
    try map.jumpTo(.{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    });
}

fn renderTexture(
    io: std.Io,
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
    texture: *maplibre.RenderSessionHandle,
) !void {
    const map_id = try map.id();
    var rendered_frame = false;
    const started = std.Io.Clock.awake.now(io);
    while (started.durationTo(std.Io.Clock.awake.now(io)).toNanoseconds() < 5 * std.time.ns_per_s) {
        try runtime.runOnce();
        while (try runtime.pollEvent()) |event| {
            if (event.source_type != .map or event.source_id == null or !std.meta.eql(event.source_id.?, map_id)) continue;
            switch (event.event_type) {
                .map_render_update_available => {
                    texture.renderUpdate() catch |err| switch (err) {
                        error.InvalidState => continue,
                        else => return err,
                    };
                    rendered_frame = true;
                },
                .map_still_image_finished => {
                    if (!rendered_frame) return error.StillImageFinishedWithoutFrame;
                    return;
                },
                .map_loading_failed => return error.MapLoadingFailed,
                .map_render_error => return error.MapRenderFailed,
                .map_still_image_failed => return error.StillImageFailed,
                else => {},
            }
        }

        try io.sleep(.fromMilliseconds(10), .awake);
    }
    return error.RenderTimedOut;
}

fn writePpm(
    io: std.Io,
    allocator: std.mem.Allocator,
    output_path: []const u8,
    rgba: []const u8,
    info: maplibre.TextureImageInfo,
) !void {
    const pixel_count = @as(usize, @intCast(info.width)) * @as(usize, @intCast(info.height));
    const rgb = try allocator.alloc(u8, pixel_count * 3);
    defer allocator.free(rgb);

    for (0..pixel_count) |index| {
        rgb[index * 3 + 0] = rgba[index * 4 + 0];
        rgb[index * 3 + 1] = rgba[index * 4 + 1];
        rgb[index * 3 + 2] = rgba[index * 4 + 2];
    }

    var file = try std.Io.Dir.cwd().createFile(io, output_path, .{});
    defer file.close(io);

    var header_buffer: [64]u8 = undefined;
    const header = try std.fmt.bufPrint(
        &header_buffer,
        "P6\n{d} {d}\n255\n",
        .{ info.width, info.height },
    );
    try file.writeStreamingAll(io, header);
    try file.writeStreamingAll(io, rgb);
}
