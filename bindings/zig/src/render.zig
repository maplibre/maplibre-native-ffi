const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map_module = @import("map.zig");
const native_temp = @import("native_temp.zig");
const status = @import("status.zig");
const values = @import("values.zig");

const NativeRenderSession = opaque {};
const MetalOwnedTextureFrameStorage = struct { raw: c.mln_metal_owned_texture_frame };
const VulkanOwnedTextureFrameStorage = struct { raw: c.mln_vulkan_owned_texture_frame };

const RenderSessionFrameState = struct {
    metal_frame: ?MetalOwnedTextureFrameStorage = null,
    metal_frame_active: bool = false,
    metal_frame_generation: u64 = 0,
    vulkan_frame: ?VulkanOwnedTextureFrameStorage = null,
    vulkan_frame_active: bool = false,
    vulkan_frame_generation: u64 = 0,
};

pub const NativePointer = struct {
    ptr: *anyopaque,
};

pub const RenderBackendSupport = struct {
    metal: bool,
    vulkan: bool,
};

pub fn supportedRenderBackends() RenderBackendSupport {
    const mask = c.mln_supported_render_backend_mask();
    return .{
        .metal = (mask & c.MLN_RENDER_BACKEND_FLAG_METAL) != 0,
        .vulkan = (mask & c.MLN_RENDER_BACKEND_FLAG_VULKAN) != 0,
    };
}

pub const RenderTargetExtent = struct {
    width: u32 = 512,
    height: u32 = 512,
    scale_factor: f64 = 1.0,
};

pub const MetalContextDescriptor = struct {
    device: ?NativePointer = null,
};

pub const VulkanContextDescriptor = struct {
    instance: NativePointer,
    physical_device: NativePointer,
    device: NativePointer,
    graphics_queue: NativePointer,
    graphics_queue_family_index: u32,
};

pub const MetalOwnedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: MetalContextDescriptor,
};

pub const MetalBorrowedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    texture: NativePointer,
};

pub const VulkanOwnedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: VulkanContextDescriptor,
};

pub const VulkanBorrowedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: VulkanContextDescriptor,
    image: NativePointer,
    image_view: NativePointer,
    format: u32,
    initial_layout: u32,
    final_layout: u32,
};

pub const MetalSurfaceDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: MetalContextDescriptor = .{},
    layer: NativePointer,
};

pub const VulkanSurfaceDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: VulkanContextDescriptor,
    surface: NativePointer,
};

pub const TextureImageInfo = struct {
    width: u32,
    height: u32,
    stride: u32,
    byte_length: usize,
};

pub const FeatureStateSelector = struct {
    source_id: []const u8,
    source_layer_id: ?[]const u8 = null,
    feature_id: ?[]const u8 = null,
    state_key: ?[]const u8 = null,
};

pub const ScreenBox = struct {
    min: values.ScreenPoint,
    max: values.ScreenPoint,
};

pub const RenderedQueryGeometry = union(enum) {
    point: values.ScreenPoint,
    box: ScreenBox,
    line_string: []const values.ScreenPoint,
};

pub const RenderedFeatureQueryOptions = struct {
    layer_ids: ?[]const []const u8 = null,
    filter: ?values.JsonValue = null,
};

pub const SourceFeatureQueryOptions = struct {
    source_layer_ids: ?[]const []const u8 = null,
    filter: ?values.JsonValue = null,
};

pub const OwnedFeature = struct {
    allocator: std.mem.Allocator,
    geometry: values.OwnedGeometry,
    properties: []values.OwnedJsonMember,
    identifier: values.FeatureIdentifier,

    pub fn deinit(self: *OwnedFeature) void {
        self.geometry.deinit(self.allocator);
        for (self.properties) |*property| {
            self.allocator.free(property.key);
            property.value.deinit(self.allocator);
        }
        self.allocator.free(self.properties);
        if (self.identifier == .string) self.allocator.free(self.identifier.string);
        self.properties = &.{};
        self.identifier = .null;
    }
};

pub const QueriedFeature = struct {
    feature: OwnedFeature,
    source_id: ?[]const u8,
    source_layer_id: ?[]const u8,
    state: ?values.OwnedJsonValue,

    pub fn deinit(self: *QueriedFeature) void {
        const allocator = self.feature.allocator;
        self.feature.deinit();
        if (self.source_id) |source_id| allocator.free(source_id);
        if (self.source_layer_id) |source_layer_id| allocator.free(source_layer_id);
        if (self.state) |*state_value| state_value.deinit(allocator);
        self.source_id = null;
        self.source_layer_id = null;
        self.state = null;
    }
};

