const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const maplibre = @import("maplibre_native_ffi");

extern "c" fn MTLCreateSystemDefaultDevice() ?*anyopaque;

const vk = if (build_options.supports_vulkan) @import("vulkan") else struct {};

const supports_egl = build_options.supports_opengl and (builtin.os.tag == .linux or builtin.os.tag == .macos);
const uses_caller_driver = build_options.supports_opengl and !supports_egl;

const egl = if (supports_egl) @import("egl") else struct {};

const sdl = if (build_options.supports_opengl and builtin.os.tag == .windows) @import("sdl") else struct {};

const width = 512;
const height = 512;
const style_url = "https://tiles.openfreemap.org/styles/bright";
fn waitForFuture(future: *maplibre.Future(void), diagnostic_store: ?*maplibre.DiagnosticStore) !void {
    try future.wait(diagnostic_store);
}
fn waitForSessionFuture(session: *maplibre.RenderSessionHandle, future: *maplibre.Future(void), diagnostic_store: ?*maplibre.DiagnosticStore) !void {
    if (!uses_caller_driver) return waitForFuture(future, diagnostic_store);
    while (!try future.poll()) _ = try session.serviceDriverWork(0);
    try future.wait(diagnostic_store);
}

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
    defer if (runtime.close()) |future| {
        var teardown = future;
        _ = teardown.wait(null) catch {};
        teardown.deinit();
    } else |_| {};

    var map_future = try maplibre.MapHandle.create(&runtime, .{ .mode = .static });
    defer map_future.deinit();
    var map = try map_future.wait(&diagnostic_store);
    defer map.close() catch {};
    var event_mask = try map.setEventMask(.{
        .map_render_update_available = true,
        .map_still_image_finished = true,
        .map_still_image_failed = true,
        .map_loading_failed = true,
        .map_render_error = true,
    });
    event_mask.deinit();
    var resize = try map.resize(width, height, 1.0);
    resize.deinit();
    try setInitialCamera(&map);
    var style = try map.setStyleUrl(allocator, style_url);
    style.deinit();

    var barrier = try runtime.barrier();
    defer barrier.deinit();
    try waitForFuture(&barrier, &diagnostic_store);

    var context = try OwnedTextureContext.init();
    defer context.deinit();
    try renderWithDriver(
        init_args.io,
        allocator,
        &map,
        &context,
        output_path,
    );
}

/// Uses a native core worker except for WGL, whose device context stays on this
/// graphics thread.
fn renderWithDriver(
    io: std.Io,
    allocator: std.mem.Allocator,
    map: *maplibre.MapHandle,
    context: *OwnedTextureContext,
    output_path: []const u8,
) !void {
    var attachment = try attachOwnedTexture(context, map, .{
        .extent = .{ .width = width, .height = height, .scale_factor = 1.0 },
    });
    var attachment_needs_cleanup = true;
    errdefer if (attachment_needs_cleanup) {
        _ = attachment.session.abandon() catch {};
        attachment.session.destroy() catch {};
    };
    defer attachment.completion.deinit();
    try waitForSessionFuture(&attachment.session, &attachment.completion, null);

    var session = attachment.session;
    defer {
        var detach = session.detach() catch null;
        if (detach) |*completion| {
            defer completion.deinit();
            waitForSessionFuture(&session, completion, null) catch {
                _ = session.abandon() catch {};
            };
        } else {
            _ = session.abandon() catch {};
        }
        session.destroy() catch {};
    }
    attachment_needs_cleanup = false;

    var still_image = try map.requestStillImage();
    defer still_image.deinit();

    try session.requestFrame(.{ .token = 1, .if_needed = false });
    try waitForRenderedFrame(io, &session, &still_image, 1);

    const capabilities = try session.capabilities();
    if (capabilities.frame_acquisition) {
        var frame = try session.acquireFrame();
        var frame_owned = true;
        errdefer if (frame_owned) frame.release(.cpu_complete) catch {};
        _ = try frame.producerSync();
        try frame.release(.cpu_complete);
        frame_owned = false;
    }

    var readback = try session.readback(allocator);
    defer readback.deinit();
    if (uses_caller_driver) {
        while (!try readback.poll()) _ = try session.serviceDriverWork(0);
    }
    var image = try readback.wait(null);
    defer image.deinit();

    try writePpm(io, allocator, output_path, image.data, image.info);
    std.debug.print("wrote {s} ({d}x{d})\n", .{ output_path, image.info.width, image.info.height });
}

