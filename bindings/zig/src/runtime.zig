const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const native_temp = @import("native_temp.zig");
const status = @import("status.zig");
const values = @import("values.zig");

const NativeRuntime = opaque {};

const MapRegistration = struct {
    native: *c.mln_map,
    id: values.MapId,
};

pub const RuntimeRegistry = struct {
    maps: std.ArrayList(MapRegistration),
    next_map_id: u64,
};

const ResourceProviderState = struct {
    provider: ResourceProvider,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

const ResourceRequestState = struct {
    native: ?*c.mln_resource_request_handle,
    completed: bool,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

const ResourceRequestRegistrySlot = struct {
    state: ?*ResourceRequestState,
    generation: u64,
};

var resource_request_registry_lock = std.atomic.Value(bool).init(false);
var resource_request_registry: std.ArrayList(ResourceRequestRegistrySlot) = .empty;
var resource_request_free_list: std.ArrayList(usize) = .empty;

pub const RuntimeOptions = struct {
    asset_path: ?[]const u8 = null,
    cache_path: ?[]const u8 = null,
    maximum_cache_size: ?u64 = null,
};

pub const NetworkStatus = union(enum) {
    online,
    offline,
    unknown: u32,

    fn fromRaw(raw: u32) NetworkStatus {
        return switch (raw) {
            c.MLN_NETWORK_STATUS_ONLINE => .online,
            c.MLN_NETWORK_STATUS_OFFLINE => .offline,
            else => .{ .unknown = raw },
        };
    }

    fn toRaw(self: NetworkStatus) u32 {
        return switch (self) {
            .online => c.MLN_NETWORK_STATUS_ONLINE,
            .offline => c.MLN_NETWORK_STATUS_OFFLINE,
            .unknown => |raw| raw,
        };
    }
};

pub const AmbientCacheOperation = enum {
    reset_database,
    pack_database,
    invalidate,
    clear,

    fn toRaw(self: AmbientCacheOperation) u32 {
        return switch (self) {
            .reset_database => c.MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE,
            .pack_database => c.MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE,
            .invalidate => c.MLN_AMBIENT_CACHE_OPERATION_INVALIDATE,
            .clear => c.MLN_AMBIENT_CACHE_OPERATION_CLEAR,
        };
    }
};

pub const OfflineRegionId = i64;

pub const OfflineTilePyramidRegionDefinition = struct {
    style_url: []const u8,
    bounds: values.LatLngBounds,
    min_zoom: f64,
    max_zoom: f64,
    pixel_ratio: f32 = 1.0,
    include_ideographs: bool = true,
};

pub const OfflineGeometryRegionDefinition = struct {
    style_url: []const u8,
    geometry: values.Geometry,
    min_zoom: f64,
    max_zoom: f64,
    pixel_ratio: f32 = 1.0,
    include_ideographs: bool = true,
};

pub const OfflineRegionDefinition = union(enum) {
    tile_pyramid: OfflineTilePyramidRegionDefinition,
    geometry: OfflineGeometryRegionDefinition,
};

pub const OwnedOfflineTilePyramidRegionDefinition = struct {
    style_url: []const u8,
    bounds: values.LatLngBounds,
    min_zoom: f64,
    max_zoom: f64,
    pixel_ratio: f32,
    include_ideographs: bool,
};

pub const OwnedOfflineGeometryRegionDefinition = struct {
    style_url: []const u8,
    geometry: values.OwnedGeometry,
    min_zoom: f64,
    max_zoom: f64,
    pixel_ratio: f32,
    include_ideographs: bool,
};

pub const OwnedOfflineRegionDefinition = union(enum) {
    tile_pyramid: OwnedOfflineTilePyramidRegionDefinition,
    geometry: OwnedOfflineGeometryRegionDefinition,

    pub fn deinit(self: *OwnedOfflineRegionDefinition, allocator: std.mem.Allocator) void {
        switch (self.*) {
            .tile_pyramid => |definition| allocator.free(definition.style_url),
            .geometry => |*definition| {
                allocator.free(definition.style_url);
                definition.geometry.deinit(allocator);
            },
        }
        self.* = .{ .tile_pyramid = .{
            .style_url = "",
            .bounds = .{ .southwest = .{ .latitude = 0, .longitude = 0 }, .northeast = .{ .latitude = 0, .longitude = 0 } },
            .min_zoom = 0,
            .max_zoom = 0,
            .pixel_ratio = 0,
            .include_ideographs = false,
        } };
    }
};

pub const OwnedOfflineRegion = struct {
    allocator: std.mem.Allocator,
    id: OfflineRegionId,
    definition: OwnedOfflineRegionDefinition,
    metadata: []const u8,

    pub fn deinit(self: *OwnedOfflineRegion) void {
        self.definition.deinit(self.allocator);
        self.allocator.free(self.metadata);
        self.metadata = "";
    }
};

pub const OfflineRegionList = struct {
    allocator: std.mem.Allocator,
    items: []OwnedOfflineRegion,

    pub fn deinit(self: *OfflineRegionList) void {
        for (self.items) |*item| item.deinit();
        self.allocator.free(self.items);
        self.items = &.{};
    }
};

pub const ResourceKind = union(enum) {
    unknown_kind,
    style,
    source,
    tile,
    glyphs,
    sprite_image,
    sprite_json,
    image,
    unknown: u32,

    fn fromRaw(raw: u32) ResourceKind {
        return switch (raw) {
            c.MLN_RESOURCE_KIND_UNKNOWN => .unknown_kind,
            c.MLN_RESOURCE_KIND_STYLE => .style,
            c.MLN_RESOURCE_KIND_SOURCE => .source,
            c.MLN_RESOURCE_KIND_TILE => .tile,
            c.MLN_RESOURCE_KIND_GLYPHS => .glyphs,
            c.MLN_RESOURCE_KIND_SPRITE_IMAGE => .sprite_image,
            c.MLN_RESOURCE_KIND_SPRITE_JSON => .sprite_json,
            c.MLN_RESOURCE_KIND_IMAGE => .image,
            else => .{ .unknown = raw },
        };
    }
};

pub const ResourceTransformRequest = struct {
    kind: ResourceKind,
    url: []const u8,
};

pub const ResourceTransformResponse = struct {
    /// Replacement URL borrowed by native code during the current callback invocation.
    ///
    /// Safety: when set, the pointed-to null-terminated storage must remain valid
    /// after the handler returns until native code copies it before completing the
    /// current transform invocation. String literals and context-owned storage are
    /// suitable; stack or temporary formatted buffers are not.
    replacement_url: ?[:0]const u8 = null,
};

pub const ResourceTransformHandler = *const fn (
    context: ?*anyopaque,
    request: ResourceTransformRequest,
) ResourceTransformResponse;

pub const ResourceTransform = struct {
    handler: ResourceTransformHandler,
    context: ?*anyopaque = null,
};

pub const ResourceLoadingMethod = union(enum) {
    all,
    cache_only,
    network_only,
    unknown: u32,

    fn fromRaw(raw: u32) ResourceLoadingMethod {
        return switch (raw) {
            c.MLN_RESOURCE_LOADING_METHOD_ALL => .all,
            c.MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY => .cache_only,
            c.MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY => .network_only,
            else => .{ .unknown = raw },
        };
    }
};

pub const ResourcePriority = union(enum) {
    regular,
    low,
    unknown: u32,

    fn fromRaw(raw: u32) ResourcePriority {
        return switch (raw) {
            c.MLN_RESOURCE_PRIORITY_REGULAR => .regular,
            c.MLN_RESOURCE_PRIORITY_LOW => .low,
            else => .{ .unknown = raw },
        };
    }
};

pub const ResourceUsage = union(enum) {
    online,
    offline,
    unknown: u32,

    fn fromRaw(raw: u32) ResourceUsage {
        return switch (raw) {
            c.MLN_RESOURCE_USAGE_ONLINE => .online,
            c.MLN_RESOURCE_USAGE_OFFLINE => .offline,
            else => .{ .unknown = raw },
        };
    }
};

pub const ResourceStoragePolicy = union(enum) {
    permanent,
    @"volatile",
    unknown: u32,

    fn fromRaw(raw: u32) ResourceStoragePolicy {
        return switch (raw) {
            c.MLN_RESOURCE_STORAGE_POLICY_PERMANENT => .permanent,
            c.MLN_RESOURCE_STORAGE_POLICY_VOLATILE => .@"volatile",
            else => .{ .unknown = raw },
        };
    }
};

pub const ResourceResponseStatus = enum {
    ok,
    @"error",
    no_content,
    not_modified,

    fn toRaw(self: ResourceResponseStatus) u32 {
        return switch (self) {
            .ok => c.MLN_RESOURCE_RESPONSE_STATUS_OK,
            .@"error" => c.MLN_RESOURCE_RESPONSE_STATUS_ERROR,
            .no_content => c.MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT,
            .not_modified => c.MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
        };
    }
};

pub const ResourceProviderDecision = enum {
    pass_through,
    handle,
};

pub const ResourceByteRange = struct {
    start: u64,
    end: u64,
};

pub const ResourceRequest = struct {
    url: []const u8,
    kind: ResourceKind,
    loading_method: ResourceLoadingMethod,
    priority: ResourcePriority,
    usage: ResourceUsage,
    storage_policy: ResourceStoragePolicy,
    range: ?ResourceByteRange,
    prior_modified_unix_ms: ?i64,
    prior_expires_unix_ms: ?i64,
    prior_etag: ?[]const u8,
    prior_data: []const u8,
};

pub const ResourceResponse = struct {
    status: ResourceResponseStatus = .ok,
    error_reason: ResourceErrorReason = .none,
    bytes: []const u8 = "",
    error_message: ?[:0]const u8 = null,
    must_revalidate: bool = false,
    modified_unix_ms: ?i64 = null,
    expires_unix_ms: ?i64 = null,
    etag: ?[:0]const u8 = null,
    retry_after_unix_ms: ?i64 = null,
};

pub const ResourceProviderHandler = *const fn (
    context: ?*anyopaque,
    request: ResourceRequest,
    handle: ?ResourceRequestHandle,
) ResourceProviderDecision;

pub const ResourceProvider = struct {
    handler: ResourceProviderHandler,
    context: ?*anyopaque = null,
};

pub const ResourceRequestHandle = enum(u128) {
    _,

    pub fn complete(self: ResourceRequestHandle, response: ResourceResponse) status.Error!void {
        lockResourceRequestRegistry();
        defer unlockResourceRequestRegistry();

        const request_state = resourceRequestState(self) orelse return error.ClosedHandle;
        if (request_state.completed) return error.AlreadyCompleted;
        const native_handle = request_state.native orelse return error.ClosedHandle;
        var native_response = resourceResponseToNative(response);
        request_state.completed = true;
        try status.checkStatus(c.mln_resource_request_complete(native_handle, &native_response), request_state.diagnostic_store);
    }

    pub fn cancelled(self: ResourceRequestHandle) status.Error!bool {
        lockResourceRequestRegistry();
        defer unlockResourceRequestRegistry();

        const request_state = resourceRequestState(self) orelse return error.ClosedHandle;
        const native_handle = request_state.native orelse return error.ClosedHandle;
        var is_cancelled = false;
        try status.checkStatus(c.mln_resource_request_cancelled(native_handle, &is_cancelled), request_state.diagnostic_store);
        return is_cancelled;
    }

    pub fn release(self: ResourceRequestHandle) void {
        lockResourceRequestRegistry();
        defer unlockResourceRequestRegistry();

        const request_state = unregisterResourceRequestState(self) orelse return;
        defer std.heap.smp_allocator.destroy(request_state);
        const native_handle = request_state.native orelse return;
        c.mln_resource_request_release(native_handle);
        request_state.native = null;
    }
};

pub const RuntimeEvent = struct {
    event_type: RuntimeEventType,
    source_type: RuntimeEventSourceType,
    source_id: ?values.MapId,
    payload_type: RuntimeEventPayloadType,
    code: i32,
};

pub const OwnedRuntimeEvent = struct {
    allocator: std.mem.Allocator,
    event_type: RuntimeEventType,
    source_type: RuntimeEventSourceType,
    source_id: ?values.MapId,
    payload_type: RuntimeEventPayloadType,
    code: i32,
    message: []const u8,
    payload: RuntimeEventPayload,

    pub fn deinit(self: *OwnedRuntimeEvent) void {
        self.payload.deinit(self.allocator);
        self.allocator.free(self.message);
        self.message = "";
        self.payload = .none;
    }
};

pub const RuntimeEventPayload = union(enum) {
    none,
    render_frame: RenderFramePayload,
    render_map: RenderMapPayload,
    style_image_missing: StyleImageMissingPayload,
    tile_action: TileActionPayload,
    offline_region_status: OfflineRegionStatusPayload,
    offline_region_response_error: OfflineRegionResponseErrorPayload,
    offline_region_tile_count_limit: OfflineRegionTileCountLimitPayload,
    unknown: UnknownPayload,

    pub fn deinit(self: *RuntimeEventPayload, allocator: std.mem.Allocator) void {
        switch (self.*) {
            .style_image_missing => |payload| allocator.free(payload.image_id),
            .tile_action => |payload| allocator.free(payload.source_id),
            .unknown => |payload| allocator.free(payload.bytes),
            else => {},
        }
        self.* = .none;
    }
};

pub const RenderMode = union(enum) {
    partial,
    full,
    unknown: u32,

    fn fromRaw(raw: u32) RenderMode {
        return switch (raw) {
            c.MLN_RENDER_MODE_PARTIAL => .partial,
            c.MLN_RENDER_MODE_FULL => .full,
            else => .{ .unknown = raw },
        };
    }
};

pub const RenderingStats = struct {
    encoding_time: f64,
    rendering_time: f64,
    frame_count: i64,
    draw_call_count: i64,
    total_draw_call_count: i64,
};

pub const RenderFramePayload = struct {
    mode: RenderMode,
    needs_repaint: bool,
    placement_changed: bool,
    stats: RenderingStats,
};

pub const RenderMapPayload = struct {
    mode: RenderMode,
};

pub const StyleImageMissingPayload = struct {
    image_id: []const u8,
};

pub const TileOperation = union(enum) {
    requested_from_cache,
    requested_from_network,
    load_from_network,
    load_from_cache,
    start_parse,
    end_parse,
    @"error",
    cancelled,
    null,
    unknown: u32,

    fn fromRaw(raw: u32) TileOperation {
        return switch (raw) {
            c.MLN_TILE_OPERATION_REQUESTED_FROM_CACHE => .requested_from_cache,
            c.MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK => .requested_from_network,
            c.MLN_TILE_OPERATION_LOAD_FROM_NETWORK => .load_from_network,
            c.MLN_TILE_OPERATION_LOAD_FROM_CACHE => .load_from_cache,
            c.MLN_TILE_OPERATION_START_PARSE => .start_parse,
            c.MLN_TILE_OPERATION_END_PARSE => .end_parse,
            c.MLN_TILE_OPERATION_ERROR => .@"error",
            c.MLN_TILE_OPERATION_CANCELLED => .cancelled,
            c.MLN_TILE_OPERATION_NULL => .null,
            else => .{ .unknown = raw },
        };
    }
};

pub const TileId = struct {
    overscaled_z: u32,
    wrap: i32,
    canonical_z: u32,
    canonical_x: u32,
    canonical_y: u32,
};

pub const TileActionPayload = struct {
    operation: TileOperation,
    tile_id: TileId,
    source_id: []const u8,
};

pub const OfflineRegionDownloadState = union(enum) {
    inactive,
    active,
    unknown: u32,

    fn fromRaw(raw: u32) OfflineRegionDownloadState {
        return switch (raw) {
            c.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE => .inactive,
            c.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE => .active,
            else => .{ .unknown = raw },
        };
    }

    fn toRaw(self: OfflineRegionDownloadState) u32 {
        return switch (self) {
            .inactive => c.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE,
            .active => c.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
            .unknown => |raw| raw,
        };
    }
};

pub const OfflineRegionStatus = struct {
    download_state: OfflineRegionDownloadState,
    completed_resource_count: u64,
    completed_resource_size: u64,
    completed_tile_count: u64,
    required_tile_count: u64,
    completed_tile_size: u64,
    required_resource_count: u64,
    required_resource_count_is_precise: bool,
    complete: bool,
};

pub const OfflineRegionStatusPayload = struct {
    region_id: i64,
    status: OfflineRegionStatus,
};

pub const ResourceErrorReason = union(enum) {
    none,
    not_found,
    server,
    connection,
    rate_limit,
    other,
    unknown: u32,

    fn fromRaw(raw: u32) ResourceErrorReason {
        return switch (raw) {
            c.MLN_RESOURCE_ERROR_REASON_NONE => .none,
            c.MLN_RESOURCE_ERROR_REASON_NOT_FOUND => .not_found,
            c.MLN_RESOURCE_ERROR_REASON_SERVER => .server,
            c.MLN_RESOURCE_ERROR_REASON_CONNECTION => .connection,
            c.MLN_RESOURCE_ERROR_REASON_RATE_LIMIT => .rate_limit,
            c.MLN_RESOURCE_ERROR_REASON_OTHER => .other,
            else => .{ .unknown = raw },
        };
    }

    fn toRaw(self: ResourceErrorReason) u32 {
        return switch (self) {
            .none => c.MLN_RESOURCE_ERROR_REASON_NONE,
            .not_found => c.MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
            .server => c.MLN_RESOURCE_ERROR_REASON_SERVER,
            .connection => c.MLN_RESOURCE_ERROR_REASON_CONNECTION,
            .rate_limit => c.MLN_RESOURCE_ERROR_REASON_RATE_LIMIT,
            .other => c.MLN_RESOURCE_ERROR_REASON_OTHER,
            .unknown => |raw| raw,
        };
    }
};

pub const OfflineRegionResponseErrorPayload = struct {
    region_id: i64,
    reason: ResourceErrorReason,
};

pub const OfflineRegionTileCountLimitPayload = struct {
    region_id: i64,
    limit: u64,
};

pub const UnknownPayload = struct {
    payload_type: u32,
    bytes: []const u8,
};

pub const RuntimeEventType = union(enum) {
    map_camera_will_change,
    map_camera_is_changing,
    map_camera_did_change,
    map_style_loaded,
    map_loading_started,
    map_loading_finished,
    map_loading_failed,
    map_idle,
    map_render_update_available,
    map_render_error,
    map_still_image_finished,
    map_still_image_failed,
    map_render_frame_started,
    map_render_frame_finished,
    map_render_map_started,
    map_render_map_finished,
    map_style_image_missing,
    map_tile_action,
    offline_region_status_changed,
    offline_region_response_error,
    offline_region_tile_count_limit_exceeded,
    unknown: u32,

    fn fromRaw(raw: u32) RuntimeEventType {
        return switch (raw) {
            c.MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE => .map_camera_will_change,
            c.MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING => .map_camera_is_changing,
            c.MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE => .map_camera_did_change,
            c.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED => .map_style_loaded,
            c.MLN_RUNTIME_EVENT_MAP_LOADING_STARTED => .map_loading_started,
            c.MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED => .map_loading_finished,
            c.MLN_RUNTIME_EVENT_MAP_LOADING_FAILED => .map_loading_failed,
            c.MLN_RUNTIME_EVENT_MAP_IDLE => .map_idle,
            c.MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE => .map_render_update_available,
            c.MLN_RUNTIME_EVENT_MAP_RENDER_ERROR => .map_render_error,
            c.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED => .map_still_image_finished,
            c.MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED => .map_still_image_failed,
            c.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED => .map_render_frame_started,
            c.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED => .map_render_frame_finished,
            c.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED => .map_render_map_started,
            c.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED => .map_render_map_finished,
            c.MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING => .map_style_image_missing,
            c.MLN_RUNTIME_EVENT_MAP_TILE_ACTION => .map_tile_action,
            c.MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED => .offline_region_status_changed,
            c.MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR => .offline_region_response_error,
            c.MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED => .offline_region_tile_count_limit_exceeded,
            else => .{ .unknown = raw },
        };
    }
};

pub const RuntimeEventSourceType = union(enum) {
    runtime,
    map,
    unknown: u32,

    fn fromRaw(raw: u32) RuntimeEventSourceType {
        return switch (raw) {
            c.MLN_RUNTIME_EVENT_SOURCE_RUNTIME => .runtime,
            c.MLN_RUNTIME_EVENT_SOURCE_MAP => .map,
            else => .{ .unknown = raw },
        };
    }
};

pub const RuntimeEventPayloadType = union(enum) {
    none,
    render_frame,
    render_map,
    style_image_missing,
    tile_action,
    offline_region_status,
    offline_region_response_error,
    offline_region_tile_count_limit,
    unknown: u32,

    fn fromRaw(raw: u32) RuntimeEventPayloadType {
        return switch (raw) {
            c.MLN_RUNTIME_EVENT_PAYLOAD_NONE => .none,
            c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => .render_frame,
            c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => .render_map,
            c.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING => .style_image_missing,
            c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => .tile_action,
            c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => .offline_region_status,
            c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => .offline_region_response_error,
            c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => .offline_region_tile_count_limit,
            else => .{ .unknown = raw },
        };
    }
};

pub const RuntimeHandle = struct {
    native: ?*NativeRuntime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    registry: ?*RuntimeRegistry,
    resource_transform: ?*ResourceTransform,
    resource_provider: ?*ResourceProviderState,

    pub fn init(diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!RuntimeHandle {
        var native_options = c.mln_runtime_options_default();
        return createNative(&native_options, diagnostic_store);
    }

    pub fn create(
        allocator: std.mem.Allocator,
        options: RuntimeOptions,
        diagnostic_store: ?*diagnostics.DiagnosticStore,
    ) status.Error!RuntimeHandle {
        var native_options = c.mln_runtime_options_default();
        var asset_path: ?[:0]u8 = null;
        defer if (asset_path) |value| allocator.free(value);
        var cache_path: ?[:0]u8 = null;
        defer if (cache_path) |value| allocator.free(value);

        if (options.asset_path) |value| {
            asset_path = try nulTerminated(allocator, value);
            native_options.asset_path = asset_path.?.ptr;
        }
        if (options.cache_path) |value| {
            cache_path = try nulTerminated(allocator, value);
            native_options.cache_path = cache_path.?.ptr;
        }
        if (options.maximum_cache_size) |value| {
            native_options.flags |= c.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE;
            native_options.maximum_cache_size = value;
        }

        return createNative(&native_options, diagnostic_store);
    }

    pub fn runOnce(self: *RuntimeHandle) status.Error!void {
        try status.checkStatus(c.mln_runtime_run_once(try native(self)), self.diagnostic_store);
    }

    pub fn pollEvent(self: *RuntimeHandle) status.Error!?RuntimeEvent {
        var native_event = emptyNativeEvent();
        var has_event = false;
        try status.checkStatus(
            c.mln_runtime_poll_event(try native(self), &native_event, &has_event),
            self.diagnostic_store,
        );
        if (!has_event) return null;
        return runtimeEventFromNative(self, native_event);
    }

    pub fn pollEventOwned(self: *RuntimeHandle, allocator: std.mem.Allocator) status.Error!?OwnedRuntimeEvent {
        var native_event = emptyNativeEvent();
        var has_event = false;
        try status.checkStatus(
            c.mln_runtime_poll_event(try native(self), &native_event, &has_event),
            self.diagnostic_store,
        );
        if (!has_event) return null;

        const event = runtimeEventFromNative(self, native_event);
        const message = try copyOptionalBytes(allocator, native_event.message, native_event.message_size);
        errdefer allocator.free(message);
        return .{
            .allocator = allocator,
            .event_type = event.event_type,
            .source_type = event.source_type,
            .source_id = event.source_id,
            .payload_type = event.payload_type,
            .code = event.code,
            .message = message,
            .payload = try copyPayload(allocator, native_event),
        };
    }

    pub fn runAmbientCacheOperation(self: *RuntimeHandle, operation: AmbientCacheOperation) status.Error!void {
        try status.checkStatus(
            c.mln_runtime_run_ambient_cache_operation(try native(self), operation.toRaw()),
            self.diagnostic_store,
        );
    }

    pub fn createOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        definition: OfflineRegionDefinition,
        metadata: []const u8,
    ) status.Error!OwnedOfflineRegion {
        var temp = native_temp.TempStorage.init(allocator);
        defer temp.deinit();
        const native_definition = try temp.offlineRegionDefinition(definition);
        var snapshot: ?*c.mln_offline_region_snapshot = null;
        try status.checkStatus(
            c.mln_runtime_offline_region_create(
                try native(self),
                native_definition,
                if (metadata.len == 0) null else metadata.ptr,
                metadata.len,
                &snapshot,
            ),
            self.diagnostic_store,
        );
        const snapshot_handle = snapshot orelse return error.NativeError;
        defer c.mln_offline_region_snapshot_destroy(snapshot_handle);
        return copyOfflineRegionSnapshot(allocator, snapshot_handle);
    }

    pub fn getOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        region_id: OfflineRegionId,
    ) status.Error!?OwnedOfflineRegion {
        var snapshot: ?*c.mln_offline_region_snapshot = null;
        var found = false;
        try status.checkStatus(
            c.mln_runtime_offline_region_get(try native(self), region_id, &snapshot, &found),
            self.diagnostic_store,
        );
        if (!found) return null;
        const snapshot_handle = snapshot orelse return error.NativeError;
        defer c.mln_offline_region_snapshot_destroy(snapshot_handle);
        return try copyOfflineRegionSnapshot(allocator, snapshot_handle);
    }

    pub fn listOfflineRegions(self: *RuntimeHandle, allocator: std.mem.Allocator) status.Error!OfflineRegionList {
        var list: ?*c.mln_offline_region_list = null;
        try status.checkStatus(
            c.mln_runtime_offline_regions_list(try native(self), &list),
            self.diagnostic_store,
        );
        const list_handle = list orelse return error.NativeError;
        defer c.mln_offline_region_list_destroy(list_handle);
        return copyOfflineRegionList(allocator, list_handle);
    }

    pub fn mergeOfflineRegionsDatabase(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        side_database_path: []const u8,
    ) status.Error!OfflineRegionList {
        const path = try nulTerminated(allocator, side_database_path);
        defer allocator.free(path);
        var list: ?*c.mln_offline_region_list = null;
        try status.checkStatus(
            c.mln_runtime_offline_regions_merge_database(try native(self), path.ptr, &list),
            self.diagnostic_store,
        );
        const list_handle = list orelse return error.NativeError;
        defer c.mln_offline_region_list_destroy(list_handle);
        return copyOfflineRegionList(allocator, list_handle);
    }

    pub fn updateOfflineRegionMetadata(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        region_id: OfflineRegionId,
        metadata: []const u8,
    ) status.Error!OwnedOfflineRegion {
        var snapshot: ?*c.mln_offline_region_snapshot = null;
        try status.checkStatus(
            c.mln_runtime_offline_region_update_metadata(
                try native(self),
                region_id,
                if (metadata.len == 0) null else metadata.ptr,
                metadata.len,
                &snapshot,
            ),
            self.diagnostic_store,
        );
        const snapshot_handle = snapshot orelse return error.NativeError;
        defer c.mln_offline_region_snapshot_destroy(snapshot_handle);
        return copyOfflineRegionSnapshot(allocator, snapshot_handle);
    }

    pub fn getOfflineRegionStatus(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OfflineRegionStatus {
        var native_status: c.mln_offline_region_status = undefined;
        native_status.size = @sizeOf(c.mln_offline_region_status);
        try status.checkStatus(
            c.mln_runtime_offline_region_get_status(try native(self), region_id, &native_status),
            self.diagnostic_store,
        );
        return offlineStatusFromNative(native_status);
    }

    pub fn setOfflineRegionObserved(self: *RuntimeHandle, region_id: OfflineRegionId, observed: bool) status.Error!void {
        try status.checkStatus(
            c.mln_runtime_offline_region_set_observed(try native(self), region_id, observed),
            self.diagnostic_store,
        );
    }

    pub fn setOfflineRegionDownloadState(
        self: *RuntimeHandle,
        region_id: OfflineRegionId,
        download_state: OfflineRegionDownloadState,
    ) status.Error!void {
        try status.checkStatus(
            c.mln_runtime_offline_region_set_download_state(try native(self), region_id, download_state.toRaw()),
            self.diagnostic_store,
        );
    }

    pub fn invalidateOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!void {
        try status.checkStatus(
            c.mln_runtime_offline_region_invalidate(try native(self), region_id),
            self.diagnostic_store,
        );
    }

    pub fn deleteOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!void {
        try status.checkStatus(
            c.mln_runtime_offline_region_delete(try native(self), region_id),
            self.diagnostic_store,
        );
    }

    pub fn setResourceTransform(self: *RuntimeHandle, transform: ?ResourceTransform) status.Error!void {
        const runtime_state = self;
        if (transform) |value| {
            const replacement = try std.heap.smp_allocator.create(ResourceTransform);
            errdefer std.heap.smp_allocator.destroy(replacement);
            replacement.* = value;
            var native_transform = c.mln_resource_transform{
                .size = @sizeOf(c.mln_resource_transform),
                .callback = resourceTransformTrampoline,
                .user_data = replacement,
            };
            try status.checkStatus(
                c.mln_runtime_set_resource_transform(try native(self), &native_transform),
                self.diagnostic_store,
            );
            const previous = runtime_state.resource_transform;
            runtime_state.resource_transform = replacement;
            if (previous) |old| std.heap.smp_allocator.destroy(old);
            return;
        }

        try status.checkStatus(
            c.mln_runtime_clear_resource_transform(try native(self)),
            self.diagnostic_store,
        );
        if (runtime_state.resource_transform) |old| {
            runtime_state.resource_transform = null;
            std.heap.smp_allocator.destroy(old);
        }
    }

    pub fn setResourceProvider(self: *RuntimeHandle, provider: ResourceProvider) status.Error!void {
        const replacement = try std.heap.smp_allocator.create(ResourceProviderState);
        errdefer std.heap.smp_allocator.destroy(replacement);
        replacement.* = .{ .provider = provider, .diagnostic_store = self.diagnostic_store };
        var native_provider = c.mln_resource_provider{
            .size = @sizeOf(c.mln_resource_provider),
            .callback = resourceProviderTrampoline,
            .user_data = replacement,
        };
        try status.checkStatus(
            c.mln_runtime_set_resource_provider(try native(self), &native_provider),
            self.diagnostic_store,
        );
        const previous = self.resource_provider;
        self.resource_provider = replacement;
        if (previous) |old_provider| std.heap.smp_allocator.destroy(old_provider);
    }

    pub fn close(self: *RuntimeHandle) status.Error!void {
        const runtime: *c.mln_runtime = @ptrCast(self.native orelse return);
        try status.checkStatus(c.mln_runtime_destroy(runtime), self.diagnostic_store);
        if (self.resource_transform) |old| {
            self.resource_transform = null;
            std.heap.smp_allocator.destroy(old);
        }
        if (self.resource_provider) |old| {
            self.resource_provider = null;
            std.heap.smp_allocator.destroy(old);
        }
        if (self.registry) |runtime_registry| {
            self.registry = null;
            runtime_registry.maps.deinit(std.heap.smp_allocator);
            std.heap.smp_allocator.destroy(runtime_registry);
        }
        self.native = null;
    }
};

pub fn getNetworkStatus(diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!NetworkStatus {
    var raw: u32 = 0;
    try status.checkStatus(c.mln_network_status_get(&raw), diagnostic_store);
    return NetworkStatus.fromRaw(raw);
}

pub fn setNetworkStatus(network_status: NetworkStatus, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!void {
    try status.checkStatus(c.mln_network_status_set(network_status.toRaw()), diagnostic_store);
}

fn createNative(
    native_options: *c.mln_runtime_options,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!RuntimeHandle {
    var runtime: ?*c.mln_runtime = null;
    try status.checkStatus(c.mln_runtime_create(native_options, &runtime), diagnostic_store);
    errdefer {
        if (runtime) |handle| _ = c.mln_runtime_destroy(handle);
    }

    const runtime_registry = try std.heap.smp_allocator.create(RuntimeRegistry);
    runtime_registry.* = .{ .maps = .empty, .next_map_id = 1 };
    errdefer std.heap.smp_allocator.destroy(runtime_registry);

    return .{
        .native = @ptrCast(runtime.?),
        .diagnostic_store = diagnostic_store,
        .registry = runtime_registry,
        .resource_transform = null,
        .resource_provider = null,
    };
}

fn runtimeEventFromNative(handle: *RuntimeHandle, native_event: c.mln_runtime_event) RuntimeEvent {
    return .{
        .event_type = RuntimeEventType.fromRaw(native_event.type),
        .source_type = RuntimeEventSourceType.fromRaw(native_event.source_type),
        .source_id = mapIdForNativeSource(handle, native_event.source_type, native_event.source),
        .payload_type = RuntimeEventPayloadType.fromRaw(native_event.payload_type),
        .code = native_event.code,
    };
}

fn mapIdForNativeSource(handle: *RuntimeHandle, source_type: u32, source: ?*anyopaque) ?values.MapId {
    if (source_type != c.MLN_RUNTIME_EVENT_SOURCE_MAP) return null;
    const source_ptr = source orelse return null;
    const runtime_registry = handle.registry orelse return null;
    for (runtime_registry.maps.items) |registration| {
        if (@intFromPtr(registration.native) == @intFromPtr(source_ptr)) return registration.id;
    }
    return null;
}

fn resourceTransformTrampoline(
    user_data: ?*anyopaque,
    kind: u32,
    url: [*c]const u8,
    out_response: [*c]c.mln_resource_transform_response,
) callconv(.c) c.mln_status {
    const transform: *ResourceTransform = @ptrCast(@alignCast(user_data orelse return c.MLN_STATUS_INVALID_ARGUMENT));
    const copied_url = std.heap.smp_allocator.dupe(u8, if (url == null) "" else std.mem.span(url)) catch return c.MLN_STATUS_NATIVE_ERROR;
    defer std.heap.smp_allocator.free(copied_url);
    const response = transform.handler(transform.context, .{
        .kind = ResourceKind.fromRaw(kind),
        .url = copied_url,
    });
    if (out_response) |native_response| {
        native_response.* = .{
            .size = @sizeOf(c.mln_resource_transform_response),
            .url = if (response.replacement_url) |replacement_url| replacement_url.ptr else null,
        };
    }
    return c.MLN_STATUS_OK;
}

fn resourceProviderTrampoline(
    user_data: ?*anyopaque,
    request: ?*const c.mln_resource_request,
    native_handle: ?*c.mln_resource_request_handle,
) callconv(.c) u32 {
    const provider_state: *ResourceProviderState = @ptrCast(@alignCast(user_data orelse return c.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH));
    const provider = provider_state.provider;
    const raw_request = request orelse return c.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
    const request_view = resourceRequestFromNative(std.heap.smp_allocator, raw_request) catch return c.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
    defer resourceRequestDeinit(std.heap.smp_allocator, request_view);
    const handle = createResourceRequestHandle(native_handle, provider_state.diagnostic_store) catch null;
    const decision = provider.handler(provider.context, request_view, handle);
    return switch (decision) {
        .pass_through => blk: {
            if (handle) |value| destroyUnreleasedResourceRequestHandle(value);
            break :blk c.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH;
        },
        .handle => if (handle == null)
            c.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        else
            c.MLN_RESOURCE_PROVIDER_DECISION_HANDLE,
    };
}

fn createResourceRequestHandle(
    native_handle: ?*c.mln_resource_request_handle,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) std.mem.Allocator.Error!?ResourceRequestHandle {
    const request_handle = native_handle orelse return null;
    const request_state = try std.heap.smp_allocator.create(ResourceRequestState);
    request_state.* = .{ .native = request_handle, .completed = false, .diagnostic_store = diagnostic_store };
    errdefer std.heap.smp_allocator.destroy(request_state);
    return try registerResourceRequestState(request_state);
}

fn destroyUnreleasedResourceRequestHandle(handle: ResourceRequestHandle) void {
    lockResourceRequestRegistry();
    defer unlockResourceRequestRegistry();

    const request_state = unregisterResourceRequestState(handle) orelse return;
    request_state.native = null;
    std.heap.smp_allocator.destroy(request_state);
}

fn registerResourceRequestState(request_state: *ResourceRequestState) std.mem.Allocator.Error!ResourceRequestHandle {
    lockResourceRequestRegistry();
    defer unlockResourceRequestRegistry();

    if (resource_request_free_list.items.len > 0) {
        const slot_index = resource_request_free_list.pop().?;
        resource_request_registry.items[slot_index].state = request_state;
        return resourceRequestHandle(slot_index + 1, resource_request_registry.items[slot_index].generation);
    }

    try resource_request_registry.append(std.heap.smp_allocator, .{ .state = request_state, .generation = 1 });
    return resourceRequestHandle(resource_request_registry.items.len, 1);
}

fn resourceRequestHandle(index: usize, generation: u64) ResourceRequestHandle {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn resourceRequestIndex(handle: ResourceRequestHandle) ?usize {
    const index = @intFromEnum(handle) & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn resourceRequestGeneration(handle: ResourceRequestHandle) u64 {
    return @intCast(@intFromEnum(handle) >> 64);
}

fn resourceRequestState(handle: ResourceRequestHandle) ?*ResourceRequestState {
    const index = resourceRequestIndex(handle) orelse return null;
    if (index > resource_request_registry.items.len) return null;
    const slot = resource_request_registry.items[index - 1];
    if (slot.generation != resourceRequestGeneration(handle)) return null;
    return slot.state;
}

fn unregisterResourceRequestState(handle: ResourceRequestHandle) ?*ResourceRequestState {
    const index = resourceRequestIndex(handle) orelse return null;
    if (index > resource_request_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &resource_request_registry.items[slot_index];
    if (slot.generation != resourceRequestGeneration(handle)) return null;
    const request_state = slot.state orelse return null;
    slot.state = null;
    slot.generation +%= 1;
    resource_request_free_list.append(std.heap.smp_allocator, slot_index) catch {};
    return request_state;
}

fn lockResourceRequestRegistry() void {
    while (resource_request_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockResourceRequestRegistry() void {
    resource_request_registry_lock.store(false, .seq_cst);
}

fn resourceRequestFromNative(allocator: std.mem.Allocator, request: *const c.mln_resource_request) std.mem.Allocator.Error!ResourceRequest {
    const url = try allocator.dupe(u8, if (request.url == null) "" else std.mem.span(request.url));
    errdefer allocator.free(url);
    const prior_etag = if (request.prior_etag == null) null else try allocator.dupe(u8, std.mem.span(request.prior_etag));
    errdefer if (prior_etag) |value| allocator.free(value);
    const prior_data = try allocator.dupe(u8, if (request.prior_data_size == 0) "" else request.prior_data[0..request.prior_data_size]);
    errdefer allocator.free(prior_data);

    return .{
        .url = url,
        .kind = ResourceKind.fromRaw(request.kind),
        .loading_method = ResourceLoadingMethod.fromRaw(request.loading_method),
        .priority = ResourcePriority.fromRaw(request.priority),
        .usage = ResourceUsage.fromRaw(request.usage),
        .storage_policy = ResourceStoragePolicy.fromRaw(request.storage_policy),
        .range = if (request.has_range) .{ .start = request.range_start, .end = request.range_end } else null,
        .prior_modified_unix_ms = if (request.has_prior_modified) request.prior_modified_unix_ms else null,
        .prior_expires_unix_ms = if (request.has_prior_expires) request.prior_expires_unix_ms else null,
        .prior_etag = prior_etag,
        .prior_data = prior_data,
    };
}

fn resourceRequestDeinit(allocator: std.mem.Allocator, request: ResourceRequest) void {
    allocator.free(request.url);
    if (request.prior_etag) |prior_etag| allocator.free(prior_etag);
    allocator.free(request.prior_data);
}

fn resourceResponseToNative(response: ResourceResponse) c.mln_resource_response {
    return .{
        .size = @sizeOf(c.mln_resource_response),
        .status = response.status.toRaw(),
        .error_reason = response.error_reason.toRaw(),
        .bytes = if (response.bytes.len == 0) null else response.bytes.ptr,
        .byte_count = response.bytes.len,
        .error_message = if (response.error_message) |message| message.ptr else null,
        .must_revalidate = response.must_revalidate,
        .has_modified = response.modified_unix_ms != null,
        .modified_unix_ms = response.modified_unix_ms orelse 0,
        .has_expires = response.expires_unix_ms != null,
        .expires_unix_ms = response.expires_unix_ms orelse 0,
        .etag = if (response.etag) |etag| etag.ptr else null,
        .has_retry_after = response.retry_after_unix_ms != null,
        .retry_after_unix_ms = response.retry_after_unix_ms orelse 0,
    };
}

fn copyPayload(allocator: std.mem.Allocator, native_event: c.mln_runtime_event) status.Error!RuntimeEventPayload {
    return switch (native_event.payload_type) {
        c.MLN_RUNTIME_EVENT_PAYLOAD_NONE => .none,
        c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => blk: {
            const payload = try payloadAs(c.mln_runtime_event_render_frame, native_event.payload, native_event.payload_size);
            break :blk .{ .render_frame = .{
                .mode = RenderMode.fromRaw(payload.mode),
                .needs_repaint = payload.needs_repaint,
                .placement_changed = payload.placement_changed,
                .stats = renderingStatsFromNative(payload.stats),
            } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => blk: {
            const payload = try payloadAs(c.mln_runtime_event_render_map, native_event.payload, native_event.payload_size);
            break :blk .{ .render_map = .{ .mode = RenderMode.fromRaw(payload.mode) } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING => blk: {
            const payload = try payloadAs(c.mln_runtime_event_style_image_missing, native_event.payload, native_event.payload_size);
            break :blk .{ .style_image_missing = .{ .image_id = try copyOptionalBytes(allocator, payload.image_id, payload.image_id_size) } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => blk: {
            const payload = try payloadAs(c.mln_runtime_event_tile_action, native_event.payload, native_event.payload_size);
            break :blk .{ .tile_action = .{
                .operation = TileOperation.fromRaw(payload.operation),
                .tile_id = tileIdFromNative(payload.tile_id),
                .source_id = try copyOptionalBytes(allocator, payload.source_id, payload.source_id_size),
            } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => blk: {
            const payload = try payloadAs(c.mln_runtime_event_offline_region_status, native_event.payload, native_event.payload_size);
            break :blk .{ .offline_region_status = .{ .region_id = payload.region_id, .status = offlineStatusFromNative(payload.status) } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => blk: {
            const payload = try payloadAs(c.mln_runtime_event_offline_region_response_error, native_event.payload, native_event.payload_size);
            break :blk .{ .offline_region_response_error = .{ .region_id = payload.region_id, .reason = ResourceErrorReason.fromRaw(payload.reason) } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => blk: {
            const payload = try payloadAs(c.mln_runtime_event_offline_region_tile_count_limit, native_event.payload, native_event.payload_size);
            break :blk .{ .offline_region_tile_count_limit = .{ .region_id = payload.region_id, .limit = payload.limit } };
        },
        else => .{ .unknown = .{
            .payload_type = native_event.payload_type,
            .bytes = try copyOptionalOpaqueBytes(allocator, native_event.payload, native_event.payload_size),
        } },
    };
}

fn payloadAs(comptime T: type, payload: ?*const anyopaque, size: usize) status.Error!*const T {
    if (size < @sizeOf(T)) return error.NativeError;
    const raw = payload orelse return error.NativeError;
    if (@intFromPtr(raw) % @alignOf(T) != 0) return error.NativeError;
    const typed: *const T = @ptrCast(@alignCast(raw));
    if (typed.size < @sizeOf(T)) return error.NativeError;
    return typed;
}

fn copyOptionalBytes(allocator: std.mem.Allocator, data: ?[*]const u8, size: usize) status.Error![]const u8 {
    if (size == 0) return allocator.dupe(u8, "");
    const bytes = data orelse return error.NativeError;
    return allocator.dupe(u8, bytes[0..size]);
}

fn copyOptionalOpaqueBytes(allocator: std.mem.Allocator, data: ?*const anyopaque, size: usize) status.Error![]const u8 {
    if (size == 0) return allocator.dupe(u8, "");
    const bytes: [*]const u8 = @ptrCast(data orelse return error.NativeError);
    return allocator.dupe(u8, bytes[0..size]);
}

fn renderingStatsFromNative(raw: c.mln_rendering_stats) RenderingStats {
    return .{
        .encoding_time = raw.encoding_time,
        .rendering_time = raw.rendering_time,
        .frame_count = raw.frame_count,
        .draw_call_count = raw.draw_call_count,
        .total_draw_call_count = raw.total_draw_call_count,
    };
}

fn tileIdFromNative(raw: c.mln_tile_id) TileId {
    return .{
        .overscaled_z = raw.overscaled_z,
        .wrap = raw.wrap,
        .canonical_z = raw.canonical_z,
        .canonical_x = raw.canonical_x,
        .canonical_y = raw.canonical_y,
    };
}

fn offlineStatusFromNative(raw: c.mln_offline_region_status) OfflineRegionStatus {
    return .{
        .download_state = OfflineRegionDownloadState.fromRaw(raw.download_state),
        .completed_resource_count = raw.completed_resource_count,
        .completed_resource_size = raw.completed_resource_size,
        .completed_tile_count = raw.completed_tile_count,
        .required_tile_count = raw.required_tile_count,
        .completed_tile_size = raw.completed_tile_size,
        .required_resource_count = raw.required_resource_count,
        .required_resource_count_is_precise = raw.required_resource_count_is_precise,
        .complete = raw.complete,
    };
}

fn copyOfflineRegionSnapshot(
    allocator: std.mem.Allocator,
    snapshot: *const c.mln_offline_region_snapshot,
) status.Error!OwnedOfflineRegion {
    var info: c.mln_offline_region_info = undefined;
    info.size = @sizeOf(c.mln_offline_region_info);
    try status.checkStatus(c.mln_offline_region_snapshot_get(snapshot, &info), null);
    return copyOfflineRegionInfo(allocator, info);
}

fn copyOfflineRegionList(allocator: std.mem.Allocator, list: *const c.mln_offline_region_list) status.Error!OfflineRegionList {
    var count: usize = 0;
    try status.checkStatus(c.mln_offline_region_list_count(list, &count), null);
    const items = try allocator.alloc(OwnedOfflineRegion, count);
    var initialized: usize = 0;
    errdefer {
        for (items[0..initialized]) |*item| item.deinit();
        allocator.free(items);
    }
    for (items, 0..) |*item, index| {
        var info: c.mln_offline_region_info = undefined;
        info.size = @sizeOf(c.mln_offline_region_info);
        try status.checkStatus(c.mln_offline_region_list_get(list, index, &info), null);
        item.* = try copyOfflineRegionInfo(allocator, info);
        initialized += 1;
    }
    return .{ .allocator = allocator, .items = items };
}

fn copyOfflineRegionInfo(allocator: std.mem.Allocator, info: c.mln_offline_region_info) status.Error!OwnedOfflineRegion {
    var definition = try copyOfflineRegionDefinition(allocator, info.definition);
    errdefer definition.deinit(allocator);
    const metadata = try copyOptionalBytes(allocator, info.metadata, info.metadata_size);
    return .{
        .allocator = allocator,
        .id = info.id,
        .definition = definition,
        .metadata = metadata,
    };
}

fn copyOfflineRegionDefinition(
    allocator: std.mem.Allocator,
    raw: c.mln_offline_region_definition,
) status.Error!OwnedOfflineRegionDefinition {
    return switch (raw.type) {
        c.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID => blk: {
            const definition = raw.data.tile_pyramid;
            break :blk .{ .tile_pyramid = .{
                .style_url = try allocator.dupe(u8, std.mem.span(definition.style_url)),
                .bounds = values.latLngBoundsFromNative(definition.bounds),
                .min_zoom = definition.min_zoom,
                .max_zoom = definition.max_zoom,
                .pixel_ratio = definition.pixel_ratio,
                .include_ideographs = definition.include_ideographs,
            } };
        },
        c.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY => blk: {
            const definition = raw.data.geometry;
            const style_url = try allocator.dupe(u8, std.mem.span(definition.style_url));
            errdefer allocator.free(style_url);
            const geometry = try values.ownedGeometryFromNative(allocator, definition.geometry orelse return error.NativeError);
            break :blk .{ .geometry = .{
                .style_url = style_url,
                .geometry = geometry,
                .min_zoom = definition.min_zoom,
                .max_zoom = definition.max_zoom,
                .pixel_ratio = definition.pixel_ratio,
                .include_ideographs = definition.include_ideographs,
            } };
        },
        else => error.UnknownStatus,
    };
}

pub fn native(handle: *RuntimeHandle) status.BindingError!*c.mln_runtime {
    return @ptrCast(handle.native orelse return error.ClosedHandle);
}

pub fn diagnosticStore(handle: *RuntimeHandle) ?*diagnostics.DiagnosticStore {
    return handle.diagnostic_store;
}

pub fn registry(handle: *RuntimeHandle) status.BindingError!*RuntimeRegistry {
    return handle.registry orelse error.ClosedHandle;
}

pub fn registerMap(runtime_registry: *RuntimeRegistry, map: *c.mln_map) std.mem.Allocator.Error!values.MapId {
    const id = values.MapId{ .value = runtime_registry.next_map_id };
    runtime_registry.next_map_id += 1;
    try runtime_registry.maps.append(std.heap.smp_allocator, .{ .native = map, .id = id });
    return id;
}

pub fn unregisterMap(runtime_registry: *RuntimeRegistry, map: *c.mln_map) void {
    for (runtime_registry.maps.items, 0..) |registration, index| {
        if (@intFromPtr(registration.native) == @intFromPtr(map)) {
            _ = runtime_registry.maps.orderedRemove(index);
            return;
        }
    }
}

fn emptyNativeEvent() c.mln_runtime_event {
    return .{
        .size = @sizeOf(c.mln_runtime_event),
        .type = 0,
        .source_type = c.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
        .source = null,
        .code = 0,
        .payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_NONE,
        .payload = null,
        .payload_size = 0,
        .message = null,
        .message_size = 0,
    };
}

fn nulTerminated(allocator: std.mem.Allocator, value: []const u8) status.Error![:0]u8 {
    if (std.mem.indexOfScalar(u8, value, 0) != null) return error.InvalidString;
    return allocator.dupeZ(u8, value);
}

test "runtime event raw domains preserve unknown values" {
    try std.testing.expect(std.meta.eql(RuntimeEventType.fromRaw(0xfeed), RuntimeEventType{ .unknown = 0xfeed }));
    try std.testing.expect(std.meta.eql(RuntimeEventSourceType.fromRaw(0xbeef), RuntimeEventSourceType{ .unknown = 0xbeef }));
    try std.testing.expect(std.meta.eql(RuntimeEventPayloadType.fromRaw(0xace), RuntimeEventPayloadType{ .unknown = 0xace }));
    try std.testing.expect(std.meta.eql(RenderMode.fromRaw(0xbad), RenderMode{ .unknown = 0xbad }));
    try std.testing.expect(std.meta.eql(TileOperation.fromRaw(0xcafe), TileOperation{ .unknown = 0xcafe }));
    try std.testing.expect(std.meta.eql(OfflineRegionDownloadState.fromRaw(0xd00d), OfflineRegionDownloadState{ .unknown = 0xd00d }));
    try std.testing.expect(std.meta.eql(ResourceErrorReason.fromRaw(0xf00d), ResourceErrorReason{ .unknown = 0xf00d }));
}

test "runtime event payload copying owns borrowed bytes" {
    const source_id = try std.testing.allocator.dupe(u8, "composite-source");
    defer std.testing.allocator.free(source_id);
    var native_payload = c.mln_runtime_event_tile_action{
        .size = @sizeOf(c.mln_runtime_event_tile_action),
        .operation = c.MLN_TILE_OPERATION_LOAD_FROM_NETWORK,
        .tile_id = .{ .overscaled_z = 3, .wrap = -1, .canonical_z = 2, .canonical_x = 1, .canonical_y = 0 },
        .source_id = source_id.ptr,
        .source_id_size = source_id.len,
    };
    const native_event = c.mln_runtime_event{
        .size = @sizeOf(c.mln_runtime_event),
        .type = c.MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
        .source_type = c.MLN_RUNTIME_EVENT_SOURCE_MAP,
        .source = null,
        .code = 0,
        .payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
        .payload = &native_payload,
        .payload_size = @sizeOf(c.mln_runtime_event_tile_action),
        .message = null,
        .message_size = 0,
    };

    var payload = try copyPayload(std.testing.allocator, native_event);
    defer payload.deinit(std.testing.allocator);

    const tile_action = payload.tile_action;
    @memset(source_id, 'x');
    try std.testing.expect(std.meta.eql(tile_action.operation, TileOperation.load_from_network));
    try std.testing.expectEqual(@as(u32, 3), tile_action.tile_id.overscaled_z);
    try std.testing.expectEqual(@as(i32, -1), tile_action.tile_id.wrap);
    try std.testing.expectEqualSlices(u8, "composite-source", tile_action.source_id);
}

test "runtime event unknown payload copies raw bytes" {
    var raw = [_]u8{ 0xde, 0xad, 0xbe, 0xef };
    const native_event = c.mln_runtime_event{
        .size = @sizeOf(c.mln_runtime_event),
        .type = 0xffff,
        .source_type = c.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
        .source = null,
        .code = 0,
        .payload_type = 0xfeed,
        .payload = &raw,
        .payload_size = raw.len,
        .message = null,
        .message_size = 0,
    };

    var payload = try copyPayload(std.testing.allocator, native_event);
    defer payload.deinit(std.testing.allocator);

    @memset(&raw, 0);
    try std.testing.expectEqual(@as(u32, 0xfeed), payload.unknown.payload_type);
    try std.testing.expectEqualSlices(u8, &[_]u8{ 0xde, 0xad, 0xbe, 0xef }, payload.unknown.bytes);
}

test "runtime event payload copying rejects malformed borrowed payloads" {
    const null_payload_event = c.mln_runtime_event{
        .size = @sizeOf(c.mln_runtime_event),
        .type = c.MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
        .source_type = c.MLN_RUNTIME_EVENT_SOURCE_MAP,
        .source = null,
        .code = 0,
        .payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
        .payload = null,
        .payload_size = @sizeOf(c.mln_runtime_event_tile_action),
        .message = null,
        .message_size = 0,
    };
    try std.testing.expectError(error.NativeError, copyPayload(std.testing.allocator, null_payload_event));

    var undersized_payload = c.mln_runtime_event_tile_action{
        .size = @sizeOf(c.mln_runtime_event_tile_action) - 1,
        .operation = c.MLN_TILE_OPERATION_LOAD_FROM_NETWORK,
        .tile_id = .{ .overscaled_z = 0, .wrap = 0, .canonical_z = 0, .canonical_x = 0, .canonical_y = 0 },
        .source_id = null,
        .source_id_size = 0,
    };
    var undersized_event = null_payload_event;
    undersized_event.payload = &undersized_payload;
    try std.testing.expectError(error.NativeError, copyPayload(std.testing.allocator, undersized_event));

    const AlignedPayload = extern struct {
        size: u32,
        value: u32,
    };
    const alignment = @alignOf(AlignedPayload);
    try std.testing.expect(alignment > 1);
    var misaligned_storage: [@sizeOf(AlignedPayload) + alignment]u8 align(1) = undefined;
    const base_address = @intFromPtr(&misaligned_storage[0]);
    var misaligned_offset: usize = 0;
    while ((base_address + misaligned_offset) % alignment == 0) : (misaligned_offset += 1) {}
    try std.testing.expect(misaligned_offset < alignment);
    try std.testing.expectError(
        error.NativeError,
        payloadAs(AlignedPayload, @ptrCast(&misaligned_storage[misaligned_offset]), @sizeOf(AlignedPayload)),
    );
}