pub const FeatureQueryResult = struct {
    allocator: std.mem.Allocator,
    features: []QueriedFeature,

    pub fn deinit(self: *FeatureQueryResult) void {
        for (self.features) |*feature| feature.deinit();
        self.allocator.free(self.features);
        self.features = &.{};
    }
};

pub const OwnedFeatureCollection = struct {
    allocator: std.mem.Allocator,
    features: []OwnedFeature,

    pub fn deinit(self: *OwnedFeatureCollection) void {
        for (self.features) |*feature| feature.deinit();
        self.allocator.free(self.features);
        self.features = &.{};
    }
};

pub const FeatureExtensionResult = union(enum) {
    value: values.OwnedJsonValue,
    feature_collection: OwnedFeatureCollection,

    pub fn deinit(self: *FeatureExtensionResult, allocator: std.mem.Allocator) void {
        switch (self.*) {
            .value => |*value| value.deinit(allocator),
            .feature_collection => |*collection| collection.deinit(),
        }
        self.* = .{ .value = .null };
    }
};

pub const MetalOwnedTextureFrameInfo = struct {
    generation: u64,
    width: u32,
    height: u32,
    scale_factor: f64,
    texture: NativePointer,
    device: NativePointer,
    pixel_format: u64,
};

pub const VulkanOwnedTextureFrameInfo = struct {
    generation: u64,
    width: u32,
    height: u32,
    scale_factor: f64,
    image: NativePointer,
    image_view: NativePointer,
    device: NativePointer,
    format: u32,
    layout: u32,
};

pub const OwnedImage = struct {
    allocator: std.mem.Allocator,
    info: TextureImageInfo,
    data: []u8,

    pub fn deinit(self: *OwnedImage) void {
        self.allocator.free(self.data);
        self.data = &.{};
        self.info = .{ .width = 0, .height = 0, .stride = 0, .byte_length = 0 };
    }
};