fn waitForRenderedFrame(
    io: std.Io,
    session: *maplibre.RenderSessionHandle,
    still_image: *maplibre.Future(void),
    token: u64,
) !void {
    var rendered = false;
    var demand_pending = true;
    for (0..10_000) |_| {
        if (uses_caller_driver) {
            _ = try session.serviceDriverWork(0);
        }
        var results = session.drainFrameResults() catch |err| {
            if (err != error.NotReady) return err;
            try io.sleep(.fromMilliseconds(1), .awake);
            continue;
        };
        defer results.release();
        for (0..try results.count()) |index| {
            const result = try results.get(index);
            if (result.token != token) continue;
            demand_pending = false;
            switch (result.disposition) {
                .rendered => rendered = true,
                .no_update, .size_pending, .target_not_ready => {},
                else => return error.FrameNotRendered,
            }
        }
        const still_completed = try still_image.poll();
        if (still_completed) {
            try still_image.wait(null);
            if (rendered) return;
        }
        if (!demand_pending) {
            try session.requestFrame(.{ .token = token, .if_needed = false });
            demand_pending = true;
        }
        try io.sleep(.fromMilliseconds(1), .awake);
    }
    return error.FrameResultTimedOut;
}

fn logAndValidateRenderBackend() !void {
    const support = maplibre.supportedRenderBackends();
    var support_label_buffer: [32]u8 = undefined;
    std.debug.print("native render backends: {s}\n", .{renderBackendSupportLabel(&support_label_buffer, support)});
    if (build_options.supports_metal and !support.metal) return error.NativeRenderBackendMismatch;
    if (build_options.supports_opengl and !support.opengl) return error.NativeRenderBackendMismatch;
    if (build_options.supports_vulkan and !support.vulkan) return error.NativeRenderBackendMismatch;
}

fn renderBackendSupportLabel(buffer: []u8, support: maplibre.RenderBackendSupport) []const u8 {
    var len: usize = 0;
    var has_backend = false;
    if (support.metal) appendBackendLabel(buffer, &len, &has_backend, "metal");
    if (support.opengl) appendBackendLabel(buffer, &len, &has_backend, "opengl");
    if (support.vulkan) appendBackendLabel(buffer, &len, &has_backend, "vulkan");
    if (!has_backend) return "none";
    return buffer[0..len];
}

fn appendBackendLabel(buffer: []u8, len: *usize, has_backend: *bool, label: []const u8) void {
    if (has_backend.*) {
        buffer[len.*] = ',';
        len.* += 1;
    }
    @memcpy(buffer[len.*..][0..label.len], label);
    len.* += label.len;
    has_backend.* = true;
}

const OwnedTextureDescriptor = struct {
    extent: maplibre.RenderTargetExtent,
};

const OwnedTextureContext = if (build_options.supports_vulkan) VulkanAttachContext else if (build_options.supports_metal) struct {
    device: *anyopaque,

    fn init() !@This() {
        return .{ .device = MTLCreateSystemDefaultDevice() orelse return error.MetalDeviceUnavailable };
    }

    fn deinit(_: *@This()) void {}

    fn descriptor(self: *const @This()) maplibre.MetalContextDescriptor {
        return .{ .device = maplibre.NativePointer.fromPtr(self.device) };
    }
} else if (build_options.supports_opengl) OpenGLAttachContext else struct {};

/// Supplies transferable state to a core worker or a WGL context to the caller
/// driver.
fn attachOwnedTexture(
    context: *OwnedTextureContext,
    map: *maplibre.MapHandle,
    descriptor: OwnedTextureDescriptor,
) !maplibre.RenderSessionAttachment {
    return if (build_options.supports_vulkan)
        try maplibre.attachVulkanOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        }, .{ .driver = .core_worker, .requested_texture_ring_depth = 1 })
    else if (build_options.supports_metal)
        try maplibre.attachMetalOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        }, .{ .driver = .core_worker, .requested_texture_ring_depth = 1 })
    else if (build_options.supports_opengl)
        try maplibre.attachOpenGLOwnedTexture(map, .{
            .extent = descriptor.extent,
            .context = context.descriptor(),
        }, .{
            .driver = if (supports_egl) .core_worker else .caller_graphics_thread,
            .requested_texture_ring_depth = 1,
        })
    else
        return error.RenderBackendUnavailable;
}

