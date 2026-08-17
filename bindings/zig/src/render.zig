const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const map_module = @import("map.zig");
const native_temp = @import("native_temp.zig");
const runtime_module = @import("runtime.zig");
const status = @import("status.zig");
const values = @import("values.zig");

const RenderSessionState = struct {
    map_handle: ?map_module.MapHandle,
    runtime: runtime_module.RuntimeHandle,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    active_leases: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    closing: bool = false,
};

const RenderSessionLease = struct {
    state: *RenderSessionState,
    native: c.mln_render_session,
    runtime: runtime_module.RuntimeHandle,
    diagnostic_store: ?*diagnostics.DiagnosticStore,

    fn release(self: RenderSessionLease) void {
        _ = self.state.active_leases.fetchSub(1, .seq_cst);
    }
};

const RenderSessionClose = struct {
    state: *RenderSessionState,
    native: c.mln_render_session,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

var render_session_registry_lock = std.atomic.Value(bool).init(false);
var render_session_registry: std.AutoHashMapUnmanaged(c.mln_render_session, *RenderSessionState) = .empty;

pub const NativePointer = enum(usize) {
    _,

    /// Creates a borrowed backend-native pointer. The caller keeps the backend
    /// object valid and synchronized for the borrow window documented by the
    /// descriptor that receives it.
    pub fn fromPtr(ptr: *anyopaque) NativePointer {
        return @enumFromInt(@intFromPtr(ptr));
    }

    /// Returns the borrowed backend-native address. The pointer grants no
    /// ownership and is valid only for the borrow window documented by the
    /// operation that produced or accepts this value.
    pub fn toPtr(self: NativePointer) *anyopaque {
        return @ptrFromInt(@intFromEnum(self));
    }
};

pub const RenderDriver = enum {
    core_worker,
    caller_graphics_thread,

    fn toRaw(self: RenderDriver) u32 {
        return switch (self) {
            .core_worker => c.MLN_RENDER_DRIVER_CORE_WORKER,
            .caller_graphics_thread => c.MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
        };
    }
};

pub const RenderSessionAttachOptions = struct {
    driver: RenderDriver,
    requested_texture_ring_depth: u32 = 1,
};

pub const RenderResult = union(enum) {
    rendered,
    no_update,
    size_pending,
    target_not_ready,
    superseded,
    deadline_missed,
    unknown: u32,

    fn fromRaw(raw: u32) RenderResult {
        return switch (raw) {
            c.MLN_RENDER_RESULT_RENDERED => .rendered,
            c.MLN_RENDER_RESULT_NO_UPDATE => .no_update,
            c.MLN_RENDER_RESULT_SIZE_PENDING => .size_pending,
            c.MLN_RENDER_RESULT_TARGET_NOT_READY => .target_not_ready,
            c.MLN_RENDER_RESULT_SUPERSEDED => .superseded,
            c.MLN_RENDER_RESULT_DEADLINE_MISSED => .deadline_missed,
            else => .{ .unknown = raw },
        };
    }
};

pub const FrameDemand = struct {
    if_needed: bool = true,
    present: bool = false,
    token: u64 = 0,
    coalescing_boundary: u64 = 0,
    deadline_ns: i64 = 0,
};

pub const FrameResult = struct {
    disposition: RenderResult,
    token: u64,
    map_update_generation: u64,
    extent_generation: u64,
    frame_generation: u64,
    /// Whether the map asked for another frame while it rendered this one, as
    /// during an ongoing camera transition. Set only when `disposition` is
    /// `.rendered`, and false for every other outcome.
    needs_repaint: bool,
};

pub const RenderSessionCapabilities = struct {
    driver: RenderDriver,
    texture_ring_depth: u32,
    frame_acquisition: bool,
    readback: bool,
    consumer_sync: bool,
    presentation: bool,
};
pub const RenderSessionLifecycle = union(enum) {
    attaching,
    attached,
    detaching,
    detached,
    target_lost,
    abandoned,
    unknown: u32,
};

pub const RenderSessionSnapshot = struct {
    state: RenderSessionLifecycle,
    driver: RenderDriver,
    latest_result: RenderResult,
    extent: RenderTargetExtent,
    generation: u64,
    map_update_generation: u64,
    rendered_update_generation: u64,
    extent_generation: u64,
    frame_generation: u64,
    latest_demand_token: u64,
    pending_demand_count: u32,
    acquired_frame_count: u32,
    target_ready: bool,
    pending_changes: bool,
};

pub const GpuSync = union(enum) {
    cpu_complete,
    metal_shared_event: struct { object: NativePointer, value: u64 },
    vulkan_timeline_semaphore: struct { object: NativePointer, value: u64 },
    opengl_fence: NativePointer,
    webgpu_token: struct { object: NativePointer, value: u64 },
};

pub const AbandonDisposition = union(enum) {
    clean,
    quarantined,
    unknown: u32,
};

pub const AbandonResult = struct {
    disposition: AbandonDisposition,
    quarantined_resource_count: u32,
};

pub const RenderSessionAttachment = struct {
    session: RenderSessionHandle,
    operation: runtime_module.OperationHandle,
};

pub const RenderBackendSupport = struct {
    metal: bool,
    opengl: bool,
    vulkan: bool,
    webgpu: bool,
};

pub fn supportedRenderBackends() RenderBackendSupport {
    const mask = c.mln_supported_render_backend_mask();
    return .{
        .metal = (mask & c.MLN_RENDER_BACKEND_FLAG_METAL) != 0,
        .opengl = (mask & c.MLN_RENDER_BACKEND_FLAG_OPENGL) != 0,
        .vulkan = (mask & c.MLN_RENDER_BACKEND_FLAG_VULKAN) != 0,
        .webgpu = (mask & c.MLN_RENDER_BACKEND_FLAG_WEBGPU) != 0,
    };
}

pub const OpenGLContextProviderSupport = struct {
    wgl: bool,
    egl: bool,
    webgl: bool,
};

pub fn supportedOpenGLContextProviders() OpenGLContextProviderSupport {
    const mask = c.mln_opengl_supported_context_provider_mask();
    return .{
        .wgl = (mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL) != 0,
        .egl = (mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL) != 0,
        .webgl = (mask & c.MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL) != 0,
    };
}

pub const RenderTargetExtent = struct {
    width: u32 = 512,
    height: u32 = 512,
    scale_factor: f64 = 1.0,

    /// Returns the physical device-pixel size as `ceil(logical * scale_factor)`
    /// per dimension. Session-owned texture targets and surface targets are
    /// sized this way; borrowed texture targets state their physical size.
    pub fn physicalSize(
        self: RenderTargetExtent,
        diagnostic_store: ?*diagnostics.DiagnosticStore,
    ) status.Error!struct { width: u32, height: u32 } {
        const raw = renderTargetExtentToNative(self);
        var width: u32 = 0;
        var height: u32 = 0;
        try status.checkStatus(
            c.mln_render_target_extent_physical_size(&raw, &width, &height),
            diagnostic_store,
        );
        return .{ .width = width, .height = height };
    }
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
pub const WebGPUContextDescriptor = struct {
    instance: ?NativePointer = null,
    device: NativePointer,
    queue: ?NativePointer = null,
};

/// How a render session's OpenGL context relates to its driver thread and host
/// graphics state.
pub const OpenGLContextOwnership = union(enum) {
    /// The session shares its thread with host graphics work. Every render
    /// makes the session context current and restores whatever was current
    /// before, and the session context joins the share group that the
    /// descriptor names.
    shared,
    /// The session owns its driver thread's OpenGL context. It keeps the
    /// context current between renders and joins no share group. The driver may
    /// be a native core worker or a dedicated host thread.
    dedicated,
    unknown: u32,

    fn toRaw(self: OpenGLContextOwnership) u32 {
        return switch (self) {
            .shared => c.MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED,
            .dedicated => c.MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED,
            .unknown => |raw| raw,
        };
    }
};

/// OpenGL client API that a dedicated EGL session creates its context for.
pub const OpenGLClientApi = union(enum) {
    /// No client API is named.
    unspecified,
    /// Desktop OpenGL, as `EGL_OPENGL_API` names it.
    gl,
    /// OpenGL ES, as `EGL_OPENGL_ES_API` names it.
    gles,
    unknown: u32,

    fn toRaw(self: OpenGLClientApi) u32 {
        return switch (self) {
            .unspecified => c.MLN_OPENGL_CLIENT_API_UNSPECIFIED,
            .gl => c.MLN_OPENGL_CLIENT_API_GL,
            .gles => c.MLN_OPENGL_CLIENT_API_GLES,
            .unknown => |raw| raw,
        };
    }
};

pub const WglContextDescriptor = struct {
    device_context: NativePointer,
    /// Borrowed `HGLRC` whose share group the session context joins. Required
    /// under shared ownership. A dedicated session joins no share group, so it
    /// must be null there.
    share_context: ?NativePointer = null,
    /// Whether the session shares its thread with host graphics work.
    ownership: OpenGLContextOwnership = .shared,
    get_proc_address: ?NativePointer = null,
};

pub const EglContextDescriptor = struct {
    display: NativePointer,
    config: NativePointer,
    /// Borrowed `EGLContext` whose share group the session context joins.
    /// Required under shared ownership, where the session also takes its client
    /// API from this context. A dedicated session joins no share group, so it
    /// must be null there and names `client_api` instead.
    share_context: ?NativePointer = null,
    /// Client API that the session creates its context for. Required under
    /// dedicated ownership. A shared session queries `share_context` for it, so
    /// this is ignored there.
    client_api: OpenGLClientApi = .unspecified,
    /// Whether the session shares its thread with host graphics work.
    ownership: OpenGLContextOwnership = .shared,
    get_proc_address: ?NativePointer = null,
};
pub const WebGLContextDescriptor = union(enum) {
    existing: i32,
    transferred_canvas: []const u8,
};

pub const OpenGLContextDescriptor = union(enum) {
    wgl: WglContextDescriptor,
    egl: EglContextDescriptor,
    webgl: WebGLContextDescriptor,
};

pub const MetalOwnedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: MetalContextDescriptor,
};

pub const MetalBorrowedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    /// Physical texture size in device pixels, set by the texture's owner.
    physical_width: u32,
    physical_height: u32,
    texture: NativePointer,
};