pub const RenderSessionHandle = struct {
    native: ?*NativeRenderSession,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: ?*RenderSessionFrameState,

    pub fn resize(self: *RenderSessionHandle, extent: RenderTargetExtent) status.Error!void {
        try ensureNoActiveOwnedFrame(self);
        try status.checkStatus(
            c.mln_render_session_resize(try native(self), extent.width, extent.height, extent.scale_factor),
            self.diagnostic_store,
        );
    }

    pub fn renderUpdate(self: *RenderSessionHandle) status.Error!void {
        try ensureNoActiveOwnedFrame(self);
        try status.checkStatus(c.mln_render_session_render_update(try native(self)), self.diagnostic_store);
    }

    pub fn detach(self: *RenderSessionHandle) status.Error!void {
        try ensureNoActiveOwnedFrame(self);
        try status.checkStatus(c.mln_render_session_detach(try native(self)), self.diagnostic_store);
    }

    pub fn reduceMemoryUse(self: *RenderSessionHandle) status.Error!void {
        try status.checkStatus(c.mln_render_session_reduce_memory_use(try native(self)), self.diagnostic_store);
    }

    pub fn clearData(self: *RenderSessionHandle) status.Error!void {
        try status.checkStatus(c.mln_render_session_clear_data(try native(self)), self.diagnostic_store);
    }

    pub fn dumpDebugLogs(self: *RenderSessionHandle) status.Error!void {
        try status.checkStatus(c.mln_render_session_dump_debug_logs(try native(self)), self.diagnostic_store);
    }

    pub fn setFeatureState(
        self: *RenderSessionHandle,
        allocator: std.mem.Allocator,
        selector: FeatureStateSelector,
        feature_state: values.JsonValue,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_selector = try featureStateSelectorToNative(&temp, selector);
        const raw_state = try temp.jsonValue(feature_state);
        try status.checkStatus(
            c.mln_render_session_set_feature_state(try native(self), &raw_selector, raw_state),
            self.diagnostic_store,
        );
    }

    pub fn getFeatureState(
        self: *RenderSessionHandle,
        allocator: std.mem.Allocator,
        selector: FeatureStateSelector,
    ) status.Error!values.OwnedJsonValue {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_selector = try featureStateSelectorToNative(&temp, selector);
        var snapshot: ?*c.mln_json_snapshot = null;
        try status.checkStatus(
            c.mln_render_session_get_feature_state(try native(self), &raw_selector, &snapshot),
            self.diagnostic_store,
        );
        defer c.mln_json_snapshot_destroy(snapshot);
        var raw: ?*const c.mln_json_value = null;
        try status.checkStatus(c.mln_json_snapshot_get(snapshot.?, &raw), self.diagnostic_store);
        return try values.ownedJsonValueFromNative(allocator, raw.?);
    }

    pub fn removeFeatureState(
        self: *RenderSessionHandle,
        allocator: std.mem.Allocator,
        selector: FeatureStateSelector,
    ) status.Error!void {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_selector = try featureStateSelectorToNative(&temp, selector);
        try status.checkStatus(
            c.mln_render_session_remove_feature_state(try native(self), &raw_selector),
            self.diagnostic_store,
        );
    }

    pub fn queryRenderedFeatures(
        self: *RenderSessionHandle,
        allocator: std.mem.Allocator,
        geometry: RenderedQueryGeometry,
        options: ?RenderedFeatureQueryOptions,
    ) status.Error!FeatureQueryResult {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_geometry = try renderedQueryGeometryToNative(&temp, geometry);
        var raw_options = if (options) |query_options| try renderedFeatureQueryOptionsToNative(&temp, query_options) else undefined;
        var result: ?*c.mln_feature_query_result = null;
        try status.checkStatus(
            c.mln_render_session_query_rendered_features(try native(self), &raw_geometry, if (options != null) &raw_options else null, &result),
            self.diagnostic_store,
        );
        defer c.mln_feature_query_result_destroy(result);
        return try copyFeatureQueryResult(allocator, result.?, self.diagnostic_store);
    }

    pub fn querySourceFeatures(
        self: *RenderSessionHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        options: ?SourceFeatureQueryOptions,
    ) status.Error!FeatureQueryResult {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_source_id = try temp.stringView(source_id);
        var raw_options = if (options) |query_options| try sourceFeatureQueryOptionsToNative(&temp, query_options) else undefined;
        var result: ?*c.mln_feature_query_result = null;
        try status.checkStatus(
            c.mln_render_session_query_source_features(try native(self), raw_source_id, if (options != null) &raw_options else null, &result),
            self.diagnostic_store,
        );
        defer c.mln_feature_query_result_destroy(result);
        return try copyFeatureQueryResult(allocator, result.?, self.diagnostic_store);
    }

    pub fn queryFeatureExtension(
        self: *RenderSessionHandle,
        allocator: std.mem.Allocator,
        source_id: []const u8,
        feature: values.Feature,
        extension: []const u8,
        extension_field: []const u8,
        arguments: ?values.JsonValue,
    ) status.Error!FeatureExtensionResult {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const raw_source_id = try temp.stringView(source_id);
        const raw_feature = try temp.feature(feature);
        const raw_extension = try temp.stringView(extension);
        const raw_extension_field = try temp.stringView(extension_field);
        const raw_arguments = if (arguments) |value| try temp.jsonValue(value) else null;
        var result: ?*c.mln_feature_extension_result = null;
        try status.checkStatus(
            c.mln_render_session_query_feature_extensions(try native(self), raw_source_id, raw_feature, raw_extension, raw_extension_field, raw_arguments, &result),
            self.diagnostic_store,
        );
        defer c.mln_feature_extension_result_destroy(result);
        return try copyFeatureExtensionResult(allocator, result.?, self.diagnostic_store);
    }

    pub fn textureImageInfo(self: *RenderSessionHandle) status.Error!TextureImageInfo {
        var info = c.mln_texture_image_info_default();
        const probe_status = c.mln_texture_read_premultiplied_rgba8(try native(self), null, 0, &info);
        switch (probe_status) {
            c.MLN_STATUS_INVALID_ARGUMENT => {},
            else => try status.checkStatus(probe_status, self.diagnostic_store),
        }
        return textureImageInfoFromNative(info);
    }

    pub fn readPremultipliedRgba8Into(self: *RenderSessionHandle, buffer: []u8) status.Error!TextureImageInfo {
        var info = c.mln_texture_image_info_default();
        try status.checkStatus(
            c.mln_texture_read_premultiplied_rgba8(try native(self), buffer.ptr, buffer.len, &info),
            self.diagnostic_store,
        );
        return textureImageInfoFromNative(info);
    }

    pub fn readPremultipliedRgba8(self: *RenderSessionHandle, allocator: std.mem.Allocator) status.Error!OwnedImage {
        const info = try self.textureImageInfo();
        const data = try allocator.alloc(u8, info.byte_length);
        errdefer allocator.free(data);
        const copied_info = try self.readPremultipliedRgba8Into(data);
        return .{ .allocator = allocator, .info = copied_info, .data = data };
    }

    pub fn acquireMetalOwnedTextureFrame(self: *RenderSessionHandle) status.Error!MetalOwnedTextureFrameHandle {
        try ensureNoActiveOwnedFrame(self);
        var frame = c.mln_metal_owned_texture_frame{
            .size = @sizeOf(c.mln_metal_owned_texture_frame),
            .generation = 0,
            .width = 0,
            .height = 0,
            .scale_factor = 0,
            .frame_id = 0,
            .texture = null,
            .device = null,
            .pixel_format = 0,
        };
        const session_native = try native(self);
        const session_frame_state = try frameState(self);
        try status.checkStatus(c.mln_metal_owned_texture_acquire_frame(session_native, &frame), self.diagnostic_store);
        session_frame_state.metal_frame_generation +%= 1;
        session_frame_state.metal_frame = .{ .raw = frame };
        session_frame_state.metal_frame_active = true;
        return .{
            .session_native = @ptrCast(session_native),
            .diagnostic_store = self.diagnostic_store,
            .frame_state = session_frame_state,
            .generation = session_frame_state.metal_frame_generation,
        };
    }

    pub fn acquireVulkanOwnedTextureFrame(self: *RenderSessionHandle) status.Error!VulkanOwnedTextureFrameHandle {
        try ensureNoActiveOwnedFrame(self);
        var frame = c.mln_vulkan_owned_texture_frame{
            .size = @sizeOf(c.mln_vulkan_owned_texture_frame),
            .generation = 0,
            .width = 0,
            .height = 0,
            .scale_factor = 0,
            .frame_id = 0,
            .image = null,
            .image_view = null,
            .device = null,
            .format = 0,
            .layout = 0,
        };
        const session_native = try native(self);
        const session_frame_state = try frameState(self);
        try status.checkStatus(c.mln_vulkan_owned_texture_acquire_frame(session_native, &frame), self.diagnostic_store);
        session_frame_state.vulkan_frame_generation +%= 1;
        session_frame_state.vulkan_frame = .{ .raw = frame };
        session_frame_state.vulkan_frame_active = true;
        return .{
            .session_native = @ptrCast(session_native),
            .diagnostic_store = self.diagnostic_store,
            .frame_state = session_frame_state,
            .generation = session_frame_state.vulkan_frame_generation,
        };
    }

    pub fn close(self: *RenderSessionHandle) status.Error!void {
        const session: *c.mln_render_session = @ptrCast(self.native orelse return);
        const session_frame_state = try frameState(self);
        if (session_frame_state.metal_frame_active or session_frame_state.vulkan_frame_active) return error.ActiveBorrow;
        try status.checkStatus(c.mln_render_session_destroy(session), self.diagnostic_store);
        self.native = null;
        self.frame_state = null;
        std.heap.smp_allocator.destroy(session_frame_state);
    }
};