const OpenGLAttachContext = if (build_options.supports_opengl and builtin.os.tag == .windows) struct {
    window: *sdl.SDL_Window,
    context: sdl.SDL_GLContext,
    device_context: *anyopaque,

    fn init() !OpenGLAttachContext {
        // WGL contexts need a Win32 device context with a selected pixel
        // format, so this path uses a hidden SDL window rather than being
        // surfaceless like the EGL pbuffer path.
        if (!sdl.SDL_Init(sdl.SDL_INIT_VIDEO)) return error.WglUnavailable;
        errdefer sdl.SDL_Quit();

        const window = sdl.SDL_CreateWindow(
            "MapLibre Zig Readback WGL",
            8,
            8,
            sdl.SDL_WINDOW_OPENGL | sdl.SDL_WINDOW_HIDDEN,
        ) orelse return error.WglUnavailable;
        errdefer sdl.SDL_DestroyWindow(window);

        const context = sdl.SDL_GL_CreateContext(window) orelse return error.WglUnavailable;
        errdefer _ = sdl.SDL_GL_DestroyContext(context);
        if (!sdl.SDL_GL_MakeCurrent(window, context)) return error.WglUnavailable;

        const properties = sdl.SDL_GetWindowProperties(window);
        if (properties == 0) return error.WglUnavailable;
        const device_context = sdl.SDL_GetPointerProperty(
            properties,
            sdl.SDL_PROP_WINDOW_WIN32_HDC_POINTER,
            null,
        ) orelse return error.WglUnavailable;

        return .{
            .window = window,
            .context = context,
            .device_context = device_context,
        };
    }

    fn deinit(self: *OpenGLAttachContext) void {
        _ = sdl.SDL_GL_MakeCurrent(self.window, null);
        _ = sdl.SDL_GL_DestroyContext(self.context);
        sdl.SDL_DestroyWindow(self.window);
        sdl.SDL_Quit();
    }

    /// A WGL context is current on one thread at a time, so a thread must make
    /// the host context current before its work and release it after.
    fn descriptor(self: *const OpenGLAttachContext) maplibre.OpenGLContextDescriptor {
        return .{ .wgl = .{
            .device_context = maplibre.NativePointer.fromPtr(@ptrCast(self.device_context)),
            .share_context = maplibre.NativePointer.fromPtr(@ptrCast(self.context)),
            .get_proc_address = maplibre.NativePointer.fromPtr(@ptrCast(@constCast(&sdl.SDL_GL_GetProcAddress))),
        } };
    }
} else if (supports_egl) struct {
    display: egl.EGLDisplay,
    config: egl.EGLConfig,

    fn init() !@This() {
        const display = try initDisplay();
        errdefer _ = egl.eglTerminate(display);

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
            return error.EglUnavailable;
        }

        return .{
            .display = display,
            .config = config,
        };
    }

    fn deinit(self: *@This()) void {
        _ = egl.eglTerminate(self.display);
    }

    fn initDisplay() !egl.EGLDisplay {
        if (builtin.os.tag == .macos) {
            const display_attributes = [_]egl.EGLint{
                egl.EGL_PLATFORM_ANGLE_TYPE_ANGLE,        egl.EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
                egl.EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, egl.EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
                egl.EGL_NONE,
            };
            return initializeDisplay(egl.eglGetPlatformDisplayEXT(egl.EGL_PLATFORM_ANGLE_ANGLE, null, &display_attributes));
        }
        return initializeDisplay(egl.eglGetDisplay(egl.EGL_DEFAULT_DISPLAY));
    }

    fn initializeDisplay(display: egl.EGLDisplay) !egl.EGLDisplay {
        if (display == egl.EGL_NO_DISPLAY) return error.EglUnavailable;

        var major: egl.EGLint = 0;
        var minor: egl.EGLint = 0;
        if (egl.eglInitialize(display, &major, &minor) == egl.EGL_FALSE) return error.EglUnavailable;
        return display;
    }

    fn descriptor(self: *const @This()) maplibre.OpenGLContextDescriptor {
        return .{ .egl = .{
            .display = maplibre.NativePointer.fromPtr(@ptrCast(self.display.?)),
            .config = maplibre.NativePointer.fromPtr(@ptrCast(self.config.?)),
            .share_context = null,
            .client_api = .gles,
            .ownership = .dedicated,
            .get_proc_address = null,
        } };
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
            .instance = maplibre.NativePointer.fromPtr(@ptrCast(self.instance.?)),
            .physical_device = maplibre.NativePointer.fromPtr(@ptrCast(self.physical_device.?)),
            .device = maplibre.NativePointer.fromPtr(@ptrCast(self.device.?)),
            .graphics_queue = maplibre.NativePointer.fromPtr(@ptrCast(self.queue.?)),
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
    return maplibre.NativePointer.fromPtr(@ptrFromInt(@intFromPtr(function.?)));
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
    var completion = try map.updateCamera(.{ .camera = .{
        .center = .{ .latitude = 37.7749, .longitude = -122.4194 },
        .zoom = 13.0,
        .bearing = 12.0,
        .pitch = 30.0,
    } });
    completion.deinit();
}

fn writePpm(
    io: std.Io,
    allocator: std.mem.Allocator,
    output_path: []const u8,
    rgba: []const u8,
    info: maplibre.TextureImageInfo,
) !void {
    const image_width: usize = @intCast(info.width);
    const image_height: usize = @intCast(info.height);
    const stride: usize = @intCast(info.stride);
    const row_bytes = image_width * 4;
    const required_bytes = stride * image_height;
    if (stride < row_bytes or info.byte_length < required_bytes or rgba.len < required_bytes) return error.InvalidReadbackLayout;
    const rgb = try allocator.alloc(u8, image_width * image_height * 3);
    defer allocator.free(rgb);

    for (0..image_height) |row| {
        const source = rgba[row * stride ..][0..row_bytes];
        const destination = rgb[row * image_width * 3 ..][0 .. image_width * 3];
        for (0..image_width) |column| {
            destination[column * 3 + 0] = source[column * 4 + 0];
            destination[column * 3 + 1] = source[column * 4 + 1];
            destination[column * 3 + 2] = source[column * 4 + 2];
        }
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