pub const VulkanOwnedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: VulkanContextDescriptor,
};

pub const VulkanBorrowedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    /// Physical image size in device pixels, set by the image's owner.
    physical_width: u32,
    physical_height: u32,
    context: VulkanContextDescriptor,
    image: NativePointer,
    image_view: NativePointer,
    format: u32,
    initial_layout: u32,
    final_layout: u32,
};

pub const WebGPUOwnedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: WebGPUContextDescriptor,
};

pub const WebGPUBorrowedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    physical_width: u32,
    physical_height: u32,
    context: WebGPUContextDescriptor,
    texture: NativePointer,
    texture_view: NativePointer,
    format: u32,
};

pub const OpenGLOwnedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: OpenGLContextDescriptor,
};

pub const OpenGLBorrowedTextureDescriptor = struct {
    extent: RenderTargetExtent = .{},
    /// Physical texture size in device pixels, set by the texture's owner.
    physical_width: u32,
    physical_height: u32,
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

pub const WebGPUSurfaceDescriptor = struct {
    extent: RenderTargetExtent = .{},
    context: WebGPUContextDescriptor,
    surface: NativePointer,
    format: u32,
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

/// Screen-space box in logical map pixels. Corners may be given in any order
/// and may extend past the viewport; rendered queries normalize and clip them.
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
    filter: ?[]const u8 = null,
};

pub const SourceFeatureQueryOptions = struct {
    source_layer_ids: ?[]const []const u8 = null,
    filter: ?[]const u8 = null,
};

/// One copied query hit. `feature` is a GeoJSON Feature. `source_id`,
/// `source_layer_id`, and `state` are present when the native result set those
/// fields. `state` is a JSON object.
pub const QueriedFeature = struct {
    allocator: std.mem.Allocator,
    feature: []const u8,
    source_id: ?[]const u8,
    source_layer_id: ?[]const u8,
    state: ?[]const u8,

    pub fn deinit(self: *QueriedFeature) void {
        self.allocator.free(self.feature);
        if (self.source_id) |value| self.allocator.free(value);
        if (self.source_layer_id) |value| self.allocator.free(value);
        if (self.state) |value| self.allocator.free(value);
        self.feature = "";
        self.source_id = null;
        self.source_layer_id = null;
        self.state = null;
    }

    pub fn eql(self: QueriedFeature, other: QueriedFeature) bool {
        return std.mem.eql(u8, self.feature, other.feature) and
            optionalSliceEql(self.source_id, other.source_id) and
            optionalSliceEql(self.source_layer_id, other.source_layer_id) and
            optionalSliceEql(self.state, other.state);
    }
};

pub const QueriedFeatureList = struct {
    allocator: std.mem.Allocator,
    items: []QueriedFeature,

    pub fn deinit(self: *QueriedFeatureList) void {
        for (self.items) |*item| item.deinit();
        self.allocator.free(self.items);
        self.items = &.{};
    }

    pub fn eql(self: QueriedFeatureList, other: QueriedFeatureList) bool {
        if (self.items.len != other.items.len) return false;
        for (self.items, other.items) |left, right| {
            if (!left.eql(right)) return false;
        }
        return true;
    }
};

fn optionalSliceEql(left: ?[]const u8, right: ?[]const u8) bool {
    const left_value = left orelse return right == null;
    const right_value = right orelse return false;
    return std.mem.eql(u8, left_value, right_value);
}

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
pub const WebGPUOwnedTextureFrameInfo = struct {
    generation: u64,
    width: u32,
    height: u32,
    scale_factor: f64,
    texture: NativePointer,
    texture_view: NativePointer,
    device: NativePointer,
    format: u32,
};

pub const RenderFrameBatch = struct {
    native: c.mln_render_frame_batch,
    diagnostic_store: ?*diagnostics.DiagnosticStore,

    pub fn count(self: RenderFrameBatch) status.Error!usize {
        var result: usize = 0;
        try status.checkStatus(c.mln_render_frame_batch_count(self.native, &result), self.diagnostic_store);
        return result;
    }

    pub fn get(self: RenderFrameBatch, index: usize) status.Error!FrameResult {
        var raw: c.mln_render_frame_result = undefined;
        raw.size = @sizeOf(c.mln_render_frame_result);
        try status.checkStatus(c.mln_render_frame_batch_get(self.native, index, &raw), self.diagnostic_store);
        return frameResultFromNative(raw);
    }

    pub fn release(self: *RenderFrameBatch) void {
        if (self.native == 0) return;
        c.mln_render_frame_batch_release(self.native);
        self.native = 0;
    }
};

pub const AcquiredFrame = struct {
    native: c.mln_acquired_frame,
    session: RenderSessionHandle,

    pub fn result(self: AcquiredFrame) status.Error!FrameResult {
        const lease = try renderSessionLease(self.session);
        defer lease.release();
        var raw: c.mln_render_frame_result = undefined;
        raw.size = @sizeOf(c.mln_render_frame_result);
        try status.checkStatus(c.mln_acquired_frame_get_result(self.native, &raw), lease.diagnostic_store);
        return frameResultFromNative(raw);
    }

    pub fn producerSync(self: AcquiredFrame) status.Error!GpuSync {
        const lease = try renderSessionLease(self.session);
        defer lease.release();
        var raw = gpuSyncToNative(.cpu_complete);
        try status.checkStatus(c.mln_acquired_frame_get_producer_sync(self.native, &raw), lease.diagnostic_store);
        return gpuSyncFromNative(raw);
    }

    pub fn metalTexture(self: AcquiredFrame) status.Error!MetalOwnedTextureFrameInfo {
        const lease = try renderSessionLease(self.session);
        defer lease.release();
        var raw: c.mln_metal_owned_texture_frame = undefined;
        raw.size = @sizeOf(c.mln_metal_owned_texture_frame);
        try status.checkStatus(c.mln_acquired_frame_get_metal_texture(self.native, &raw), lease.diagnostic_store);
        return .{ .generation = raw.generation, .width = raw.width, .height = raw.height, .scale_factor = raw.scale_factor, .texture = NativePointer.fromPtr(raw.texture orelse return error.ClosedHandle), .device = NativePointer.fromPtr(raw.device orelse return error.ClosedHandle), .pixel_format = raw.pixel_format };
    }

    pub fn vulkanTexture(self: AcquiredFrame) status.Error!VulkanOwnedTextureFrameInfo {
        const lease = try renderSessionLease(self.session);
        defer lease.release();
        var raw: c.mln_vulkan_owned_texture_frame = undefined;
        raw.size = @sizeOf(c.mln_vulkan_owned_texture_frame);
        try status.checkStatus(c.mln_acquired_frame_get_vulkan_texture(self.native, &raw), lease.diagnostic_store);
        return .{ .generation = raw.generation, .width = raw.width, .height = raw.height, .scale_factor = raw.scale_factor, .image = NativePointer.fromPtr(raw.image orelse return error.ClosedHandle), .image_view = NativePointer.fromPtr(raw.image_view orelse return error.ClosedHandle), .device = NativePointer.fromPtr(raw.device orelse return error.ClosedHandle), .format = raw.format, .layout = raw.layout };
    }

    pub fn openGLTexture(self: AcquiredFrame) status.Error!OpenGLOwnedTextureFrameInfo {
        const lease = try renderSessionLease(self.session);
        defer lease.release();
        var raw: c.mln_opengl_owned_texture_frame = undefined;
        raw.size = @sizeOf(c.mln_opengl_owned_texture_frame);
        try status.checkStatus(c.mln_acquired_frame_get_opengl_texture(self.native, &raw), lease.diagnostic_store);
        return .{ .generation = raw.generation, .width = raw.width, .height = raw.height, .scale_factor = raw.scale_factor, .texture = raw.texture, .target = raw.target, .internal_format = raw.internal_format, .format = raw.format, .type = raw.type };
    }

    pub fn webGPUTexture(self: AcquiredFrame) status.Error!WebGPUOwnedTextureFrameInfo {
        const lease = try renderSessionLease(self.session);
        defer lease.release();
        var raw: c.mln_webgpu_owned_texture_frame = undefined;
        raw.size = @sizeOf(c.mln_webgpu_owned_texture_frame);
        try status.checkStatus(c.mln_acquired_frame_get_webgpu_texture(self.native, &raw), lease.diagnostic_store);
        return .{ .generation = raw.generation, .width = raw.width, .height = raw.height, .scale_factor = raw.scale_factor, .texture = NativePointer.fromPtr(raw.texture orelse return error.ClosedHandle), .texture_view = NativePointer.fromPtr(raw.texture_view orelse return error.ClosedHandle), .device = NativePointer.fromPtr(raw.device orelse return error.ClosedHandle), .format = raw.format };
    }

    pub fn releaseStart(self: *AcquiredFrame, consumer_completion: GpuSync) status.Error!runtime_module.OperationHandle {
        const lease = try renderSessionLease(self.session);
        defer lease.release();
        var sync = gpuSyncToNative(consumer_completion);
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_acquired_frame_release_start(&self.native, &sync, &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .acquired_frame_release, .none);
    }
};

pub const OwnedReadback = struct {
    allocator: std.mem.Allocator,
    data: []u8,
    info: TextureImageInfo,

    pub fn deinit(self: *OwnedReadback) void {
        self.allocator.free(self.data);
        self.data = &.{};
    }
};

pub const RenderSessionHandle = enum(c.mln_render_session) {
    _,

    pub fn capabilities(self: RenderSessionHandle) status.Error!RenderSessionCapabilities {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var raw: c.mln_render_session_capabilities = undefined;
        raw.size = @sizeOf(c.mln_render_session_capabilities);
        try status.checkStatus(c.mln_render_session_get_capabilities(lease.native, &raw), lease.diagnostic_store);
        return .{
            .driver = try driverFromRaw(raw.driver),
            .texture_ring_depth = raw.texture_ring_depth,
            .frame_acquisition = (raw.flags & c.MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION) != 0,
            .readback = (raw.flags & c.MLN_RENDER_SESSION_CAPABILITY_READBACK) != 0,
            .consumer_sync = (raw.flags & c.MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC) != 0,
            .presentation = (raw.flags & c.MLN_RENDER_SESSION_CAPABILITY_PRESENTATION) != 0,
        };
    }

    pub fn snapshot(self: RenderSessionHandle) status.Error!RenderSessionSnapshot {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var raw: c.mln_render_session_snapshot = undefined;
        raw.size = @sizeOf(c.mln_render_session_snapshot);
        try status.checkStatus(c.mln_render_session_get_snapshot(lease.native, &raw), lease.diagnostic_store);
        return .{
            .state = lifecycleFromRaw(raw.state),
            .driver = try driverFromRaw(raw.driver),
            .latest_result = RenderResult.fromRaw(raw.latest_result),
            .extent = renderTargetExtentFromNative(raw.extent),
            .generation = raw.generation,
            .map_update_generation = raw.map_update_generation,
            .rendered_update_generation = raw.rendered_update_generation,
            .extent_generation = raw.extent_generation,
            .frame_generation = raw.frame_generation,
            .latest_demand_token = raw.latest_demand_token,
            .pending_demand_count = raw.pending_demand_count,
            .acquired_frame_count = raw.acquired_frame_count,
            .target_ready = raw.target_ready,
            .pending_changes = raw.pending_changes,
        };
    }

    pub fn requestFrame(self: RenderSessionHandle, demand: FrameDemand) status.Error!void {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var raw = frameDemandToNative(demand);
        try status.checkStatus(c.mln_render_session_request_frame(lease.native, &raw), lease.diagnostic_store);
    }

    pub fn drainFrameResults(self: RenderSessionHandle) status.Error!RenderFrameBatch {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var batch: c.mln_render_frame_batch = 0;
        try status.checkStatus(c.mln_render_session_drain_frame_results(lease.native, &batch), lease.diagnostic_store);
        return .{ .native = batch, .diagnostic_store = lease.diagnostic_store };
    }

    pub fn acquireFrame(self: RenderSessionHandle) status.Error!AcquiredFrame {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var frame: c.mln_acquired_frame = 0;
        try status.checkStatus(c.mln_render_session_acquire_frame(lease.native, &frame), lease.diagnostic_store);
        return .{ .native = frame, .session = self };
    }

    /// This is the only graphics-thread-affine session method.
    pub fn serviceDriverWork(self: RenderSessionHandle, max_work: usize) status.Error!usize {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var serviced: usize = 0;
        try status.checkStatus(c.mln_render_session_service_driver_work(lease.native, max_work, &serviced), lease.diagnostic_store);
        return serviced;
    }

    pub fn resizeStart(self: RenderSessionHandle, extent: RenderTargetExtent) status.Error!runtime_module.OperationHandle {
        var raw = renderTargetExtentToNative(extent);
        return startArgumentOperation(self, c.mln_render_session_resize_start, .render_resize, &raw);
    }

    pub fn barrierStart(self: RenderSessionHandle, min_update_generation: u64) status.Error!runtime_module.OperationHandle {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_render_session_barrier_start(lease.native, min_update_generation, &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_barrier, .none);
    }

    pub fn reduceMemoryUseStart(self: RenderSessionHandle) status.Error!runtime_module.OperationHandle {
        return startSimpleOperation(self, c.mln_render_session_reduce_memory_use_start, .render_reduce_memory_use);
    }

    pub fn clearDataStart(self: RenderSessionHandle) status.Error!runtime_module.OperationHandle {
        return startSimpleOperation(self, c.mln_render_session_clear_data_start, .render_clear_data);
    }

    pub fn dumpDebugLogsStart(self: RenderSessionHandle) status.Error!runtime_module.OperationHandle {
        return startSimpleOperation(self, c.mln_render_session_dump_debug_logs_start, .render_dump_debug_logs);
    }

    pub fn setFeatureStateStart(self: RenderSessionHandle, allocator: std.mem.Allocator, selector: FeatureStateSelector, state_json: []const u8) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_render_session_set_feature_state_start(lease.native, try temp.stringView(selector.source_id), try optionalStringView(&temp, selector.source_layer_id), try optionalStringView(&temp, selector.feature_id), try temp.stringView(state_json), &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_set_feature_state, .none);
    }

    pub fn getFeatureStateStart(self: RenderSessionHandle, allocator: std.mem.Allocator, selector: FeatureStateSelector) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_render_session_get_feature_state_start(lease.native, try temp.stringView(selector.source_id), try optionalStringView(&temp, selector.source_layer_id), try optionalStringView(&temp, selector.feature_id), &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_get_feature_state, .string);
    }

    pub fn takeFeatureState(self: RenderSessionHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!values.OwnedString {
        const lease = try renderSessionLease(self);
        defer lease.release();
        const required = try operation.require(&lease.runtime, .render_get_feature_state, .string);
        defer required.lease.release();
        var buffer: c.mln_buffer = 0;
        try status.checkStatus(c.mln_render_session_get_feature_state_take_result(required.lease.native, &buffer), lease.diagnostic_store);
        return (try native_temp.copyOwnedBuffer(allocator, buffer, lease.diagnostic_store)) orelse error.NativeError;
    }

    pub fn removeFeatureStateStart(self: RenderSessionHandle, allocator: std.mem.Allocator, selector: FeatureStateSelector) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_render_session_remove_feature_state_start(lease.native, try temp.stringView(selector.source_id), try optionalStringView(&temp, selector.source_layer_id), try optionalStringView(&temp, selector.feature_id), try optionalStringView(&temp, selector.state_key), &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_remove_feature_state, .none);
    }

    pub fn queryRenderedFeaturesStart(self: RenderSessionHandle, allocator: std.mem.Allocator, geometry: RenderedQueryGeometry, options: ?RenderedFeatureQueryOptions) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_geometry = try renderedQueryGeometryToNative(&temp, geometry);
        var raw_options = if (options) |value| try renderedFeatureQueryOptionsToNative(&temp, value) else undefined;
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_render_session_query_rendered_features_start(lease.native, &raw_geometry, if (options != null) &raw_options else null, &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_query, .queried_feature_list);
    }

    pub fn querySourceFeaturesStart(self: RenderSessionHandle, allocator: std.mem.Allocator, source_id: []const u8, options: ?SourceFeatureQueryOptions) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_options = if (options) |value| try sourceFeatureQueryOptionsToNative(&temp, value) else undefined;
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_render_session_query_source_features_start(lease.native, try temp.stringView(source_id), if (options != null) &raw_options else null, &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_query, .queried_feature_list);
    }

    pub fn takeQueryFeaturesResult(self: RenderSessionHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!QueriedFeatureList {
        const lease = try renderSessionLease(self);
        defer lease.release();
        const required = try operation.require(&lease.runtime, .render_query, .queried_feature_list);
        defer required.lease.release();
        var result: c.mln_queried_feature_list = 0;
        try status.checkStatus(c.mln_render_query_features_take_result(required.lease.native, &result), lease.diagnostic_store);
        defer c.mln_queried_feature_list_destroy(result);
        return try copyQueriedFeatureList(allocator, result, lease.diagnostic_store);
    }

    pub fn queryFeatureExtensionStart(self: RenderSessionHandle, allocator: std.mem.Allocator, source_id: []const u8, feature: []const u8, extension: []const u8, extension_field: []const u8, arguments: ?[]const u8) status.Error!runtime_module.OperationHandle {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        var raw_arguments = if (arguments) |value| try temp.stringView(value) else undefined;
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_render_session_query_feature_extensions_start(
            lease.native,
            try temp.stringView(source_id),
            try temp.stringView(feature),
            try temp.stringView(extension),
            try temp.stringView(extension_field),
            if (arguments != null) &raw_arguments else null,
            &operation,
        ), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_query, .string);
    }

    pub fn takeQueryResult(self: RenderSessionHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!values.OwnedString {
        const lease = try renderSessionLease(self);
        defer lease.release();
        const required = try operation.require(&lease.runtime, .render_query, .string);
        defer required.lease.release();
        var buffer: c.mln_buffer = 0;
        try status.checkStatus(c.mln_render_query_take_result(required.lease.native, &buffer), lease.diagnostic_store);
        return (try native_temp.copyOwnedBuffer(allocator, buffer, lease.diagnostic_store)) orelse error.NativeError;
    }

    pub fn readbackStart(self: RenderSessionHandle) status.Error!runtime_module.OperationHandle {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(c.mln_texture_read_premultiplied_rgba8_start(lease.native, &operation), lease.diagnostic_store);
        return operationHandle(lease, operation, .render_readback, .render_readback);
    }

    pub fn takeReadback(self: RenderSessionHandle, allocator: std.mem.Allocator, operation: runtime_module.OperationHandle) status.Error!OwnedReadback {
        const lease = try renderSessionLease(self);
        defer lease.release();
        const required = try operation.require(&lease.runtime, .render_readback, .render_readback);
        defer required.lease.release();
        var buffer: c.mln_buffer = 0;
        var info = c.mln_texture_image_info_default();
        try status.checkStatus(c.mln_texture_read_premultiplied_rgba8_take_result(required.lease.native, &buffer, &info), lease.diagnostic_store);
        const data = try copyNativeBuffer(allocator, buffer, lease.diagnostic_store);
        return .{ .allocator = allocator, .data = data, .info = textureImageInfoFromNative(info) };
    }

    pub fn setMetalSurfaceTargetStart(self: RenderSessionHandle, descriptor: MetalSurfaceDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = metalSurfaceDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_metal_surface_set_target_start, .render_set_target, &raw);
    }

    pub fn setVulkanSurfaceTargetStart(self: RenderSessionHandle, descriptor: VulkanSurfaceDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = vulkanSurfaceDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_vulkan_surface_set_target_start, .render_set_target, &raw);
    }

    pub fn setOpenGLSurfaceTargetStart(self: RenderSessionHandle, descriptor: OpenGLSurfaceDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = openglSurfaceDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_opengl_surface_set_target_start, .render_set_target, &raw);
    }

    pub fn setMetalBorrowedTextureTargetStart(self: RenderSessionHandle, descriptor: MetalBorrowedTextureDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = metalBorrowedTextureDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_metal_borrowed_texture_set_target_start, .render_set_target, &raw);
    }

    pub fn setVulkanBorrowedTextureTargetStart(self: RenderSessionHandle, descriptor: VulkanBorrowedTextureDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = vulkanBorrowedTextureDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_vulkan_borrowed_texture_set_target_start, .render_set_target, &raw);
    }

    pub fn setOpenGLBorrowedTextureTargetStart(self: RenderSessionHandle, descriptor: OpenGLBorrowedTextureDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = openglBorrowedTextureDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_opengl_borrowed_texture_set_target_start, .render_set_target, &raw);
    }
    pub fn setWebGPUBorrowedTextureTargetStart(self: RenderSessionHandle, descriptor: WebGPUBorrowedTextureDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = webgpuBorrowedTextureDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_webgpu_borrowed_texture_set_target_start, .render_set_target, &raw);
    }

    pub fn setWebGPUSurfaceTargetStart(self: RenderSessionHandle, descriptor: WebGPUSurfaceDescriptor) status.Error!runtime_module.OperationHandle {
        var raw = webgpuSurfaceDescriptorToNative(descriptor);
        return startArgumentOperation(self, c.mln_webgpu_surface_set_target_start, .render_set_target, &raw);
    }

    pub fn detachStart(self: RenderSessionHandle) status.Error!runtime_module.OperationHandle {
        return startSimpleOperation(self, c.mln_render_session_detach_start, .render_detach);
    }

    pub fn abandon(self: RenderSessionHandle) status.Error!AbandonResult {
        const lease = try renderSessionLease(self);
        defer lease.release();
        var raw: c.mln_render_abandon_result = undefined;
        raw.size = @sizeOf(c.mln_render_abandon_result);
        try status.checkStatus(c.mln_render_session_abandon(lease.native, &raw), lease.diagnostic_store);
        if (takeMapRegistration(lease.state)) |map_handle| map_module.unregisterRenderSession(map_handle);
        return .{ .disposition = abandonDispositionFromRaw(raw.disposition), .quarantined_resource_count = raw.quarantined_resource_count };
    }

    pub fn destroy(self: *RenderSessionHandle) status.Error!void {
        const session_close = try beginRenderSessionClose(self.*) orelse return;
        status.checkStatus(c.mln_render_session_destroy(session_close.native), session_close.diagnostic_store) catch |err| {
            cancelRenderSessionClose(session_close.state);
            return err;
        };
        if (takeMapRegistration(session_close.state)) |map_handle| map_module.unregisterRenderSession(map_handle);
        const state = finishRenderSessionClose(self.*) orelse session_close.state;
        if (state.diagnostic_store) |store| {
            store.deinit();
            std.heap.smp_allocator.destroy(store);
        }
        std.heap.smp_allocator.destroy(state);
        self.* = @enumFromInt(0);
    }
};