pub const MetalOwnedTextureFrameHandle = struct {
    session_native: ?*NativeRenderSession,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: ?*RenderSessionFrameState,
    generation: u64,

    /// Returns native Metal objects for this acquired frame.
    ///
    /// Safety: returned backend pointers stay valid only until this frame handle
    /// is released. Callers must follow the backend synchronization rules from
    /// the C API while using them.
    pub fn info(self: *const MetalOwnedTextureFrameHandle) status.BindingError!MetalOwnedTextureFrameInfo {
        const frame = try metalFrame(self);
        return .{
            .generation = frame.generation,
            .width = frame.width,
            .height = frame.height,
            .scale_factor = frame.scale_factor,
            .texture = .{ .ptr = frame.texture orelse return error.ClosedHandle },
            .device = .{ .ptr = frame.device orelse return error.ClosedHandle },
            .pixel_format = frame.pixel_format,
        };
    }

    pub fn release(self: *MetalOwnedTextureFrameHandle) status.Error!void {
        const session_frame_state = self.frame_state orelse return;
        const session_native = self.session_native orelse return;
        if (!session_frame_state.metal_frame_active or self.generation != session_frame_state.metal_frame_generation) return;
        var frame = (session_frame_state.metal_frame orelse return).raw;
        try status.checkStatus(
            c.mln_metal_owned_texture_release_frame(@ptrCast(session_native), &frame),
            self.diagnostic_store,
        );
        session_frame_state.metal_frame_active = false;
        session_frame_state.metal_frame = null;
        self.* = .{ .session_native = null, .diagnostic_store = self.diagnostic_store, .frame_state = null, .generation = 0 };
    }
};

