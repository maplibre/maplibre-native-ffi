const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map_module = @import("map.zig");
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const status = @import("status.zig");
const values = @import("values.zig");

const NativeRenderSession = opaque {};
const MetalOwnedTextureFrameStorage = struct { raw: c.mln_metal_owned_texture_frame };
const OpenGLOwnedTextureFrameStorage = struct { raw: c.mln_opengl_owned_texture_frame };
const VulkanOwnedTextureFrameStorage = struct { raw: c.mln_vulkan_owned_texture_frame };

const RenderSessionFrameState = struct {
    metal_frame: ?MetalOwnedTextureFrameStorage = null,
    metal_frame_active: bool = false,
    metal_frame_generation: u64 = 0,
    opengl_frame: ?OpenGLOwnedTextureFrameStorage = null,
    opengl_frame_active: bool = false,
    opengl_frame_generation: u64 = 0,
    vulkan_frame: ?VulkanOwnedTextureFrameStorage = null,
    vulkan_frame_active: bool = false,
    vulkan_frame_generation: u64 = 0,
};

const RenderSessionState = struct {
    native: ?*NativeRenderSession,
    map_handle: map_module.MapHandle,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: ?*RenderSessionFrameState,
    active_leases: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    closing: bool = false,
};

const RenderSessionRegistrySlot = struct {
    state: ?*RenderSessionState,
    generation: u64,
};

const OwnedTextureFrameKind = enum {
    metal,
    vulkan,
    opengl,
};

const OwnedTextureFrameState = struct {
    kind: OwnedTextureFrameKind,
    session_native: ?*NativeRenderSession,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: ?*RenderSessionFrameState,
    generation: u64,
    active_leases: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    closing: bool = false,
};

const OwnedTextureFrameRegistrySlot = struct {
    state: ?*OwnedTextureFrameState,
    generation: u64,
};

const RenderSessionLease = struct {
    state: *RenderSessionState,
    native: *c.mln_render_session,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: *RenderSessionFrameState,

    fn release(self: RenderSessionLease) void {
        _ = self.state.active_leases.fetchSub(1, .seq_cst);
    }
};

const RenderSessionClose = struct {
    state: *RenderSessionState,
    native: *c.mln_render_session,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: *RenderSessionFrameState,
    map_handle: map_module.MapHandle,
};

const OwnedTextureFrameLease = struct {
    state: *OwnedTextureFrameState,
    session_native: *c.mln_render_session,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    frame_state: *RenderSessionFrameState,
    generation: u64,

    fn release(self: OwnedTextureFrameLease) void {
        _ = self.state.active_leases.fetchSub(1, .seq_cst);
    }
};

var render_session_registry_lock = std.atomic.Value(bool).init(false);
var render_session_registry: std.ArrayList(RenderSessionRegistrySlot) = .empty;
var render_session_free_list: std.ArrayList(usize) = .empty;

var owned_texture_frame_registry_lock = std.atomic.Value(bool).init(false);
var owned_texture_frame_registry: std.ArrayList(OwnedTextureFrameRegistrySlot) = .empty;
var owned_texture_frame_free_list: std.ArrayList(usize) = .empty;
var fail_next_owned_texture_frame_wrapper_allocation_for_testing = std.atomic.Value(bool).init(false);

const FeatureQueryResultDestroyFn = *const fn (?*c.mln_feature_query_result) callconv(.c) void;
var feature_query_result_destroy_for_testing: FeatureQueryResultDestroyFn = c.mln_feature_query_result_destroy;
var feature_query_result_destroy_count_for_testing = std.atomic.Value(usize).init(0);

pub const NativePointer = enum(usize) {
    _,

    /// Creates a borrowed backend-native pointer.
    ///
    /// The caller keeps the backend object valid and synchronized for the full
    /// C API borrow window documented by the descriptor that receives it.
    pub fn fromPtr(ptr: *anyopaque) NativePointer {
        return @enumFromInt(@intFromPtr(ptr));
    }

    /// Returns the borrowed backend-native address.
    ///
    /// The returned pointer grants no ownership and is valid only for the
    /// backend lifetime and synchronization scope documented by the operation
    /// that produced or accepts this value.
    pub fn toPtr(self: NativePointer) *anyopaque {
        return @ptrFromInt(@intFromEnum(self));
    }
};

pub const RenderBackendSupport = struct {
    metal: bool,
    opengl: bool,
    vulkan: bool,
};

pub fn supportedRenderBackends() RenderBackendSupport {
    const mask = c.mln_supported_render_backend_mask();
    return .{
        .metal = (mask & c.MLN_RENDER_BACKEND_FLAG_METAL) != 0,
        .opengl = (mask & c.MLN_RENDER_BACKEND_FLAG_OPENGL) != 0,
        .vulkan = (mask & c.MLN_RENDER_BACKEND_FLAG_VULKAN) != 0,
    };
}