pub fn attachMetalOwnedTexture(map: *map_module.MapHandle, descriptor: MetalOwnedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = c.mln_metal_owned_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = metalContextToNative(descriptor.context);
    return attach(map, c.mln_metal_owned_texture_attach_start, &raw, options);
}

pub fn attachMetalBorrowedTexture(map: *map_module.MapHandle, descriptor: MetalBorrowedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = metalBorrowedTextureDescriptorToNative(descriptor);
    return attach(map, c.mln_metal_borrowed_texture_attach_start, &raw, options);
}

pub fn attachVulkanOwnedTexture(map: *map_module.MapHandle, descriptor: VulkanOwnedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = c.mln_vulkan_owned_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = vulkanContextToNative(descriptor.context);
    return attach(map, c.mln_vulkan_owned_texture_attach_start, &raw, options);
}

pub fn attachVulkanBorrowedTexture(map: *map_module.MapHandle, descriptor: VulkanBorrowedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = vulkanBorrowedTextureDescriptorToNative(descriptor);
    return attach(map, c.mln_vulkan_borrowed_texture_attach_start, &raw, options);
}

pub fn attachOpenGLOwnedTexture(map: *map_module.MapHandle, descriptor: OpenGLOwnedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = c.mln_opengl_owned_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = openglContextToNative(descriptor.context);
    return attach(map, c.mln_opengl_owned_texture_attach_start, &raw, options);
}