pub const VulkanOwnedTextureFrameHandle = struct {
    session_native: ?*NativeRenderSession,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: ?*RenderSessionFrameState,
    generation: u64,

    /// Returns native Vulkan objects for this acquired frame.
    ///
    /// Safety: returned backend pointers stay valid only until this frame handle
    /// is released. Callers must follow the backend synchronization rules from
    /// the C API while using them.
    pub fn info(self: *const VulkanOwnedTextureFrameHandle) status.BindingError!VulkanOwnedTextureFrameInfo {
        const frame = try vulkanFrame(self);
        return .{
            .generation = frame.generation,
            .width = frame.width,
            .height = frame.height,
            .scale_factor = frame.scale_factor,
            .image = .{ .ptr = frame.image orelse return error.ClosedHandle },
            .image_view = .{ .ptr = frame.image_view orelse return error.ClosedHandle },
            .device = .{ .ptr = frame.device orelse return error.ClosedHandle },
            .format = frame.format,
            .layout = frame.layout,
        };
    }

    pub fn release(self: *VulkanOwnedTextureFrameHandle) status.Error!void {
        const session_frame_state = self.frame_state orelse return;
        const session_native = self.session_native orelse return;
        if (!session_frame_state.vulkan_frame_active or self.generation != session_frame_state.vulkan_frame_generation) return;
        var frame = (session_frame_state.vulkan_frame orelse return).raw;
        try status.checkStatus(
            c.mln_vulkan_owned_texture_release_frame(@ptrCast(session_native), &frame),
            self.diagnostic_store,
        );
        session_frame_state.vulkan_frame_active = false;
        session_frame_state.vulkan_frame = null;
        self.* = .{ .session_native = null, .diagnostic_store = self.diagnostic_store, .frame_state = null, .generation = 0 };
    }
};

pub fn attachMetalOwnedTexture(map: *map_module.MapHandle, descriptor: MetalOwnedTextureDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_metal_owned_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = metalContextToNative(descriptor.context);
    return try attach(map, c.mln_metal_owned_texture_attach, &raw);
}

pub fn attachMetalBorrowedTexture(map: *map_module.MapHandle, descriptor: MetalBorrowedTextureDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_metal_borrowed_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.texture = descriptor.texture.ptr;
    return try attach(map, c.mln_metal_borrowed_texture_attach, &raw);
}

pub fn attachVulkanOwnedTexture(map: *map_module.MapHandle, descriptor: VulkanOwnedTextureDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_vulkan_owned_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = vulkanContextToNative(descriptor.context);
    return try attach(map, c.mln_vulkan_owned_texture_attach, &raw);
}

pub fn attachVulkanBorrowedTexture(map: *map_module.MapHandle, descriptor: VulkanBorrowedTextureDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_vulkan_borrowed_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = vulkanContextToNative(descriptor.context);
    raw.image = descriptor.image.ptr;
    raw.image_view = descriptor.image_view.ptr;
    raw.format = descriptor.format;
    raw.initial_layout = descriptor.initial_layout;
    raw.final_layout = descriptor.final_layout;
    return try attach(map, c.mln_vulkan_borrowed_texture_attach, &raw);
}

pub fn attachMetalSurface(map: *map_module.MapHandle, descriptor: MetalSurfaceDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_metal_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = metalContextToNative(descriptor.context);
    raw.layer = descriptor.layer.ptr;
    return try attach(map, c.mln_metal_surface_attach, &raw);
}

pub fn attachVulkanSurface(map: *map_module.MapHandle, descriptor: VulkanSurfaceDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_vulkan_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = vulkanContextToNative(descriptor.context);
    raw.surface = descriptor.surface.ptr;
    return try attach(map, c.mln_vulkan_surface_attach, &raw);
}