pub const OpenGLContextProviderSupport = struct {
    wgl: bool,
    egl: bool,
};

pub fn supportedOpenGLContextProviders() OpenGLContextProviderSupport {
    const mask = c.mln_opengl_supported_context_provider_mask();
    return .{
        .wgl = (mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL) != 0,
        .egl = (mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL) != 0,
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
    get_instance_proc_addr: ?NativePointer = null,
    get_device_proc_addr: ?NativePointer = null,
};

pub const WglContextDescriptor = struct {
    device_context: NativePointer,
    share_context: NativePointer,
    get_proc_address: ?NativePointer = null,
};

pub const EglContextDescriptor = struct {
    display: NativePointer,
    config: NativePointer,
    share_context: NativePointer,
    get_proc_address: ?NativePointer = null,
};

pub const OpenGLContextDescriptor = union(enum) {
    wgl: WglContextDescriptor,
    egl: EglContextDescriptor,
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

pub const OpenGLOwnedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: OpenGLContextDescriptor,
};

pub const OpenGLBorrowedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: OpenGLContextDescriptor,
    texture: u32,
    target: u32,
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

pub const OpenGLSurfaceDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: OpenGLContextDescriptor,
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

pub const OpenGLOwnedTextureFrameInfo = struct {
    generation: u64,
    width: u32,
    height: u32,
    scale_factor: f64,
    texture: u32,
    target: u32,
    internal_format: u32,
    format: u32,
    type: u32,
};

pub const RenderSessionHandle = enum(u128) {
    _,

    pub fn resize(self: *RenderSessionHandle, extent: RenderTargetExtent) status.Error!void {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try ensureNoActiveOwnedFrame(lease);
        try status.checkStatus(
            c.mln_render_session_resize(lease.native, extent.width, extent.height, extent.scale_factor),
            lease.diagnostic_store,
        );
    }

    pub fn renderUpdate(self: *RenderSessionHandle) status.Error!void {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try ensureNoActiveOwnedFrame(lease);
        try status.checkStatus(c.mln_render_session_render_update(lease.native), lease.diagnostic_store);
    }

    pub fn detach(self: *RenderSessionHandle) status.Error!void {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try ensureNoActiveOwnedFrame(lease);
        try status.checkStatus(c.mln_render_session_detach(lease.native), lease.diagnostic_store);
    }

    pub fn reduceMemoryUse(self: *RenderSessionHandle) status.Error!void {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(c.mln_render_session_reduce_memory_use(lease.native), lease.diagnostic_store);
    }

    pub fn clearData(self: *RenderSessionHandle) status.Error!void {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(c.mln_render_session_clear_data(lease.native), lease.diagnostic_store);
    }

    pub fn dumpDebugLogs(self: *RenderSessionHandle) status.Error!void {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(c.mln_render_session_dump_debug_logs(lease.native), lease.diagnostic_store);
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
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_render_session_set_feature_state(lease.native, &raw_selector, raw_state),
            lease.diagnostic_store,
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
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_render_session_get_feature_state(lease.native, &raw_selector, &snapshot),
            lease.diagnostic_store,
        );
        defer c.mln_json_snapshot_destroy(snapshot);
        var raw: ?*const c.mln_json_value = null;
        try status.checkStatus(c.mln_json_snapshot_get(snapshot.?, &raw), lease.diagnostic_store);
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
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_render_session_remove_feature_state(lease.native, &raw_selector),
            lease.diagnostic_store,
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
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_render_session_query_rendered_features(lease.native, &raw_geometry, if (options != null) &raw_options else null, &result),
            lease.diagnostic_store,
        );
        defer destroyFeatureQueryResult(result);
        return try copyFeatureQueryResult(allocator, result.?, lease.diagnostic_store);
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
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_render_session_query_source_features(lease.native, raw_source_id, if (options != null) &raw_options else null, &result),
            lease.diagnostic_store,
        );
        defer destroyFeatureQueryResult(result);
        return try copyFeatureQueryResult(allocator, result.?, lease.diagnostic_store);
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
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_render_session_query_feature_extensions(lease.native, raw_source_id, raw_feature, raw_extension, raw_extension_field, raw_arguments, &result),
            lease.diagnostic_store,
        );
        defer c.mln_feature_extension_result_destroy(result);
        return try copyFeatureExtensionResult(allocator, result.?, lease.diagnostic_store);
    }

    pub fn readPremultipliedRgba8Into(self: *RenderSessionHandle, buffer: []u8) status.Error!TextureImageInfo {
        var info = c.mln_texture_image_info_default();
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try status.checkStatus(
            c.mln_texture_read_premultiplied_rgba8(lease.native, buffer.ptr, buffer.len, &info),
            lease.diagnostic_store,
        );
        return textureImageInfoFromNative(info);
    }

    pub fn acquireMetalOwnedTextureFrame(self: *RenderSessionHandle) status.Error!MetalOwnedTextureFrameHandle {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try ensureNoActiveOwnedFrame(lease);
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
        try status.checkStatus(c.mln_metal_owned_texture_acquire_frame(lease.native, &frame), lease.diagnostic_store);
        const session_frame_state = lease.frame_state;
        session_frame_state.metal_frame_generation +%= 1;
        session_frame_state.metal_frame = .{ .raw = frame };
        session_frame_state.metal_frame_active = true;
        errdefer {
            var release_frame = frame;
            _ = c.mln_metal_owned_texture_release_frame(lease.native, &release_frame);
            session_frame_state.metal_frame_active = false;
            session_frame_state.metal_frame = null;
        }
        return try newOwnedTextureFrameHandle(MetalOwnedTextureFrameHandle, .{
            .kind = .metal,
            .session_native = @ptrCast(lease.native),
            .diagnostic_store = lease.diagnostic_store,
            .frame_state = session_frame_state,
            .generation = session_frame_state.metal_frame_generation,
        });
    }

    pub fn acquireVulkanOwnedTextureFrame(self: *RenderSessionHandle) status.Error!VulkanOwnedTextureFrameHandle {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try ensureNoActiveOwnedFrame(lease);
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
        try status.checkStatus(c.mln_vulkan_owned_texture_acquire_frame(lease.native, &frame), lease.diagnostic_store);
        const session_frame_state = lease.frame_state;
        session_frame_state.vulkan_frame_generation +%= 1;
        session_frame_state.vulkan_frame = .{ .raw = frame };
        session_frame_state.vulkan_frame_active = true;
        errdefer {
            var release_frame = frame;
            _ = c.mln_vulkan_owned_texture_release_frame(lease.native, &release_frame);
            session_frame_state.vulkan_frame_active = false;
            session_frame_state.vulkan_frame = null;
        }
        return try newOwnedTextureFrameHandle(VulkanOwnedTextureFrameHandle, .{
            .kind = .vulkan,
            .session_native = @ptrCast(lease.native),
            .diagnostic_store = lease.diagnostic_store,
            .frame_state = session_frame_state,
            .generation = session_frame_state.vulkan_frame_generation,
        });
    }

    pub fn acquireOpenGLOwnedTextureFrame(self: *RenderSessionHandle) status.Error!OpenGLOwnedTextureFrameHandle {
        const lease = try renderSessionLease(self.*);
        defer lease.release();
        try ensureNoActiveOwnedFrame(lease);
        var frame = c.mln_opengl_owned_texture_frame{
            .size = @sizeOf(c.mln_opengl_owned_texture_frame),
            .generation = 0,
            .width = 0,
            .height = 0,
            .scale_factor = 0,
            .frame_id = 0,
            .texture = 0,
            .target = 0,
            .internal_format = 0,
            .format = 0,
            .type = 0,
        };
        try status.checkStatus(c.mln_opengl_owned_texture_acquire_frame(lease.native, &frame), lease.diagnostic_store);
        const session_frame_state = lease.frame_state;
        session_frame_state.opengl_frame_generation +%= 1;
        session_frame_state.opengl_frame = .{ .raw = frame };
        session_frame_state.opengl_frame_active = true;
        errdefer {
            var release_frame = frame;
            _ = c.mln_opengl_owned_texture_release_frame(lease.native, &release_frame);
            session_frame_state.opengl_frame_active = false;
            session_frame_state.opengl_frame = null;
        }
        return try newOwnedTextureFrameHandle(OpenGLOwnedTextureFrameHandle, .{
            .kind = .opengl,
            .session_native = @ptrCast(lease.native),
            .diagnostic_store = lease.diagnostic_store,
            .frame_state = session_frame_state,
            .generation = session_frame_state.opengl_frame_generation,
        });
    }

    pub fn close(self: *RenderSessionHandle) status.Error!void {
        const session_close = try beginRenderSessionClose(self.*) orelse return;
        status.checkStatus(c.mln_render_session_destroy(session_close.native), session_close.diagnostic_store) catch |err| {
            cancelRenderSessionClose(session_close.state);
            return err;
        };
        map_module.unregisterRenderSession(session_close.map_handle);
        std.heap.smp_allocator.destroy(session_close.frame_state);
        const session_state = finishRenderSessionClose(self.*) orelse session_close.state;
        std.heap.smp_allocator.destroy(session_state);
    }
};

