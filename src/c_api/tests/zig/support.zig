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

pub const OwnedTextureAttachContext = if (build_options.supports_vulkan) VulkanAttachContext else if (build_options.supports_metal) MetalAttachContext else struct {};
pub const OwnedTextureDescriptor = if (build_options.supports_vulkan) c.mln_vulkan_owned_texture_descriptor else if (build_options.supports_metal) c.mln_metal_owned_texture_descriptor else struct {};

pub fn ownedTextureDescriptor(context: *const OwnedTextureAttachContext) OwnedTextureDescriptor {
    var descriptor = defaultOwnedTextureDescriptor();
    configureOwnedTextureDescriptor(&descriptor, context);
    return descriptor;
}

pub fn defaultOwnedTextureDescriptor() OwnedTextureDescriptor {
    if (build_options.supports_vulkan) {
        return c.mln_vulkan_owned_texture_descriptor_default();
    } else if (build_options.supports_metal) {
        return c.mln_metal_owned_texture_descriptor_default();
    } else {
        unreachable;
    }
}

pub fn configureOwnedTextureDescriptor(descriptor: *OwnedTextureDescriptor, context: *const OwnedTextureAttachContext) void {
    if (build_options.supports_vulkan) {
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
    if (build_options.supports_vulkan) {
        return c.mln_vulkan_owned_texture_attach(map, descriptor, out_session);
    } else if (build_options.supports_metal) {
        return c.mln_metal_owned_texture_attach(map, descriptor, out_session);
    } else {
        unreachable;
    }
}

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
    loader: ?NativeDynamicLibrary = null,
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
        if (softwareVulkanLoaderPath()) |loader_path| {
            var loader = try NativeDynamicLibrary.openZ(@ptrCast(loader_path.ptr));
            errdefer loader.close();
            const get_instance_proc_addr = loader.lookup(vk.PFN_vkGetInstanceProcAddr, "vkGetInstanceProcAddr") orelse return error.VulkanLoaderSymbolMissing;
            const create_instance = try loadGlobal(vk.PFN_vkCreateInstance, get_instance_proc_addr, "vkCreateInstance");
            const get_device_proc_addr = loader.lookup(vk.PFN_vkGetDeviceProcAddr, "vkGetDeviceProcAddr");
            return .{
                .loader = loader,
                .get_instance_proc_addr = get_instance_proc_addr,
                .get_device_proc_addr = get_device_proc_addr,
                .create_instance = create_instance,
            };
        }

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

    fn deinit(self: *VulkanDispatch) void {
        if (self.loader) |*loader| loader.close();
        self.loader = null;
    }

    fn loadInstanceFunctions(self: *VulkanDispatch, instance: vk.VkInstance) void {
        if (self.loader == null) return;
        self.get_device_proc_addr = self.get_device_proc_addr orelse loadInstance(vk.PFN_vkGetDeviceProcAddr, self.get_instance_proc_addr, instance, "vkGetDeviceProcAddr") catch null;
        self.destroy_instance = loadInstance(vk.PFN_vkDestroyInstance, self.get_instance_proc_addr, instance, "vkDestroyInstance") catch null;
        self.enumerate_physical_devices = loadInstance(vk.PFN_vkEnumeratePhysicalDevices, self.get_instance_proc_addr, instance, "vkEnumeratePhysicalDevices") catch null;
        self.get_physical_device_queue_family_properties = loadInstance(vk.PFN_vkGetPhysicalDeviceQueueFamilyProperties, self.get_instance_proc_addr, instance, "vkGetPhysicalDeviceQueueFamilyProperties") catch null;
        self.get_physical_device_features = loadInstance(vk.PFN_vkGetPhysicalDeviceFeatures, self.get_instance_proc_addr, instance, "vkGetPhysicalDeviceFeatures") catch null;
        self.enumerate_device_extension_properties = loadInstance(vk.PFN_vkEnumerateDeviceExtensionProperties, self.get_instance_proc_addr, instance, "vkEnumerateDeviceExtensionProperties") catch null;
        self.create_device = loadInstance(vk.PFN_vkCreateDevice, self.get_instance_proc_addr, instance, "vkCreateDevice") catch null;
    }

    fn loadDeviceFunctions(self: *VulkanDispatch, device: vk.VkDevice) void {
        if (self.loader == null) return;
        self.destroy_device = loadDevice(vk.PFN_vkDestroyDevice, self.get_device_proc_addr, device, "vkDestroyDevice") catch null;
        self.device_wait_idle = loadDevice(vk.PFN_vkDeviceWaitIdle, self.get_device_proc_addr, device, "vkDeviceWaitIdle") catch null;
        self.get_device_queue = loadDevice(vk.PFN_vkGetDeviceQueue, self.get_device_proc_addr, device, "vkGetDeviceQueue") catch null;
    }
} else struct {};