fn attach(
    map: *map_module.MapHandle,
    comptime attachFn: anytype,
    descriptor: anytype,
) status.Error!RenderSessionHandle {
    var session: ?*c.mln_render_session = null;
    try status.checkStatus(
        attachFn(try map_module.native(map), descriptor, &session),
        map_module.diagnosticStore(map),
    );
    errdefer {
        if (session) |handle| _ = c.mln_render_session_destroy(handle);
    }
    return try newRenderSession(session.?, map_module.diagnosticStore(map));
}

fn newRenderSession(
    session: *c.mln_render_session,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) std.mem.Allocator.Error!RenderSessionHandle {
    const session_frame_state = try std.heap.smp_allocator.create(RenderSessionFrameState);
    session_frame_state.* = .{};
    return .{ .native = @ptrCast(session), .diagnostic_store = diagnostic_store, .frame_state = session_frame_state };
}

fn native(handle: *RenderSessionHandle) status.BindingError!*c.mln_render_session {
    return @ptrCast(handle.native orelse return error.ClosedHandle);
}

fn frameState(handle: *RenderSessionHandle) status.BindingError!*RenderSessionFrameState {
    return handle.frame_state orelse error.ClosedHandle;
}

fn ensureNoActiveOwnedFrame(handle: *RenderSessionHandle) status.BindingError!void {
    _ = handle.native orelse return error.ClosedHandle;
    const session_frame_state = try frameState(handle);
    if (session_frame_state.metal_frame_active or session_frame_state.vulkan_frame_active) return error.ActiveBorrow;
}

fn metalFrame(handle: *const MetalOwnedTextureFrameHandle) status.BindingError!c.mln_metal_owned_texture_frame {
    const session_frame_state = handle.frame_state orelse return error.ClosedHandle;
    if (!session_frame_state.metal_frame_active or handle.generation != session_frame_state.metal_frame_generation) return error.ClosedHandle;
    return (session_frame_state.metal_frame orelse return error.ClosedHandle).raw;
}

fn vulkanFrame(handle: *const VulkanOwnedTextureFrameHandle) status.BindingError!c.mln_vulkan_owned_texture_frame {
    const session_frame_state = handle.frame_state orelse return error.ClosedHandle;
    if (!session_frame_state.vulkan_frame_active or handle.generation != session_frame_state.vulkan_frame_generation) return error.ClosedHandle;
    return (session_frame_state.vulkan_frame orelse return error.ClosedHandle).raw;
}

fn renderTargetExtentToNative(extent: RenderTargetExtent) c.mln_render_target_extent {
    return .{
        .size = @sizeOf(c.mln_render_target_extent),
        .width = extent.width,
        .height = extent.height,
        .scale_factor = extent.scale_factor,
    };
}

fn textureImageInfoFromNative(info: c.mln_texture_image_info) TextureImageInfo {
    return .{
        .width = info.width,
        .height = info.height,
        .stride = info.stride,
        .byte_length = info.byte_length,
    };
}

fn renderedQueryGeometryToNative(
    temp: *native_temp.TempStorage,
    geometry: RenderedQueryGeometry,
) status.Error!c.mln_rendered_query_geometry {
    return switch (geometry) {
        .point => |point| c.mln_rendered_query_geometry_point(values.screenPointToNative(point)),
        .box => |box| c.mln_rendered_query_geometry_box(.{
            .min = values.screenPointToNative(box.min),
            .max = values.screenPointToNative(box.max),
        }),
        .line_string => |points| blk: {
            const raw_points = try temp.screenPoints(points);
            break :blk c.mln_rendered_query_geometry_line_string(raw_points.ptr, raw_points.len);
        },
    };
}

fn renderedFeatureQueryOptionsToNative(
    temp: *native_temp.TempStorage,
    options: RenderedFeatureQueryOptions,
) status.Error!c.mln_rendered_feature_query_options {
    var raw = c.mln_rendered_feature_query_options_default();
    if (options.layer_ids) |layer_ids| {
        raw.fields |= c.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
        raw.layer_ids = (try stringViewArray(temp, layer_ids)).ptr;
        raw.layer_id_count = layer_ids.len;
    }
    if (options.filter) |filter| raw.filter = try temp.jsonValue(filter);
    return raw;
}