pub const MetalOwnedTextureFrameHandle = enum(u128) {
    _,

    /// Returns native Metal objects for this acquired frame.
    ///
    /// Safety: returned backend pointers stay valid only until this frame handle
    /// is released. Callers must follow the backend synchronization rules from
    /// the C API while using them.
    pub fn info(self: *const MetalOwnedTextureFrameHandle) status.BindingError!MetalOwnedTextureFrameInfo {
        const lease = try ownedTextureFrameLease(self.*, .metal);
        defer lease.release();
        const frame = try metalFrame(lease);
        return .{
            .generation = frame.generation,
            .width = frame.width,
            .height = frame.height,
            .scale_factor = frame.scale_factor,
            .texture = NativePointer.fromPtr(frame.texture orelse return error.ClosedHandle),
            .device = NativePointer.fromPtr(frame.device orelse return error.ClosedHandle),
            .pixel_format = frame.pixel_format,
        };
    }

    pub fn release(self: *MetalOwnedTextureFrameHandle) status.Error!void {
        const frame_close = try beginOwnedTextureFrameClose(self.*, .metal) orelse return;
        const session_frame_state = frame_close.frame_state;
        const session_native = frame_close.session_native;
        if (!session_frame_state.metal_frame_active or frame_close.generation != session_frame_state.metal_frame_generation) {
            cancelOwnedTextureFrameClose(frame_close.state);
            return;
        }
        var frame = (session_frame_state.metal_frame orelse {
            cancelOwnedTextureFrameClose(frame_close.state);
            return;
        }).raw;
        status.checkStatus(c.mln_metal_owned_texture_release_frame(session_native, &frame), frame_close.diagnostic_store) catch |err| {
            cancelOwnedTextureFrameClose(frame_close.state);
            return err;
        };
        session_frame_state.metal_frame_active = false;
        session_frame_state.metal_frame = null;
        const frame_state = unregisterOwnedTextureFrameState(self.*) orelse return;
        std.heap.smp_allocator.destroy(frame_state);
    }
};