pub fn attachOpenGLBorrowedTexture(map: *map_module.MapHandle, descriptor: OpenGLBorrowedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = openglBorrowedTextureDescriptorToNative(descriptor);
    return attach(map, c.mln_opengl_borrowed_texture_attach_start, &raw, options);
}
pub fn attachWebGPUOwnedTexture(map: *map_module.MapHandle, descriptor: WebGPUOwnedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = c.mln_webgpu_owned_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = webgpuContextToNative(descriptor.context);
    return attach(map, c.mln_webgpu_owned_texture_attach_start, &raw, options);
}

pub fn attachWebGPUBorrowedTexture(map: *map_module.MapHandle, descriptor: WebGPUBorrowedTextureDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = webgpuBorrowedTextureDescriptorToNative(descriptor);
    return attach(map, c.mln_webgpu_borrowed_texture_attach_start, &raw, options);
}

pub fn attachMetalSurface(map: *map_module.MapHandle, descriptor: MetalSurfaceDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = metalSurfaceDescriptorToNative(descriptor);
    return attach(map, c.mln_metal_surface_attach_start, &raw, options);
}

pub fn attachVulkanSurface(map: *map_module.MapHandle, descriptor: VulkanSurfaceDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = vulkanSurfaceDescriptorToNative(descriptor);
    return attach(map, c.mln_vulkan_surface_attach_start, &raw, options);
}

