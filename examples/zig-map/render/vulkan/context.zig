const std = @import("std");

const c = @import("../../c.zig").c;
const maplibre = @import("maplibre_native_ffi");
const types = @import("../../types.zig");
const util = @import("util.zig");

pub const Context = struct {
    allocator: std.mem.Allocator,
    instance: c.VkInstance,
    surface: c.VkSurfaceKHR,
    physical_device: c.VkPhysicalDevice,
    device: c.VkDevice,
    /// The one queue that the host and the render session both submit to.
    queue: c.VkQueue,
    /// Held around every call on `queue`. A core worker submits from its own
    /// thread, and Vulkan requires the calls on one queue to be externally
    /// synchronized, so the session takes it too, through its queue lock.
    queue_mutex: *c.SDL_Mutex,
    queue_family_index: u32,

    pub fn init(allocator: std.mem.Allocator, window: *c.SDL_Window) !Context {
        var self = Context{
            .allocator = allocator,
            .instance = null,
            .surface = util.nullHandle(c.VkSurfaceKHR),
            .physical_device = null,
            .device = null,
            .queue = null,
            .queue_mutex = c.SDL_CreateMutex() orelse return types.AppError.BackendSetupFailed,
            .queue_family_index = 0,
        };
        errdefer self.deinit();

        try self.createInstance();
        try util.expectSdl(c.SDL_Vulkan_CreateSurface(
            window,
            self.instance,
            null,
            &self.surface,
        ));
        try self.pickDevice();
        try self.createDevice();
        return self;
    }

    /// Call only once the session that took the queue lock has been
    /// destroyed.
    pub fn deinit(self: *Context) void {
        if (self.device != null) c.vkDestroyDevice(self.device, null);
        if (!util.isNullHandle(self.surface)) {
            c.SDL_Vulkan_DestroySurface(self.instance, self.surface, null);
        }
        if (self.instance != null) c.vkDestroyInstance(self.instance, null);
        c.SDL_DestroyMutex(self.queue_mutex);
    }

    /// Holds the queue for one call on it.
    pub fn lockQueue(self: *const Context) void {
        c.SDL_LockMutex(self.queue_mutex);
    }

    pub fn unlockQueue(self: *const Context) void {
        c.SDL_UnlockMutex(self.queue_mutex);
    }

    /// The session's lock on `queue`, which takes `queue_mutex`.
    pub fn queueLock(self: *const Context) maplibre.QueueLock {
        return .{ .context = self.queue_mutex, .lock = lockMutex, .unlock = unlockMutex };
    }

    fn lockMutex(mutex: ?*anyopaque) maplibre.Error!void {
        c.SDL_LockMutex(@ptrCast(mutex));
    }

    fn unlockMutex(mutex: ?*anyopaque) maplibre.Error!void {
        c.SDL_UnlockMutex(@ptrCast(mutex));
    }

    /// Waits for the device. Call only once the session submits nothing
    /// more.
    pub fn waitIdle(self: *Context) void {
        if (self.device == null) return;
        self.lockQueue();
        defer self.unlockQueue();
        _ = c.vkDeviceWaitIdle(self.device);
    }

    fn createInstance(self: *Context) !void {
        var sdl_extension_count: u32 = 0;
        const sdl_extensions = c.SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
        if (sdl_extensions == null or sdl_extension_count == 0) {
            return types.AppError.BackendSetupFailed;
        }
        const needs_portability = try self.hasInstanceExtension(c.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        var extensions = try self.allocator.alloc([*c]const u8, sdl_extension_count + @intFromBool(needs_portability));
        defer self.allocator.free(extensions);
        for (extensions[0..sdl_extension_count], 0..) |*extension, index| {
            extension.* = sdl_extensions[index];
        }
        if (needs_portability) {
            extensions[sdl_extension_count] = c.VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        }

        const app_info = c.VkApplicationInfo{
            .sType = c.VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = null,
            .pApplicationName = "zig-map",
            .applicationVersion = 1,
            .pEngineName = "zig-map",
            .engineVersion = 1,
            .apiVersion = c.VK_API_VERSION_1_0,
        };
        const create_info = c.VkInstanceCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = null,
            .flags = if (needs_portability) c.VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR else 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = null,
            .enabledExtensionCount = @intCast(extensions.len),
            .ppEnabledExtensionNames = extensions.ptr,
        };
        try util.expectVk(c.vkCreateInstance(&create_info, null, &self.instance));
    }

    fn hasInstanceExtension(self: *Context, name: [*c]const u8) !bool {
        var count: u32 = 0;
        try util.expectVk(c.vkEnumerateInstanceExtensionProperties(null, &count, null));
        const properties = try self.allocator.alloc(c.VkExtensionProperties, count);
        defer self.allocator.free(properties);
        try util.expectVk(c.vkEnumerateInstanceExtensionProperties(null, &count, properties.ptr));

        const expected = std.mem.span(name);
        for (properties[0..@intCast(count)]) |property| {
            if (std.mem.eql(u8, std.mem.span(@as([*:0]const u8, @ptrCast(&property.extensionName))), expected)) {
                return true;
            }
        }
        return false;
    }

    fn pickDevice(self: *Context) !void {
        var count: u32 = 0;
        try util.expectVk(c.vkEnumeratePhysicalDevices(self.instance, &count, null));
        if (count == 0) return types.AppError.BackendSetupFailed;
        const devices = try self.allocator.alloc(c.VkPhysicalDevice, count);
        defer self.allocator.free(devices);
        try util.expectVk(c.vkEnumeratePhysicalDevices(
            self.instance,
            &count,
            devices.ptr,
        ));

        for (devices) |device| {
            var family_count: u32 = 0;
            c.vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, null);
            const families = try self.allocator.alloc(
                c.VkQueueFamilyProperties,
                family_count,
            );
            defer self.allocator.free(families);
            c.vkGetPhysicalDeviceQueueFamilyProperties(
                device,
                &family_count,
                families.ptr,
            );
            for (families, 0..) |family, index| {
                if ((family.queueFlags & c.VK_QUEUE_GRAPHICS_BIT) == 0) continue;
                if (!c.SDL_Vulkan_GetPresentationSupport(
                    self.instance,
                    device,
                    @intCast(index),
                )) continue;
                self.physical_device = device;
                self.queue_family_index = @intCast(index);
                return;
            }
        }
        return types.AppError.BackendSetupFailed;
    }

    fn hasDeviceExtension(self: *Context, name: []const u8) !bool {
        var count: u32 = 0;
        try util.expectVk(c.vkEnumerateDeviceExtensionProperties(
            self.physical_device,
            null,
            &count,
            null,
        ));
        const properties = try self.allocator.alloc(c.VkExtensionProperties, count);
        defer self.allocator.free(properties);
        try util.expectVk(c.vkEnumerateDeviceExtensionProperties(
            self.physical_device,
            null,
            &count,
            properties.ptr,
        ));

        for (properties[0..@intCast(count)]) |property| {
            if (std.mem.eql(u8, std.mem.span(@as([*:0]const u8, @ptrCast(&property.extensionName))), name)) {
                return true;
            }
        }
        return false;
    }

    fn createDevice(self: *Context) !void {
        const priority: f32 = 1.0;
        const queue_info = c.VkDeviceQueueCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .queueFamilyIndex = self.queue_family_index,
            .queueCount = 1,
            .pQueuePriorities = &priority,
        };
        // A device that exposes the portability subset requires enabling it.
        // The name is spelled out because its constant lives behind
        // vulkan_beta.h.
        const extensions = [_][*:0]const u8{
            c.VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            "VK_KHR_portability_subset",
        };
        const extension_count: u32 =
            if (try self.hasDeviceExtension("VK_KHR_portability_subset")) 2 else 1;
        var features = std.mem.zeroes(c.VkPhysicalDeviceFeatures);
        c.vkGetPhysicalDeviceFeatures(self.physical_device, &features);
        const create_info = c.VkDeviceCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = null,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = null,
            .enabledExtensionCount = extension_count,
            .ppEnabledExtensionNames = &extensions,
            .pEnabledFeatures = &features,
        };
        try util.expectVk(c.vkCreateDevice(
            self.physical_device,
            &create_info,
            null,
            &self.device,
        ));
        c.vkGetDeviceQueue(self.device, self.queue_family_index, 0, &self.queue);
    }
};