pub const VulkanOwnedTextureFrameHandle = enum(u128) {
    _,

    /// Returns native Vulkan objects for this acquired frame.
    ///
    /// Safety: returned backend pointers stay valid only until this frame handle
    /// is released. Callers must follow the backend synchronization rules from
    /// the C API while using them.
    pub fn info(self: *const VulkanOwnedTextureFrameHandle) status.BindingError!VulkanOwnedTextureFrameInfo {
        const lease = try ownedTextureFrameLease(self.*, .vulkan);
        defer lease.release();
        const frame = try vulkanFrame(lease);
        return .{
            .generation = frame.generation,
            .width = frame.width,
            .height = frame.height,
            .scale_factor = frame.scale_factor,
            .image = NativePointer.fromPtr(frame.image orelse return error.ClosedHandle),
            .image_view = NativePointer.fromPtr(frame.image_view orelse return error.ClosedHandle),
            .device = NativePointer.fromPtr(frame.device orelse return error.ClosedHandle),
            .format = frame.format,
            .layout = frame.layout,
        };
    }

    pub fn release(self: *VulkanOwnedTextureFrameHandle) status.Error!void {
        const frame_close = try beginOwnedTextureFrameClose(self.*, .vulkan) orelse return;
        const session_frame_state = frame_close.frame_state;
        const session_native = frame_close.session_native;
        if (!session_frame_state.vulkan_frame_active or frame_close.generation != session_frame_state.vulkan_frame_generation) {
            cancelOwnedTextureFrameClose(frame_close.state);
            return;
        }
        var frame = (session_frame_state.vulkan_frame orelse {
            cancelOwnedTextureFrameClose(frame_close.state);
            return;
        }).raw;
        status.checkStatus(c.mln_vulkan_owned_texture_release_frame(session_native, &frame), frame_close.diagnostic_store) catch |err| {
            cancelOwnedTextureFrameClose(frame_close.state);
            return err;
        };
        session_frame_state.vulkan_frame_active = false;
        session_frame_state.vulkan_frame = null;
        const frame_state = unregisterOwnedTextureFrameState(self.*) orelse return;
        std.heap.smp_allocator.destroy(frame_state);
    }
};