pub fn attachOpenGLSurface(map: *map_module.MapHandle, descriptor: OpenGLSurfaceDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = openglSurfaceDescriptorToNative(descriptor);
    return attach(map, c.mln_opengl_surface_attach_start, &raw, options);
}
pub fn attachWebGPUSurface(map: *map_module.MapHandle, descriptor: WebGPUSurfaceDescriptor, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    var raw = webgpuSurfaceDescriptorToNative(descriptor);
    return attach(map, c.mln_webgpu_surface_attach_start, &raw, options);
}

fn attach(map: *map_module.MapHandle, comptime attachFn: anytype, descriptor: anytype, options: RenderSessionAttachOptions) status.Error!RenderSessionAttachment {
    const registration = try map_module.registerRenderSession(map);
    var raw_options = attachOptionsToNative(options);
    var session: c.mln_render_session = 0;
    var operation: c.mln_operation = 0;
    status.checkStatus(attachFn(registration.native, descriptor, &raw_options, &session, &operation), registration.diagnostic_store) catch |err| {
        map_module.unregisterRenderSession(map.*);
        return err;
    };
    const handle = newRenderSession(session, map.*, registration.runtime, registration.diagnostic_store) catch |err| {
        c.mln_operation_release(operation);
        var abandoned: c.mln_render_abandon_result = undefined;
        abandoned.size = @sizeOf(c.mln_render_abandon_result);
        _ = c.mln_render_session_abandon(session, &abandoned);
        _ = c.mln_render_session_destroy(session);
        map_module.unregisterRenderSession(map.*);
        return err;
    };
    const operation_handle = runtime_module.OperationHandle.init(&registration.runtime, operation, .render_attach, .none) catch |err| {
        c.mln_operation_release(operation);
        var mutable = handle;
        _ = mutable.abandon() catch {};
        mutable.destroy() catch {};
        return err;
    };
    return .{ .session = handle, .operation = operation_handle };
}