fn sourceFeatureQueryOptionsToNative(
    temp: *native_temp.TempStorage,
    options: SourceFeatureQueryOptions,
) status.Error!c.mln_source_feature_query_options {
    var raw = c.mln_source_feature_query_options_default();
    if (options.source_layer_ids) |source_layer_ids| {
        raw.fields |= c.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
        raw.source_layer_ids = (try stringViewArray(temp, source_layer_ids)).ptr;
        raw.source_layer_id_count = source_layer_ids.len;
    }
    if (options.filter) |filter| raw.filter = try temp.jsonValue(filter);
    return raw;
}

fn stringViewArray(temp: *native_temp.TempStorage, values_list: []const []const u8) status.Error![]c.mln_string_view {
    const raw = try temp.arena.allocator().alloc(c.mln_string_view, values_list.len);
    for (values_list, raw) |value, *out| out.* = try temp.stringView(value);
    return raw;
}

fn featureStateSelectorToNative(
    temp: *native_temp.TempStorage,
    selector: FeatureStateSelector,
) status.Error!c.mln_feature_state_selector {
    var raw = c.mln_feature_state_selector{
        .size = @sizeOf(c.mln_feature_state_selector),
        .fields = 0,
        .source_id = try temp.stringView(selector.source_id),
        .source_layer_id = .{ .data = null, .size = 0 },
        .feature_id = .{ .data = null, .size = 0 },
        .state_key = .{ .data = null, .size = 0 },
    };
    if (selector.source_layer_id) |source_layer_id| {
        raw.fields |= c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID;
        raw.source_layer_id = try temp.stringView(source_layer_id);
    }
    if (selector.feature_id) |feature_id| {
        raw.fields |= c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID;
        raw.feature_id = try temp.stringView(feature_id);
    }
    if (selector.state_key) |state_key| {
        raw.fields |= c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY;
        raw.state_key = try temp.stringView(state_key);
    }
    return raw;
}

