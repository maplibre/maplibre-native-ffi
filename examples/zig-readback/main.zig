const std = @import("std");
const builtin = @import("builtin");
const build_options = @import("build_options");
const maplibre = @import("maplibre_native");

extern "c" fn MTLCreateSystemDefaultDevice() ?*anyopaque;

const vk = if (build_options.supports_vulkan) @cImport({
    @cInclude("vulkan/vulkan.h");
}) else struct {};

const width = 512;
const height = 512;
const style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-readback",
    \\  "sources": {},
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
    \\  ]
    \\}
;

pub fn main(init_args: std.process.Init) !void {
    const allocator = init_args.gpa;
    var args = try std.process.Args.Iterator.initAllocator(init_args.minimal.args, allocator);
    defer args.deinit();
    _ = args.skip();
    const output_path = args.next() orelse "map.ppm";

    try maplibre.setAsyncLogSeverityMask(.none, null);
    defer maplibre.setAsyncLogSeverityMask(.default, null) catch {};
    try logAndValidateRenderBackend();

    var runtime = try maplibre.RuntimeHandle.create(allocator, .{ .cache_path = ":memory:" }, null);
    defer runtime.close() catch {};

    var map = try maplibre.MapHandle.create(&runtime, .{
        .width = width,
        .height = height,
        .scale_factor = 1.0,
        .mode = .continuous,
    });
    defer map.close() catch {};

    var texture_target = try OwnedTextureTarget.attach(&map, .{
        .extent = .{ .width = width, .height = height, .scale_factor = 1.0 },
    });
    defer texture_target.close() catch {};
    const texture = &texture_target.session;

    try setInitialCamera(&map);
    try map.setStyleJson(allocator, style_json);
    try renderTexture(&runtime, &map, texture);

    var image = try texture.readPremultipliedRgba8(allocator);
    defer image.deinit();

    try writePpm(init_args.io, allocator, output_path, image.data, image.info);
    std.debug.print("wrote {s} ({d}x{d})\n", .{ output_path, image.info.width, image.info.height });
}

fn logAndValidateRenderBackend() !void {
    const support = maplibre.supportedRenderBackends();
    std.debug.print("native render backends: {s}\n", .{renderBackendSupportLabel(support)});
    if (build_options.supports_metal and !support.metal) return error.NativeRenderBackendMismatch;
    if (build_options.supports_vulkan and !support.vulkan) return error.NativeRenderBackendMismatch;
}

fn renderBackendSupportLabel(support: maplibre.RenderBackendSupport) []const u8 {
    if (support.metal and support.vulkan) return "metal,vulkan";
    if (support.metal) return "metal";
    if (support.vulkan) return "vulkan";
    return "none";
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

        var session = if (build_options.supports_vulkan)
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

const VulkanAttachContext = if (build_options.supports_vulkan) struct {
    instance: vk.VkInstance,
    physical_device: vk.VkPhysicalDevice,
    device: vk.VkDevice,
    queue: vk.VkQueue,
    queue_family_index: u32,

    fn init() !VulkanAttachContext {
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
        try expectVk(vk.vkCreateInstance(&instance_info, null, &instance));
        errdefer vk.vkDestroyInstance(instance, null);

        var physical_device_count: u32 = 0;
        try expectVk(vk.vkEnumeratePhysicalDevices(instance, &physical_device_count, null));
        if (physical_device_count == 0) return error.NoVulkanPhysicalDevice;

        var physical_devices_buffer: [16]vk.VkPhysicalDevice = undefined;
        if (physical_device_count > physical_devices_buffer.len) physical_device_count = physical_devices_buffer.len;
        try expectVk(vk.vkEnumeratePhysicalDevices(instance, &physical_device_count, &physical_devices_buffer));

        for (physical_devices_buffer[0..physical_device_count]) |physical_device| {
            var queue_family_count: u32 = 0;
            vk.vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, null);
            if (queue_family_count == 0) continue;

            var queue_families_buffer: [32]vk.VkQueueFamilyProperties = undefined;
            if (queue_family_count > queue_families_buffer.len) queue_family_count = queue_families_buffer.len;
            vk.vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, &queue_families_buffer);

            for (queue_families_buffer[0..queue_family_count], 0..) |queue_family, index| {
                if ((queue_family.queueFlags & vk.VK_QUEUE_GRAPHICS_BIT) == 0 or queue_family.queueCount == 0) continue;

                var priority: f32 = 1.0;
                var queue_info = std.mem.zeroes(vk.VkDeviceQueueCreateInfo);
                queue_info.sType = vk.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_info.queueFamilyIndex = @intCast(index);
                queue_info.queueCount = 1;
                queue_info.pQueuePriorities = &priority;

                var supported_features = std.mem.zeroes(vk.VkPhysicalDeviceFeatures);
                vk.vkGetPhysicalDeviceFeatures(physical_device, &supported_features);
                var features = std.mem.zeroes(vk.VkPhysicalDeviceFeatures);
                features.samplerAnisotropy = supported_features.samplerAnisotropy;
                features.wideLines = supported_features.wideLines;

                const portability_subset_extensions = [_][*c]const u8{"VK_KHR_portability_subset"};
                const enabled_device_extensions = if (try hasDeviceExtension(physical_device, "VK_KHR_portability_subset"))
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
                if (vk.vkCreateDevice(physical_device, &device_info, null, &device) != vk.VK_SUCCESS) continue;

                var queue: vk.VkQueue = null;
                vk.vkGetDeviceQueue(device, @intCast(index), 0, &queue);
                return .{
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
        vk.vkDestroyDevice(self.device, null);
        vk.vkDestroyInstance(self.instance, null);
    }

    fn descriptor(self: *const VulkanAttachContext) maplibre.VulkanContextDescriptor {
        return .{
            .instance = .{ .ptr = @ptrCast(self.instance.?) },
            .physical_device = .{ .ptr = @ptrCast(self.physical_device.?) },
            .device = .{ .ptr = @ptrCast(self.device.?) },
            .graphics_queue = .{ .ptr = @ptrCast(self.queue.?) },
            .graphics_queue_family_index = self.queue_family_index,
        };
    }
} else struct {};

fn hasDeviceExtension(physical_device: if (build_options.supports_vulkan) vk.VkPhysicalDevice else ?*anyopaque, name: [*c]const u8) !bool {
    if (!build_options.supports_vulkan) return false;

    var count: u32 = 0;
    try expectVk(vk.vkEnumerateDeviceExtensionProperties(physical_device, null, &count, null));

    var properties_buffer: [256]vk.VkExtensionProperties = undefined;
    if (count > properties_buffer.len) count = properties_buffer.len;
    try expectVk(vk.vkEnumerateDeviceExtensionProperties(physical_device, null, &count, &properties_buffer));

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
    runtime: *maplibre.RuntimeHandle,
    map: *maplibre.MapHandle,
    texture: *maplibre.RenderSessionHandle,
) !void {
    const map_id = try map.id();
    for (0..10_000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent()) |event| {
            if (event.source_type != .map or event.source_id == null or !std.meta.eql(event.source_id.?, map_id)) continue;
            switch (event.event_type) {
                .map_loading_failed => return error.MapLoadingFailed,
                .map_render_error => return error.MapRenderFailed,
                else => {},
            }
        }

        texture.renderUpdate() catch |err| switch (err) {
            error.InvalidState => {
                std.Thread.yield() catch {};
                continue;
            },
            else => return err,
        };
        return;
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