pub const OpenGLOwnedTextureFrameHandle = enum(u128) {
    _,

    /// Returns native OpenGL object names for this acquired frame.
    ///
    /// Safety: returned texture names stay valid only until this frame handle is
    /// released. Callers must follow the C API context-sharing and
    /// synchronization rules while using them.
    pub fn info(self: *const OpenGLOwnedTextureFrameHandle) status.BindingError!OpenGLOwnedTextureFrameInfo {
        const lease = try ownedTextureFrameLease(self.*, .opengl);
        defer lease.release();
        const frame = try openglFrame(lease);
        return .{
            .generation = frame.generation,
            .width = frame.width,
            .height = frame.height,
            .scale_factor = frame.scale_factor,
            .texture = frame.texture,
            .target = frame.target,
            .internal_format = frame.internal_format,
            .format = frame.format,
            .type = frame.type,
        };
    }

    pub fn release(self: *OpenGLOwnedTextureFrameHandle) status.Error!void {
        const frame_close = try beginOwnedTextureFrameClose(self.*, .opengl) orelse return;
        const session_frame_state = frame_close.frame_state;
        const session_native = frame_close.session_native;
        if (!session_frame_state.opengl_frame_active or frame_close.generation != session_frame_state.opengl_frame_generation) {
            cancelOwnedTextureFrameClose(frame_close.state);
            return;
        }
        var frame = (session_frame_state.opengl_frame orelse {
            cancelOwnedTextureFrameClose(frame_close.state);
            return;
        }).raw;
        status.checkStatus(c.mln_opengl_owned_texture_release_frame(session_native, &frame), frame_close.diagnostic_store) catch |err| {
            cancelOwnedTextureFrameClose(frame_close.state);
            return err;
        };
        session_frame_state.opengl_frame_active = false;
        session_frame_state.opengl_frame = null;
        const frame_state = unregisterOwnedTextureFrameState(self.*) orelse return;
        std.heap.smp_allocator.destroy(frame_state);
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
    raw.texture = descriptor.texture.toPtr();
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
    raw.image = descriptor.image.toPtr();
    raw.image_view = descriptor.image_view.toPtr();
    raw.format = descriptor.format;
    raw.initial_layout = descriptor.initial_layout;
    raw.final_layout = descriptor.final_layout;
    return try attach(map, c.mln_vulkan_borrowed_texture_attach, &raw);
}

pub fn attachOpenGLOwnedTexture(map: *map_module.MapHandle, descriptor: OpenGLOwnedTextureDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_opengl_owned_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = openglContextToNative(descriptor.context);
    return try attach(map, c.mln_opengl_owned_texture_attach, &raw);
}

pub fn attachOpenGLBorrowedTexture(map: *map_module.MapHandle, descriptor: OpenGLBorrowedTextureDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_opengl_borrowed_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = openglContextToNative(descriptor.context);
    raw.texture = descriptor.texture;
    raw.target = descriptor.target;
    return try attach(map, c.mln_opengl_borrowed_texture_attach, &raw);
}

pub fn attachMetalSurface(map: *map_module.MapHandle, descriptor: MetalSurfaceDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_metal_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = metalContextToNative(descriptor.context);
    raw.layer = descriptor.layer.toPtr();
    return try attach(map, c.mln_metal_surface_attach, &raw);
}

pub fn attachVulkanSurface(map: *map_module.MapHandle, descriptor: VulkanSurfaceDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_vulkan_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = vulkanContextToNative(descriptor.context);
    raw.surface = descriptor.surface.toPtr();
    return try attach(map, c.mln_vulkan_surface_attach, &raw);
}

pub fn attachOpenGLSurface(map: *map_module.MapHandle, descriptor: OpenGLSurfaceDescriptor) status.Error!RenderSessionHandle {
    var raw = c.mln_opengl_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = openglContextToNative(descriptor.context);
    raw.surface = descriptor.surface.toPtr();
    return try attach(map, c.mln_opengl_surface_attach, &raw);
}

fn attach(
    map: *map_module.MapHandle,
    comptime attachFn: anytype,
    descriptor: anytype,
) status.Error!RenderSessionHandle {
    var session: ?*c.mln_render_session = null;
    const registration = try map_module.registerRenderSession(map);
    errdefer map_module.unregisterRenderSession(map.*);
    try status.checkStatus(
        attachFn(registration.native, descriptor, &session),
        registration.diagnostic_store,
    );
    errdefer {
        if (session) |handle| _ = c.mln_render_session_destroy(handle);
    }
    return try newRenderSession(session.?, map.*, registration.diagnostic_store);
}

fn newRenderSession(
    session: *c.mln_render_session,
    map_handle: map_module.MapHandle,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) std.mem.Allocator.Error!RenderSessionHandle {
    const session_frame_state = try std.heap.smp_allocator.create(RenderSessionFrameState);
    session_frame_state.* = .{};
    errdefer std.heap.smp_allocator.destroy(session_frame_state);

    const session_state = try std.heap.smp_allocator.create(RenderSessionState);
    session_state.* = .{
        .native = @ptrCast(session),
        .map_handle = map_handle,
        .diagnostic_store = diagnostic_store,
        .frame_state = session_frame_state,
    };
    errdefer std.heap.smp_allocator.destroy(session_state);

    return try registerRenderSessionState(session_state);
}

fn registerRenderSessionState(session_state: *RenderSessionState) std.mem.Allocator.Error!RenderSessionHandle {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();

    if (render_session_free_list.items.len > 0) {
        const slot_index = render_session_free_list.pop().?;
        render_session_registry.items[slot_index].state = session_state;
        render_session_registry.items[slot_index].generation = runtime_module.nextHandleGeneration();
        return renderSessionHandle(slot_index + 1, render_session_registry.items[slot_index].generation);
    }

    const generation = runtime_module.nextHandleGeneration();
    try render_session_free_list.ensureTotalCapacity(std.heap.smp_allocator, render_session_registry.items.len + 1);
    try render_session_registry.append(std.heap.smp_allocator, .{ .state = session_state, .generation = generation });
    return renderSessionHandle(render_session_registry.items.len, generation);
}

fn renderSessionHandle(index: usize, generation: u64) RenderSessionHandle {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn renderSessionHandleIndex(handle: RenderSessionHandle) ?usize {
    const index = @intFromEnum(handle) & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn renderSessionHandleGeneration(handle: RenderSessionHandle) u64 {
    return @intCast(@intFromEnum(handle) >> 64);
}

fn renderSessionLease(handle: RenderSessionHandle) status.BindingError!RenderSessionLease {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();

    const index = renderSessionHandleIndex(handle) orelse return error.ClosedHandle;
    if (index > render_session_registry.items.len) return error.ClosedHandle;
    const slot = render_session_registry.items[index - 1];
    if (slot.generation != renderSessionHandleGeneration(handle)) return error.ClosedHandle;
    const session_state = slot.state orelse return error.ClosedHandle;
    if (session_state.closing) return error.ActiveBorrow;
    const session: *c.mln_render_session = @ptrCast(session_state.native orelse return error.ClosedHandle);
    const session_frame_state = session_state.frame_state orelse return error.ClosedHandle;
    _ = session_state.active_leases.fetchAdd(1, .seq_cst);
    return .{
        .state = session_state,
        .native = session,
        .diagnostic_store = session_state.diagnostic_store,
        .frame_state = session_frame_state,
    };
}

fn beginRenderSessionClose(handle: RenderSessionHandle) status.BindingError!?RenderSessionClose {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();

    const index = renderSessionHandleIndex(handle) orelse return null;
    if (index > render_session_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &render_session_registry.items[slot_index];
    if (slot.generation != renderSessionHandleGeneration(handle)) return null;
    const session_state = slot.state orelse return null;
    if (session_state.closing) return error.ActiveBorrow;
    if (session_state.active_leases.load(.seq_cst) != 0) return error.ActiveBorrow;
    const session_frame_state = session_state.frame_state orelse return null;
    if (session_frame_state.metal_frame_active or session_frame_state.opengl_frame_active or session_frame_state.vulkan_frame_active) return error.ActiveBorrow;
    const session: *c.mln_render_session = @ptrCast(session_state.native orelse return null);
    session_state.closing = true;
    return .{
        .state = session_state,
        .native = session,
        .diagnostic_store = session_state.diagnostic_store,
        .frame_state = session_frame_state,
        .map_handle = session_state.map_handle,
    };
}

fn cancelRenderSessionClose(session_state: *RenderSessionState) void {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();

    session_state.closing = false;
}

fn finishRenderSessionClose(handle: RenderSessionHandle) ?*RenderSessionState {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();

    const index = renderSessionHandleIndex(handle) orelse return null;
    if (index > render_session_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &render_session_registry.items[slot_index];
    if (slot.generation != renderSessionHandleGeneration(handle)) return null;
    const session_state = slot.state orelse return null;
    slot.state = null;
    slot.generation = runtime_module.nextHandleGeneration();
    session_state.native = null;
    session_state.frame_state = null;
    render_session_free_list.appendAssumeCapacity(slot_index);
    return session_state;
}

fn lockRenderSessionRegistry() void {
    while (render_session_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockRenderSessionRegistry() void {
    render_session_registry_lock.store(false, .seq_cst);
}

fn newOwnedTextureFrameHandle(comptime T: type, initial_state: OwnedTextureFrameState) std.mem.Allocator.Error!T {
    if (fail_next_owned_texture_frame_wrapper_allocation_for_testing.swap(false, .seq_cst)) return error.OutOfMemory;
    const frame_state_handle = try std.heap.smp_allocator.create(OwnedTextureFrameState);
    frame_state_handle.* = initial_state;
    errdefer std.heap.smp_allocator.destroy(frame_state_handle);
    return try registerOwnedTextureFrameState(T, frame_state_handle);
}

pub fn failNextOwnedTextureFrameWrapperAllocationForTesting() void {
    fail_next_owned_texture_frame_wrapper_allocation_for_testing.store(true, .seq_cst);
}

fn destroyFeatureQueryResult(result: ?*c.mln_feature_query_result) void {
    feature_query_result_destroy_for_testing(result);
}

fn countingFeatureQueryResultDestroy(result: ?*c.mln_feature_query_result) callconv(.c) void {
    _ = feature_query_result_destroy_count_for_testing.fetchAdd(1, .seq_cst);
    c.mln_feature_query_result_destroy(result);
}

pub fn useCountingFeatureQueryResultDestroyForTesting() void {
    feature_query_result_destroy_count_for_testing.store(0, .seq_cst);
    feature_query_result_destroy_for_testing = countingFeatureQueryResultDestroy;
}

pub fn restoreFeatureQueryResultDestroyForTesting() void {
    feature_query_result_destroy_for_testing = c.mln_feature_query_result_destroy;
}

pub fn featureQueryResultDestroyCountForTesting() usize {
    return feature_query_result_destroy_count_for_testing.load(.seq_cst);
}

fn registerOwnedTextureFrameState(comptime T: type, frame_state_handle: *OwnedTextureFrameState) std.mem.Allocator.Error!T {
    lockOwnedTextureFrameRegistry();
    defer unlockOwnedTextureFrameRegistry();

    if (owned_texture_frame_free_list.items.len > 0) {
        const slot_index = owned_texture_frame_free_list.pop().?;
        owned_texture_frame_registry.items[slot_index].state = frame_state_handle;
        owned_texture_frame_registry.items[slot_index].generation = runtime_module.nextHandleGeneration();
        return ownedTextureFrameHandle(T, slot_index + 1, owned_texture_frame_registry.items[slot_index].generation);
    }

    const generation = runtime_module.nextHandleGeneration();
    try owned_texture_frame_free_list.ensureTotalCapacity(std.heap.smp_allocator, owned_texture_frame_registry.items.len + 1);
    try owned_texture_frame_registry.append(std.heap.smp_allocator, .{ .state = frame_state_handle, .generation = generation });
    return ownedTextureFrameHandle(T, owned_texture_frame_registry.items.len, generation);
}

fn ownedTextureFrameHandle(comptime T: type, index: usize, generation: u64) T {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn ownedTextureFrameHandleIndex(handle_value: u128) ?usize {
    const index = handle_value & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn ownedTextureFrameHandleGeneration(handle_value: u128) u64 {
    return @intCast(handle_value >> 64);
}

fn ownedTextureFrameLease(handle: anytype, kind: OwnedTextureFrameKind) status.BindingError!OwnedTextureFrameLease {
    lockOwnedTextureFrameRegistry();
    defer unlockOwnedTextureFrameRegistry();

    const handle_value = @intFromEnum(handle);
    const index = ownedTextureFrameHandleIndex(handle_value) orelse return error.ClosedHandle;
    if (index > owned_texture_frame_registry.items.len) return error.ClosedHandle;
    const slot = owned_texture_frame_registry.items[index - 1];
    if (slot.generation != ownedTextureFrameHandleGeneration(handle_value)) return error.ClosedHandle;
    const frame_state_handle = slot.state orelse return error.ClosedHandle;
    if (frame_state_handle.kind != kind) return error.ClosedHandle;
    if (frame_state_handle.closing) return error.ActiveBorrow;
    const session_native: *c.mln_render_session = @ptrCast(frame_state_handle.session_native orelse return error.ClosedHandle);
    const session_frame_state = frame_state_handle.frame_state orelse return error.ClosedHandle;
    _ = frame_state_handle.active_leases.fetchAdd(1, .seq_cst);
    return .{
        .state = frame_state_handle,
        .session_native = session_native,
        .diagnostic_store = frame_state_handle.diagnostic_store,
        .frame_state = session_frame_state,
        .generation = frame_state_handle.generation,
    };
}

fn beginOwnedTextureFrameClose(handle: anytype, kind: OwnedTextureFrameKind) status.BindingError!?OwnedTextureFrameLease {
    lockOwnedTextureFrameRegistry();
    defer unlockOwnedTextureFrameRegistry();

    const handle_value = @intFromEnum(handle);
    const index = ownedTextureFrameHandleIndex(handle_value) orelse return null;
    if (index > owned_texture_frame_registry.items.len) return null;
    const slot = owned_texture_frame_registry.items[index - 1];
    if (slot.generation != ownedTextureFrameHandleGeneration(handle_value)) return null;
    const frame_state_handle = slot.state orelse return null;
    if (frame_state_handle.kind != kind) return null;
    if (frame_state_handle.closing) return error.ActiveBorrow;
    if (frame_state_handle.active_leases.load(.seq_cst) != 0) return error.ActiveBorrow;
    const session_native: *c.mln_render_session = @ptrCast(frame_state_handle.session_native orelse return null);
    const session_frame_state = frame_state_handle.frame_state orelse return null;
    frame_state_handle.closing = true;
    return .{
        .state = frame_state_handle,
        .session_native = session_native,
        .diagnostic_store = frame_state_handle.diagnostic_store,
        .frame_state = session_frame_state,
        .generation = frame_state_handle.generation,
    };
}

fn cancelOwnedTextureFrameClose(frame_state_handle: *OwnedTextureFrameState) void {
    lockOwnedTextureFrameRegistry();
    defer unlockOwnedTextureFrameRegistry();

    frame_state_handle.closing = false;
}

fn unregisterOwnedTextureFrameState(handle: anytype) ?*OwnedTextureFrameState {
    lockOwnedTextureFrameRegistry();
    defer unlockOwnedTextureFrameRegistry();

    const handle_value = @intFromEnum(handle);
    const index = ownedTextureFrameHandleIndex(handle_value) orelse return null;
    if (index > owned_texture_frame_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &owned_texture_frame_registry.items[slot_index];
    if (slot.generation != ownedTextureFrameHandleGeneration(handle_value)) return null;
    const frame_state_handle = slot.state orelse return null;
    slot.state = null;
    slot.generation = runtime_module.nextHandleGeneration();
    frame_state_handle.session_native = null;
    frame_state_handle.frame_state = null;
    owned_texture_frame_free_list.appendAssumeCapacity(slot_index);
    return frame_state_handle;
}

fn lockOwnedTextureFrameRegistry() void {
    while (owned_texture_frame_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockOwnedTextureFrameRegistry() void {
    owned_texture_frame_registry_lock.store(false, .seq_cst);
}

fn ensureNoActiveOwnedFrame(lease: RenderSessionLease) status.BindingError!void {
    const session_frame_state = lease.frame_state;
    if (session_frame_state.metal_frame_active or session_frame_state.opengl_frame_active or session_frame_state.vulkan_frame_active) return error.ActiveBorrow;
}

fn metalFrame(lease: OwnedTextureFrameLease) status.BindingError!c.mln_metal_owned_texture_frame {
    const session_frame_state = lease.frame_state;
    if (!session_frame_state.metal_frame_active or lease.generation != session_frame_state.metal_frame_generation) return error.ClosedHandle;
    return (session_frame_state.metal_frame orelse return error.ClosedHandle).raw;
}

fn vulkanFrame(lease: OwnedTextureFrameLease) status.BindingError!c.mln_vulkan_owned_texture_frame {
    const session_frame_state = lease.frame_state;
    if (!session_frame_state.vulkan_frame_active or lease.generation != session_frame_state.vulkan_frame_generation) return error.ClosedHandle;
    return (session_frame_state.vulkan_frame orelse return error.ClosedHandle).raw;
}

fn openglFrame(lease: OwnedTextureFrameLease) status.BindingError!c.mln_opengl_owned_texture_frame {
    const session_frame_state = lease.frame_state;
    if (!session_frame_state.opengl_frame_active or lease.generation != session_frame_state.opengl_frame_generation) return error.ClosedHandle;
    return (session_frame_state.opengl_frame orelse return error.ClosedHandle).raw;
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
        .device = if (context.device) |device| device.toPtr() else null,
    };
}

fn vulkanContextToNative(context: VulkanContextDescriptor) c.mln_vulkan_context_descriptor {
    return .{
        .size = @sizeOf(c.mln_vulkan_context_descriptor),
        .instance = context.instance.toPtr(),
        .physical_device = context.physical_device.toPtr(),
        .device = context.device.toPtr(),
        .graphics_queue = context.graphics_queue.toPtr(),
        .graphics_queue_family_index = context.graphics_queue_family_index,
        .get_instance_proc_addr = if (context.get_instance_proc_addr) |pointer| pointer.toPtr() else null,
        .get_device_proc_addr = if (context.get_device_proc_addr) |pointer| pointer.toPtr() else null,
    };
}

fn openglContextToNative(context: OpenGLContextDescriptor) c.mln_opengl_context_descriptor {
    var raw = c.mln_opengl_context_descriptor{
        .size = @sizeOf(c.mln_opengl_context_descriptor),
        .platform = 0,
        .data = undefined,
    };
    switch (context) {
        .wgl => |wgl| {
            raw.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WGL;
            raw.data.wgl = .{
                .size = @sizeOf(c.mln_wgl_context_descriptor),
                .device_context = wgl.device_context.toPtr(),
                .share_context = wgl.share_context.toPtr(),
                .get_proc_address = if (wgl.get_proc_address) |pointer| pointer.toPtr() else null,
            };
        },
        .egl => |egl| {
            raw.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_EGL;
            raw.data.egl = .{
                .size = @sizeOf(c.mln_egl_context_descriptor),
                .display = egl.display.toPtr(),
                .config = egl.config.toPtr(),
                .share_context = egl.share_context.toPtr(),
                .get_proc_address = if (egl.get_proc_address) |pointer| pointer.toPtr() else null,
            };
        },
    }
    return raw;
}

test "feature identifier copy rejects unknown native tags" {
    var feature = std.mem.zeroes(c.mln_feature);
    feature.identifier_type = 0xbeef;
    try std.testing.expectError(error.UnknownStatus, copyFeatureIdentifier(std.testing.allocator, feature));
}