fn copyFeatureQueryResult(
    allocator: std.mem.Allocator,
    result: *c.mln_feature_query_result,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!FeatureQueryResult {
    var count: usize = 0;
    try status.checkStatus(c.mln_feature_query_result_count(result, &count), diagnostic_store);
    const features = try allocator.alloc(QueriedFeature, count);
    var initialized: usize = 0;
    errdefer {
        for (features[0..initialized]) |*feature| feature.deinit();
        allocator.free(features);
    }
    for (features, 0..) |*feature, index| {
        var raw = c.mln_queried_feature{
            .size = @sizeOf(c.mln_queried_feature),
            .fields = 0,
            .feature = undefined,
            .source_id = .{ .data = null, .size = 0 },
            .source_layer_id = .{ .data = null, .size = 0 },
            .state = null,
        };
        try status.checkStatus(c.mln_feature_query_result_get(result, index, &raw), diagnostic_store);
        feature.* = try copyQueriedFeature(allocator, &raw);
        initialized += 1;
    }
    return .{ .allocator = allocator, .features = features };
}

fn copyQueriedFeature(allocator: std.mem.Allocator, raw: *const c.mln_queried_feature) status.Error!QueriedFeature {
    const feature = try copyOwnedFeature(allocator, &raw.feature);
    errdefer {
        var mutable_feature = feature;
        mutable_feature.deinit();
    }

    const source_id = if ((raw.fields & c.MLN_QUERIED_FEATURE_SOURCE_ID) != 0) try copyStringView(allocator, raw.source_id) else null;
    errdefer if (source_id) |value| allocator.free(value);
    const source_layer_id = if ((raw.fields & c.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0) try copyStringView(allocator, raw.source_layer_id) else null;
    errdefer if (source_layer_id) |value| allocator.free(value);
    const feature_state = if ((raw.fields & c.MLN_QUERIED_FEATURE_STATE) != 0 and raw.state != null) try values.ownedJsonValueFromNative(allocator, raw.state.?) else null;
    errdefer if (feature_state) |*value| {
        var mutable_value = value.*;
        mutable_value.deinit(allocator);
    };

    return .{
        .feature = feature,
        .source_id = source_id,
        .source_layer_id = source_layer_id,
        .state = feature_state,
    };
}

fn copyFeatureExtensionResult(
    allocator: std.mem.Allocator,
    result: *c.mln_feature_extension_result,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!FeatureExtensionResult {
    var info = c.mln_feature_extension_result_info{
        .size = @sizeOf(c.mln_feature_extension_result_info),
        .type = 0,
        .data = .{ .value = null },
    };
    try status.checkStatus(c.mln_feature_extension_result_get(result, &info), diagnostic_store);
    return switch (info.type) {
        c.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE => .{ .value = try values.ownedJsonValueFromNative(allocator, info.data.value) },
        c.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION => .{ .feature_collection = try copyFeatureCollection(allocator, info.data.feature_collection) },
        else => error.UnknownStatus,
    };
}

fn copyFeatureCollection(allocator: std.mem.Allocator, raw: c.mln_feature_collection) status.Error!OwnedFeatureCollection {
    const features = try allocator.alloc(OwnedFeature, raw.feature_count);
    var initialized: usize = 0;
    errdefer {
        for (features[0..initialized]) |*feature| feature.deinit();
        allocator.free(features);
    }
    if (raw.feature_count > 0) {
        for (raw.features[0..raw.feature_count], features) |feature, *out| {
            out.* = try copyOwnedFeature(allocator, &feature);
            initialized += 1;
        }
    }
    return .{ .allocator = allocator, .features = features };
}

fn copyOwnedFeature(allocator: std.mem.Allocator, raw: *const c.mln_feature) status.Error!OwnedFeature {
    const geometry = try values.ownedGeometryFromNative(allocator, raw.geometry);
    errdefer {
        var mutable_geometry = geometry;
        mutable_geometry.deinit(allocator);
    }

    const properties = try allocator.alloc(values.OwnedJsonMember, raw.property_count);
    var properties_initialized: usize = 0;
    errdefer {
        for (properties[0..properties_initialized]) |*property| {
            allocator.free(property.key);
            property.value.deinit(allocator);
        }
        allocator.free(properties);
    }
    if (raw.property_count > 0) {
        for (raw.properties[0..raw.property_count], properties) |property, *out| {
            const key = try copyStringView(allocator, property.key);
            errdefer allocator.free(key);
            const value = try values.ownedJsonValueFromNative(allocator, property.value);
            out.* = .{ .key = key, .value = value };
            properties_initialized += 1;
        }
    }

    const identifier = try copyFeatureIdentifier(allocator, raw.*);
    errdefer if (identifier == .string) allocator.free(identifier.string);

    return .{
        .allocator = allocator,
        .geometry = geometry,
        .properties = properties,
        .identifier = identifier,
    };
}

fn copyFeatureIdentifier(allocator: std.mem.Allocator, raw: c.mln_feature) status.Error!values.FeatureIdentifier {
    return switch (raw.identifier_type) {
        c.MLN_FEATURE_IDENTIFIER_TYPE_NULL => .null,
        c.MLN_FEATURE_IDENTIFIER_TYPE_UINT => .{ .uint = raw.identifier.uint_value },
        c.MLN_FEATURE_IDENTIFIER_TYPE_INT => .{ .int = raw.identifier.int_value },
        c.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE => .{ .double = raw.identifier.double_value },
        c.MLN_FEATURE_IDENTIFIER_TYPE_STRING => .{ .string = try copyStringView(allocator, raw.identifier.string_value) },
        else => error.UnknownStatus,
    };
}

fn copyStringView(allocator: std.mem.Allocator, view: c.mln_string_view) std.mem.Allocator.Error![]const u8 {
    if (view.size == 0) return allocator.dupe(u8, "");
    return allocator.dupe(u8, view.data[0..view.size]);
}

fn metalContextToNative(context: MetalContextDescriptor) c.mln_metal_context_descriptor {
    return .{
        .size = @sizeOf(c.mln_metal_context_descriptor),
        .device = if (context.device) |device| device.ptr else null,
    };
}

fn vulkanContextToNative(context: VulkanContextDescriptor) c.mln_vulkan_context_descriptor {
    return .{
        .size = @sizeOf(c.mln_vulkan_context_descriptor),
        .instance = context.instance.ptr,
        .physical_device = context.physical_device.ptr,
        .device = context.device.ptr,
        .graphics_queue = context.graphics_queue.ptr,
        .graphics_queue_family_index = context.graphics_queue_family_index,
    };
}

test "feature identifier copy rejects unknown native tags" {
    var feature = std.mem.zeroes(c.mln_feature);
    feature.identifier_type = 0xbeef;
    try std.testing.expectError(error.UnknownStatus, copyFeatureIdentifier(std.testing.allocator, feature));
}