fn startSimpleOperation(handle: RenderSessionHandle, comptime startFn: anytype, kind: runtime_module.OperationKind) status.Error!runtime_module.OperationHandle {
    const lease = try renderSessionLease(handle);
    defer lease.release();
    var operation: c.mln_operation = 0;
    try status.checkStatus(startFn(lease.native, &operation), lease.diagnostic_store);
    return operationHandle(lease, operation, kind, .none);
}

fn startArgumentOperation(handle: RenderSessionHandle, comptime startFn: anytype, kind: runtime_module.OperationKind, argument: anytype) status.Error!runtime_module.OperationHandle {
    const lease = try renderSessionLease(handle);
    defer lease.release();
    var operation: c.mln_operation = 0;
    try status.checkStatus(startFn(lease.native, argument, &operation), lease.diagnostic_store);
    return operationHandle(lease, operation, kind, .none);
}

fn operationHandle(lease: RenderSessionLease, operation: c.mln_operation, kind: runtime_module.OperationKind, result_kind: runtime_module.OperationResultKind) status.Error!runtime_module.OperationHandle {
    return runtime_module.OperationHandle.init(&lease.runtime, operation, kind, result_kind) catch |err| {
        c.mln_operation_release(operation);
        return err;
    };
}

fn newRenderSession(session: c.mln_render_session, map_handle: map_module.MapHandle, runtime: runtime_module.RuntimeHandle, diagnostic_store: ?*diagnostics.DiagnosticStore) std.mem.Allocator.Error!RenderSessionHandle {
    const session_store: ?*diagnostics.DiagnosticStore = if (diagnostic_store) |map_store| store: {
        const result = try std.heap.smp_allocator.create(diagnostics.DiagnosticStore);
        result.* = diagnostics.DiagnosticStore.init(map_store.allocator);
        break :store result;
    } else null;
    errdefer if (session_store) |store| {
        store.deinit();
        std.heap.smp_allocator.destroy(store);
    };
    const state = try std.heap.smp_allocator.create(RenderSessionState);
    state.* = .{ .map_handle = map_handle, .runtime = runtime, .diagnostic_store = session_store };
    errdefer std.heap.smp_allocator.destroy(state);
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();
    try render_session_registry.put(std.heap.smp_allocator, session, state);
    return @enumFromInt(session);
}

fn renderSessionLease(handle: RenderSessionHandle) status.BindingError!RenderSessionLease {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();
    const state = render_session_registry.get(@intFromEnum(handle)) orelse return error.ClosedHandle;
    if (state.closing) return error.ActiveBorrow;
    _ = state.active_leases.fetchAdd(1, .seq_cst);
    return .{ .state = state, .native = @intFromEnum(handle), .runtime = state.runtime, .diagnostic_store = state.diagnostic_store };
}

fn beginRenderSessionClose(handle: RenderSessionHandle) status.BindingError!?RenderSessionClose {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();
    const state = render_session_registry.get(@intFromEnum(handle)) orelse return null;
    if (state.closing or state.active_leases.load(.seq_cst) != 0) return error.ActiveBorrow;
    state.closing = true;
    return .{ .state = state, .native = @intFromEnum(handle), .diagnostic_store = state.diagnostic_store };
}

fn takeMapRegistration(state: *RenderSessionState) ?map_module.MapHandle {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();
    const map_handle = state.map_handle orelse return null;
    state.map_handle = null;
    return map_handle;
}

fn cancelRenderSessionClose(state: *RenderSessionState) void {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();
    state.closing = false;
}

fn finishRenderSessionClose(handle: RenderSessionHandle) ?*RenderSessionState {
    lockRenderSessionRegistry();
    defer unlockRenderSessionRegistry();
    return (render_session_registry.fetchRemove(@intFromEnum(handle)) orelse return null).value;
}

fn lockRenderSessionRegistry() void {
    while (render_session_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) std.Thread.yield() catch {};
}

fn unlockRenderSessionRegistry() void {
    render_session_registry_lock.store(false, .seq_cst);
}

fn renderTargetExtentToNative(extent: RenderTargetExtent) c.mln_render_target_extent {
    return .{
        .size = @sizeOf(c.mln_render_target_extent),
        .width = extent.width,
        .height = extent.height,
        .scale_factor = extent.scale_factor,
    };
}