const NativeDynamicLibrary = if (build_options.supports_vulkan) struct {
    handle: *anyopaque,

    fn openZ(path: [*:0]const u8) !NativeDynamicLibrary {
        if (builtin.os.tag == .windows) {
            const handle = LoadLibraryA(path) orelse return error.VulkanLoaderUnavailable;
            return .{ .handle = handle };
        }
        const handle = dlopen(path, 2) orelse return error.VulkanLoaderUnavailable;
        return .{ .handle = handle };
    }

    fn close(self: *NativeDynamicLibrary) void {
        if (builtin.os.tag == .windows) {
            _ = FreeLibrary(self.handle);
        } else {
            _ = dlclose(self.handle);
        }
    }

    fn lookup(self: *NativeDynamicLibrary, comptime T: type, name: [:0]const u8) ?std.meta.Child(T) {
        const symbol = if (builtin.os.tag == .windows)
            GetProcAddress(self.handle, name.ptr)
        else
            dlsym(self.handle, name.ptr);
        return @ptrCast(@alignCast(symbol orelse return null));
    }
} else struct {};

extern "kernel32" fn LoadLibraryA(lpLibFileName: [*:0]const u8) callconv(.winapi) ?*anyopaque;
extern "kernel32" fn GetProcAddress(hModule: *anyopaque, lpProcName: [*:0]const u8) callconv(.winapi) ?*anyopaque;
extern "kernel32" fn FreeLibrary(hLibModule: *anyopaque) callconv(.winapi) c_int;

extern "c" fn dlopen(filename: [*:0]const u8, flags: c_int) ?*anyopaque;
extern "c" fn dlsym(handle: *anyopaque, symbol: [*:0]const u8) ?*anyopaque;
extern "c" fn dlclose(handle: *anyopaque) c_int;

fn softwareVulkanLoaderPath() ?[]const u8 {
    if (!build_options.supports_vulkan) return null;
    const value = std.c.getenv("MLN_FFI_SOFTWARE_VULKAN_LOADER") orelse return null;
    const path = std.mem.span(value);
    return if (path.len == 0) null else path;
}

fn loadGlobal(comptime T: type, get_instance_proc_addr: if (build_options.supports_vulkan) vk.PFN_vkGetInstanceProcAddr else ?*anyopaque, name: [:0]const u8) !std.meta.Child(T) {
    if (!build_options.supports_vulkan) return error.VulkanLoaderSymbolMissing;
    const symbol = get_instance_proc_addr.?(null, name.ptr) orelse return error.VulkanLoaderSymbolMissing;
    return @ptrCast(symbol);
}

fn loadInstance(comptime T: type, get_instance_proc_addr: if (build_options.supports_vulkan) vk.PFN_vkGetInstanceProcAddr else ?*anyopaque, instance: if (build_options.supports_vulkan) vk.VkInstance else ?*anyopaque, name: [:0]const u8) !std.meta.Child(T) {
    if (!build_options.supports_vulkan) return error.VulkanLoaderSymbolMissing;
    const symbol = get_instance_proc_addr.?(instance, name.ptr) orelse return error.VulkanLoaderSymbolMissing;
    return @ptrCast(symbol);
}

fn loadDevice(comptime T: type, get_device_proc_addr: if (build_options.supports_vulkan) vk.PFN_vkGetDeviceProcAddr else ?*anyopaque, device: if (build_options.supports_vulkan) vk.VkDevice else ?*anyopaque, name: [:0]const u8) !std.meta.Child(T) {
    if (!build_options.supports_vulkan) return error.VulkanLoaderSymbolMissing;
    const symbol = get_device_proc_addr.?(device, name.ptr) orelse return error.VulkanLoaderSymbolMissing;
    return @ptrCast(symbol);
}

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