fn lifecycleFromRaw(raw: u32) RenderSessionLifecycle {
    return switch (raw) {
        c.MLN_RENDER_SESSION_STATE_ATTACHING => .attaching,
        c.MLN_RENDER_SESSION_STATE_ATTACHED => .attached,
        c.MLN_RENDER_SESSION_STATE_DETACHING => .detaching,
        c.MLN_RENDER_SESSION_STATE_DETACHED => .detached,
        c.MLN_RENDER_SESSION_STATE_TARGET_LOST => .target_lost,
        c.MLN_RENDER_SESSION_STATE_ABANDONED => .abandoned,
        else => .{ .unknown = raw },
    };
}

fn abandonDispositionFromRaw(raw: u32) AbandonDisposition {
    return switch (raw) {
        c.MLN_RENDER_ABANDON_DISPOSITION_CLEAN => .clean,
        c.MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED => .quarantined,
        else => .{ .unknown = raw },
    };
}
fn renderTargetExtentFromNative(raw: c.mln_render_target_extent) RenderTargetExtent {
    return .{ .width = raw.width, .height = raw.height, .scale_factor = raw.scale_factor };
}

fn driverFromRaw(raw: u32) status.Error!RenderDriver {
    return switch (raw) {
        c.MLN_RENDER_DRIVER_CORE_WORKER => .core_worker,
        c.MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD => .caller_graphics_thread,
        else => error.NativeError,
    };
}

fn attachOptionsToNative(options: RenderSessionAttachOptions) c.mln_render_session_attach_options {
    var raw = c.mln_render_session_attach_options_default();
    raw.driver = options.driver.toRaw();
    raw.requested_texture_ring_depth = options.requested_texture_ring_depth;
    return raw;
}
fn frameDemandToNative(demand: FrameDemand) c.mln_frame_demand {
    var flags: u32 = 0;
    if (demand.if_needed) flags |= c.MLN_FRAME_DEMAND_IF_NEEDED;
    if (demand.present) flags |= c.MLN_FRAME_DEMAND_PRESENT;
    return .{ .size = @sizeOf(c.mln_frame_demand), .flags = flags, .token = demand.token, .coalescing_boundary = demand.coalescing_boundary, .deadline_ns = demand.deadline_ns };
}

fn frameResultFromNative(raw: c.mln_render_frame_result) FrameResult {
    return .{ .disposition = RenderResult.fromRaw(raw.disposition), .token = raw.token, .map_update_generation = raw.map_update_generation, .extent_generation = raw.extent_generation, .frame_generation = raw.frame_generation, .needs_repaint = raw.needs_repaint };
}

fn gpuSyncToNative(sync: GpuSync) c.mln_gpu_sync {
    var raw = c.mln_gpu_sync_default();
    switch (sync) {
        .cpu_complete => {},
        .metal_shared_event => |value| {
            raw.kind = c.MLN_GPU_SYNC_METAL_SHARED_EVENT;
            raw.object = value.object.toPtr();
            raw.value = value.value;
        },
        .vulkan_timeline_semaphore => |value| {
            raw.kind = c.MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE;
            raw.object = value.object.toPtr();
            raw.value = value.value;
        },
        .opengl_fence => |value| {
            raw.kind = c.MLN_GPU_SYNC_OPENGL_FENCE;
            raw.object = value.toPtr();
        },
        .webgpu_token => |value| {
            raw.kind = c.MLN_GPU_SYNC_WEBGPU_TOKEN;
            raw.object = value.object.toPtr();
            raw.value = value.value;
        },
    }
    return raw;
}

fn gpuSyncFromNative(raw: c.mln_gpu_sync) status.Error!GpuSync {
    return switch (raw.kind) {
        c.MLN_GPU_SYNC_CPU_COMPLETE => .cpu_complete,
        c.MLN_GPU_SYNC_METAL_SHARED_EVENT => .{ .metal_shared_event = .{ .object = NativePointer.fromPtr(raw.object orelse return error.NativeError), .value = raw.value } },
        c.MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE => .{ .vulkan_timeline_semaphore = .{ .object = NativePointer.fromPtr(raw.object orelse return error.NativeError), .value = raw.value } },
        c.MLN_GPU_SYNC_OPENGL_FENCE => .{ .opengl_fence = NativePointer.fromPtr(raw.object orelse return error.NativeError) },
        c.MLN_GPU_SYNC_WEBGPU_TOKEN => .{ .webgpu_token = .{ .object = NativePointer.fromPtr(raw.object orelse return error.NativeError), .value = raw.value } },
        else => error.NativeError,
    };
}

fn optionalStringView(temp: *native_temp.TempStorage, value: ?[]const u8) status.Error!c.mln_buffer_view {
    return if (value) |bytes| try temp.stringView(bytes) else .{ .data = null, .size = 0 };
}

fn copyNativeBuffer(allocator: std.mem.Allocator, buffer: c.mln_buffer, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error![]u8 {
    const owned = (try native_temp.copyOwnedBuffer(allocator, buffer, diagnostic_store)) orelse return error.NativeError;
    return @constCast(owned.value);
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
    if (options.filter) |filter| {
        const view = try temp.arena.allocator().create(c.mln_buffer_view);
        view.* = try temp.stringView(filter);
        raw.filter = view;
    }
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
    if (options.filter) |filter| {
        const view = try temp.arena.allocator().create(c.mln_buffer_view);
        view.* = try temp.stringView(filter);
        raw.filter = view;
    }
    return raw;
}

fn copyBufferView(allocator: std.mem.Allocator, view: c.mln_buffer_view) status.Error![]const u8 {
    if (view.size == 0) return allocator.dupe(u8, "");
    const data: [*]const u8 = @ptrCast(view.data orelse return error.NativeError);
    return allocator.dupe(u8, data[0..view.size]);
}

fn copyOptionalBufferView(
    allocator: std.mem.Allocator,
    present: bool,
    view: c.mln_buffer_view,
) status.Error!?[]const u8 {
    if (!present) return null;
    return try copyBufferView(allocator, view);
}

fn copyQueriedFeature(
    allocator: std.mem.Allocator,
    raw: c.mln_queried_feature,
) status.Error!QueriedFeature {
    const feature = try copyBufferView(allocator, raw.feature);
    errdefer allocator.free(feature);
    const source_id = try copyOptionalBufferView(allocator, (raw.fields & c.MLN_QUERIED_FEATURE_SOURCE_ID) != 0, raw.source_id);
    errdefer if (source_id) |value| allocator.free(value);
    const source_layer_id = try copyOptionalBufferView(allocator, (raw.fields & c.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0, raw.source_layer_id);
    errdefer if (source_layer_id) |value| allocator.free(value);
    const state = try copyOptionalBufferView(allocator, (raw.fields & c.MLN_QUERIED_FEATURE_STATE) != 0, raw.state);
    errdefer if (state) |value| allocator.free(value);
    return .{
        .allocator = allocator,
        .feature = feature,
        .source_id = source_id,
        .source_layer_id = source_layer_id,
        .state = state,
    };
}

fn copyQueriedFeatureList(
    allocator: std.mem.Allocator,
    list: c.mln_queried_feature_list,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!QueriedFeatureList {
    var count: usize = 0;
    try status.checkStatus(c.mln_queried_feature_list_count(list, &count), diagnostic_store);
    const items = try allocator.alloc(QueriedFeature, count);
    var initialized: usize = 0;
    errdefer {
        for (items[0..initialized]) |*item| item.deinit();
        allocator.free(items);
    }
    for (items, 0..) |*item, index| {
        var raw = c.mln_queried_feature_default();
        try status.checkStatus(c.mln_queried_feature_list_get(list, index, &raw), diagnostic_store);
        item.* = try copyQueriedFeature(allocator, raw);
        initialized += 1;
    }
    return .{ .allocator = allocator, .items = items };
}

fn stringViewArray(temp: *native_temp.TempStorage, values_list: []const []const u8) status.Error![]c.mln_buffer_view {
    const raw = try temp.arena.allocator().alloc(c.mln_buffer_view, values_list.len);
    for (values_list, raw) |value, *out| out.* = try temp.stringView(value);
    return raw;
}

fn metalSurfaceDescriptorToNative(descriptor: MetalSurfaceDescriptor) c.mln_metal_surface_descriptor {
    var raw = c.mln_metal_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = metalContextToNative(descriptor.context);
    raw.layer = descriptor.layer.toPtr();
    return raw;
}

fn vulkanSurfaceDescriptorToNative(descriptor: VulkanSurfaceDescriptor) c.mln_vulkan_surface_descriptor {
    var raw = c.mln_vulkan_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = vulkanContextToNative(descriptor.context);
    raw.surface = descriptor.surface.toPtr();
    return raw;
}

fn openglSurfaceDescriptorToNative(descriptor: OpenGLSurfaceDescriptor) c.mln_opengl_surface_descriptor {
    var raw = c.mln_opengl_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = openglContextToNative(descriptor.context);
    raw.surface = descriptor.surface.toPtr();
    return raw;
}
fn webgpuSurfaceDescriptorToNative(descriptor: WebGPUSurfaceDescriptor) c.mln_webgpu_surface_descriptor {
    var raw = c.mln_webgpu_surface_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.context = webgpuContextToNative(descriptor.context);
    raw.surface = descriptor.surface.toPtr();
    raw.format = descriptor.format;
    return raw;
}

fn metalBorrowedTextureDescriptorToNative(descriptor: MetalBorrowedTextureDescriptor) c.mln_metal_borrowed_texture_descriptor {
    var raw = c.mln_metal_borrowed_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.physical_width = descriptor.physical_width;
    raw.physical_height = descriptor.physical_height;
    raw.texture = descriptor.texture.toPtr();
    return raw;
}

fn vulkanBorrowedTextureDescriptorToNative(descriptor: VulkanBorrowedTextureDescriptor) c.mln_vulkan_borrowed_texture_descriptor {
    var raw = c.mln_vulkan_borrowed_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.physical_width = descriptor.physical_width;
    raw.physical_height = descriptor.physical_height;
    raw.context = vulkanContextToNative(descriptor.context);
    raw.image = descriptor.image.toPtr();
    raw.image_view = descriptor.image_view.toPtr();
    raw.format = descriptor.format;
    raw.initial_layout = descriptor.initial_layout;
    raw.final_layout = descriptor.final_layout;
    return raw;
}

fn openglBorrowedTextureDescriptorToNative(descriptor: OpenGLBorrowedTextureDescriptor) c.mln_opengl_borrowed_texture_descriptor {
    var raw = c.mln_opengl_borrowed_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.physical_width = descriptor.physical_width;
    raw.physical_height = descriptor.physical_height;
    raw.context = openglContextToNative(descriptor.context);
    raw.texture = descriptor.texture;
    raw.target = descriptor.target;
    return raw;
}
fn webgpuBorrowedTextureDescriptorToNative(descriptor: WebGPUBorrowedTextureDescriptor) c.mln_webgpu_borrowed_texture_descriptor {
    var raw = c.mln_webgpu_borrowed_texture_descriptor_default();
    raw.extent = renderTargetExtentToNative(descriptor.extent);
    raw.physical_width = descriptor.physical_width;
    raw.physical_height = descriptor.physical_height;
    raw.context = webgpuContextToNative(descriptor.context);
    raw.texture = descriptor.texture.toPtr();
    raw.texture_view = descriptor.texture_view.toPtr();
    raw.format = descriptor.format;
    return raw;
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
fn webgpuContextToNative(context: WebGPUContextDescriptor) c.mln_webgpu_context_descriptor {
    return .{
        .size = @sizeOf(c.mln_webgpu_context_descriptor),
        .instance = if (context.instance) |value| value.toPtr() else null,
        .device = context.device.toPtr(),
        .queue = if (context.queue) |value| value.toPtr() else null,
    };
}

fn openglContextToNative(context: OpenGLContextDescriptor) c.mln_opengl_context_descriptor {
    var raw = c.mln_opengl_context_descriptor{
        .size = @sizeOf(c.mln_opengl_context_descriptor),
        .platform = 0,
        .ownership = c.MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED,
        .data = undefined,
    };
    switch (context) {
        .wgl => |wgl| {
            raw.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WGL;
            raw.ownership = wgl.ownership.toRaw();
            raw.data.wgl = .{
                .size = @sizeOf(c.mln_wgl_context_descriptor),
                .device_context = wgl.device_context.toPtr(),
                .share_context = if (wgl.share_context) |pointer| pointer.toPtr() else null,
                .get_proc_address = if (wgl.get_proc_address) |pointer| pointer.toPtr() else null,
            };
        },
        .egl => |egl| {
            raw.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_EGL;
            raw.ownership = egl.ownership.toRaw();
            raw.data.egl = .{
                .size = @sizeOf(c.mln_egl_context_descriptor),
                .display = egl.display.toPtr(),
                .config = egl.config.toPtr(),
                .share_context = if (egl.share_context) |pointer| pointer.toPtr() else null,
                .client_api = egl.client_api.toRaw(),
                .get_proc_address = if (egl.get_proc_address) |pointer| pointer.toPtr() else null,
            };
        },
        .webgl => |webgl| {
            raw.platform = c.MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
            raw.data.webgl = .{
                .size = @sizeOf(c.mln_webgl_context_descriptor),
                .kind = switch (webgl) {
                    .existing => c.MLN_WEBGL_CONTEXT_EXISTING,
                    .transferred_canvas => c.MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS,
                },
                .context = switch (webgl) {
                    .existing => |value| value,
                    .transferred_canvas => 0,
                },
                .canvas_selector = switch (webgl) {
                    .existing => .{ .data = null, .size = 0 },
                    .transferred_canvas => |value| .{ .data = value.ptr, .size = value.len },
                },
            };
            switch (webgl) {
                .existing => {},
                .transferred_canvas => raw.ownership = c.MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED,
            }
        },
    }
    return raw;
}
