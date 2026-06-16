const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const native_temp = @import("native_temp.zig");
const status = @import("status.zig");
const values = @import("values.zig");

const NativeRuntime = opaque {};

pub const MapStyleLoadedHandler = *const fn (map: *c.mln_map, context: ?*anyopaque) void;

const MapRegistration = struct {
    native: *c.mln_map,
    id: values.MapId,
    style_loaded_handler: MapStyleLoadedHandler,
    style_loaded_context: ?*anyopaque,
};

pub const RuntimeRegistry = struct {
    maps: std.ArrayList(MapRegistration),
    next_map_id: u64,
    live_offline_operations: usize,
};

const ResourceProviderState = struct {
    provider: ResourceProvider,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

const RuntimeState = struct {
    native: ?*NativeRuntime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    registry: ?*RuntimeRegistry,
    resource_transform: ?*ResourceTransform,
    resource_provider: ?*ResourceProviderState,
    active_leases: std.atomic.Value(usize),
    closing: bool,
};

pub const RuntimeLease = struct {
    state: *RuntimeState,
    native: *c.mln_runtime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,

    pub fn release(self: RuntimeLease) void {
        _ = self.state.active_leases.fetchSub(1, .seq_cst);
    }
};

pub const RegisteredMap = struct {
    registry: *RuntimeRegistry,
    id: values.MapId,
};

const RuntimeRegistrySlot = struct {
    state: ?*RuntimeState,
    generation: u64,
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

const OfflineOperationState = struct {
    runtime: RuntimeHandle,
    operation_id: OfflineOperationId,
    operation_kind: OfflineOperationKind,
    result_kind: OfflineOperationResultKind,
};

const OfflineOperationRegistrySlot = struct {
    state: ?*OfflineOperationState,
    generation: u64,
};

const RequiredOfflineOperation = struct {
    operation_id: OfflineOperationId,
    operation_kind: OfflineOperationKind,
};

var runtime_registry_lock = std.atomic.Value(bool).init(false);
var runtime_handle_registry: std.ArrayList(RuntimeRegistrySlot) = .empty;
var runtime_handle_free_list: std.ArrayList(usize) = .empty;

var resource_request_registry_lock = std.atomic.Value(bool).init(false);
var resource_request_registry: std.ArrayList(ResourceRequestRegistrySlot) = .empty;
var resource_request_free_list: std.ArrayList(usize) = .empty;

var offline_operation_registry_lock = std.atomic.Value(bool).init(false);
var offline_operation_registry: std.ArrayList(OfflineOperationRegistrySlot) = .empty;
var offline_operation_free_list: std.ArrayList(usize) = .empty;

var handle_generation_counter = std.atomic.Value(u64).init(0);
var handle_generation_seed = std.atomic.Value(u64).init(0);

const OfflineRegionSnapshotDestroyFn = *const fn (?*c.mln_offline_region_snapshot) callconv(.c) void;
const OfflineRegionListDestroyFn = *const fn (?*c.mln_offline_region_list) callconv(.c) void;

var offline_region_snapshot_destroy_for_testing: OfflineRegionSnapshotDestroyFn = c.mln_offline_region_snapshot_destroy;
var offline_region_list_destroy_for_testing: OfflineRegionListDestroyFn = c.mln_offline_region_list_destroy;

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

    fn toInputRaw(self: NetworkStatus, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!u32 {
        return switch (self) {
            .online, .offline => self.toRaw(),
            .unknown => {
                try status.setBindingDiagnostic(diagnostic_store, "network status cannot be an unknown enum value");
                return error.InvalidArgument;
            },
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
pub const OfflineOperationId = u64;

pub const OfflineOperationKind = union(enum) {
    ambient_cache,
    region_create,
    region_get,
    regions_list,
    regions_merge_database,
    region_update_metadata,
    region_get_status,
    region_set_observed,
    region_set_download_state,
    region_invalidate,
    region_delete,
    unknown: u32,

    pub fn fromRaw(raw: u32) OfflineOperationKind {
        return switch (raw) {
            c.MLN_OFFLINE_OPERATION_AMBIENT_CACHE => .ambient_cache,
            c.MLN_OFFLINE_OPERATION_REGION_CREATE => .region_create,
            c.MLN_OFFLINE_OPERATION_REGION_GET => .region_get,
            c.MLN_OFFLINE_OPERATION_REGIONS_LIST => .regions_list,
            c.MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE => .regions_merge_database,
            c.MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA => .region_update_metadata,
            c.MLN_OFFLINE_OPERATION_REGION_GET_STATUS => .region_get_status,
            c.MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED => .region_set_observed,
            c.MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE => .region_set_download_state,
            c.MLN_OFFLINE_OPERATION_REGION_INVALIDATE => .region_invalidate,
            c.MLN_OFFLINE_OPERATION_REGION_DELETE => .region_delete,
            else => .{ .unknown = raw },
        };
    }

    pub fn toRaw(self: OfflineOperationKind) u32 {
        return switch (self) {
            .ambient_cache => c.MLN_OFFLINE_OPERATION_AMBIENT_CACHE,
            .region_create => c.MLN_OFFLINE_OPERATION_REGION_CREATE,
            .region_get => c.MLN_OFFLINE_OPERATION_REGION_GET,
            .regions_list => c.MLN_OFFLINE_OPERATION_REGIONS_LIST,
            .regions_merge_database => c.MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE,
            .region_update_metadata => c.MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA,
            .region_get_status => c.MLN_OFFLINE_OPERATION_REGION_GET_STATUS,
            .region_set_observed => c.MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED,
            .region_set_download_state => c.MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE,
            .region_invalidate => c.MLN_OFFLINE_OPERATION_REGION_INVALIDATE,
            .region_delete => c.MLN_OFFLINE_OPERATION_REGION_DELETE,
            .unknown => |raw| raw,
        };
    }
};

pub const OfflineOperationResultKind = union(enum) {
    none,
    region,
    optional_region,
    region_list,
    region_status,
    unknown: u32,

    pub fn fromRaw(raw: u32) OfflineOperationResultKind {
        return switch (raw) {
            c.MLN_OFFLINE_OPERATION_RESULT_NONE => .none,
            c.MLN_OFFLINE_OPERATION_RESULT_REGION => .region,
            c.MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION => .optional_region,
            c.MLN_OFFLINE_OPERATION_RESULT_REGION_LIST => .region_list,
            c.MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS => .region_status,
            else => .{ .unknown = raw },
        };
    }

    pub fn toRaw(self: OfflineOperationResultKind) u32 {
        return switch (self) {
            .none => c.MLN_OFFLINE_OPERATION_RESULT_NONE,
            .region => c.MLN_OFFLINE_OPERATION_RESULT_REGION,
            .optional_region => c.MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION,
            .region_list => c.MLN_OFFLINE_OPERATION_RESULT_REGION_LIST,
            .region_status => c.MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS,
            .unknown => |raw| raw,
        };
    }
};

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
    /// Replacement URL copied by the binding before the native callback returns.
    ///
    /// The pointed-to storage only needs to remain valid for the handler call;
    /// string literals and context-owned storage are suitable.
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
        var native_response = try resourceResponseToNative(response, request_state.diagnostic_store);
        try status.checkStatus(c.mln_resource_request_complete(native_handle, &native_response), request_state.diagnostic_store);
        request_state.completed = true;
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

const RuntimeEvent = struct {
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
    offline_operation_completed: OfflineOperationCompletedPayload,
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

    fn toInputRaw(self: OfflineRegionDownloadState, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!u32 {
        return switch (self) {
            .inactive, .active => self.toRaw(),
            .unknown => {
                try status.setBindingDiagnostic(diagnostic_store, "offline region download state cannot be an unknown enum value");
                return error.InvalidArgument;
            },
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

    fn toInputRaw(self: ResourceErrorReason, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!u32 {
        return switch (self) {
            .none, .not_found, .server, .connection, .rate_limit, .other => self.toRaw(),
            .unknown => {
                try status.setBindingDiagnostic(diagnostic_store, "resource error reason cannot be an unknown enum value");
                return error.InvalidArgument;
            },
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

pub const OfflineOperationCompletedPayload = struct {
    operation_id: OfflineOperationId,
    operation_kind: OfflineOperationKind,
    raw_operation_kind: u32,
    result_kind: OfflineOperationResultKind,
    raw_result_kind: u32,
    result_status: i32,
    found: bool,
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
    offline_operation_completed,
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
            c.MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED => .offline_operation_completed,
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

pub const OfflineOperationHandle = enum(u128) {
    _,

    fn init(
        runtime: *RuntimeHandle,
        operation_id: OfflineOperationId,
        operation_kind: OfflineOperationKind,
        result_kind: OfflineOperationResultKind,
    ) status.Error!OfflineOperationHandle {
        if (operation_id == 0) return error.InvalidArgument;
        try registerRuntimeOfflineOperation(runtime.*);
        errdefer unregisterRuntimeOfflineOperation(runtime.*);

        const operation_state = try std.heap.smp_allocator.create(OfflineOperationState);
        operation_state.* = .{
            .runtime = runtime.*,
            .operation_id = operation_id,
            .operation_kind = operation_kind,
            .result_kind = result_kind,
        };
        errdefer std.heap.smp_allocator.destroy(operation_state);
        return try registerOfflineOperationState(operation_state);
    }

    pub fn operationId(self: OfflineOperationHandle) status.BindingError!OfflineOperationId {
        lockOfflineOperationRegistry();
        defer unlockOfflineOperationRegistry();
        const operation_state = offlineOperationState(self) orelse return error.ClosedHandle;
        return operation_state.operation_id;
    }

    fn require(
        self: OfflineOperationHandle,
        expected_runtime: *RuntimeHandle,
        operation_kind: OfflineOperationKind,
        result_kind: OfflineOperationResultKind,
    ) status.Error!RequiredOfflineOperation {
        lockOfflineOperationRegistry();
        defer unlockOfflineOperationRegistry();
        const operation_state = offlineOperationState(self) orelse return error.ClosedHandle;
        if (operation_state.runtime != expected_runtime.*) return error.InvalidState;
        if (!std.meta.eql(operation_state.operation_kind, operation_kind) or !std.meta.eql(operation_state.result_kind, result_kind)) {
            return error.InvalidState;
        }
        return .{ .operation_id = operation_state.operation_id, .operation_kind = operation_state.operation_kind };
    }

    fn requireEither(
        self: OfflineOperationHandle,
        expected_runtime: *RuntimeHandle,
        first_kind: OfflineOperationKind,
        second_kind: OfflineOperationKind,
        result_kind: OfflineOperationResultKind,
    ) status.Error!RequiredOfflineOperation {
        lockOfflineOperationRegistry();
        defer unlockOfflineOperationRegistry();
        const operation_state = offlineOperationState(self) orelse return error.ClosedHandle;
        if (operation_state.runtime != expected_runtime.*) return error.InvalidState;
        if ((!std.meta.eql(operation_state.operation_kind, first_kind) and !std.meta.eql(operation_state.operation_kind, second_kind)) or
            !std.meta.eql(operation_state.result_kind, result_kind))
        {
            return error.InvalidState;
        }
        return .{ .operation_id = operation_state.operation_id, .operation_kind = operation_state.operation_kind };
    }

    fn consume(self: OfflineOperationHandle) void {
        const operation_state = unregisterOfflineOperationState(self) orelse return;
        unregisterRuntimeOfflineOperation(operation_state.runtime);
        std.heap.smp_allocator.destroy(operation_state);
    }

    pub fn discard(self: OfflineOperationHandle) status.Error!void {
        lockOfflineOperationRegistry();
        const operation_state = offlineOperationState(self) orelse {
            unlockOfflineOperationRegistry();
            return;
        };
        const operation_id = operation_state.operation_id;
        var runtime_handle = operation_state.runtime;
        unlockOfflineOperationRegistry();

        const runtime_lease = lease(&runtime_handle) catch |err| {
            return err;
        };
        defer runtime_lease.release();
        try status.checkStatus(
            c.mln_runtime_offline_operation_discard(runtime_lease.native, operation_id),
            runtime_lease.diagnostic_store,
        );
        self.consume();
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
    offline_operation_completed,
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
            c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED => .offline_operation_completed,
            else => .{ .unknown = raw },
        };
    }
};

pub const RuntimeHandle = enum(u128) {
    _,

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
            asset_path = try nulTerminated(allocator, value, diagnostic_store, "runtime asset_path contains embedded NUL");
            native_options.asset_path = asset_path.?.ptr;
        }
        if (options.cache_path) |value| {
            cache_path = try nulTerminated(allocator, value, diagnostic_store, "runtime cache_path contains embedded NUL");
            native_options.cache_path = cache_path.?.ptr;
        }
        if (options.maximum_cache_size) |value| {
            native_options.flags |= c.MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE;
            native_options.maximum_cache_size = value;
        }

        return createNative(&native_options, diagnostic_store);
    }

    pub fn runOnce(self: *RuntimeHandle) status.Error!void {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        try status.checkStatus(c.mln_runtime_run_once(runtime_lease.native), runtime_lease.diagnostic_store);
    }

    pub fn pollEvent(self: *RuntimeHandle, allocator: std.mem.Allocator) status.Error!?OwnedRuntimeEvent {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var native_event = emptyNativeEvent();
        var has_event = false;
        try status.checkStatus(
            c.mln_runtime_poll_event(runtime_lease.native, &native_event, &has_event),
            runtime_lease.diagnostic_store,
        );
        if (!has_event) return null;

        applyEventSideEffects(self, native_event);
        return try copyRuntimeEventOwned(self, allocator, native_event);
    }

    fn operationHandle(
        self: *RuntimeHandle,
        operation_id: c.mln_offline_operation_id,
        operation_kind: OfflineOperationKind,
        result_kind: OfflineOperationResultKind,
    ) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, operation_kind, result_kind);
    }

    fn operationHandleWithRuntime(
        self: *RuntimeHandle,
        runtime: *c.mln_runtime,
        operation_id: c.mln_offline_operation_id,
        operation_kind: OfflineOperationKind,
        result_kind: OfflineOperationResultKind,
    ) status.Error!OfflineOperationHandle {
        // `runtime` must come from an active RuntimeLease that spans this call.
        return OfflineOperationHandle.init(self, operation_id, operation_kind, result_kind) catch |err| {
            if (operation_id != 0) _ = c.mln_runtime_offline_operation_discard(runtime, operation_id);
            return err;
        };
    }

    pub fn startAmbientCacheOperation(self: *RuntimeHandle, operation: AmbientCacheOperation) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_run_ambient_cache_operation_start(runtime_lease.native, operation.toRaw(), &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .ambient_cache, .none);
    }

    pub fn startCreateOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        definition: OfflineRegionDefinition,
        metadata: []const u8,
    ) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var temp = native_temp.TempStorage.initWithDiagnostics(allocator, runtime_lease.diagnostic_store);
        defer temp.deinit();
        const native_definition = try temp.offlineRegionDefinition(definition);
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_create_start(
                runtime_lease.native,
                native_definition,
                if (metadata.len == 0) null else metadata.ptr,
                metadata.len,
                &operation_id,
            ),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_create, .region);
    }

    pub fn startGetOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_get_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_get, .optional_region);
    }

    pub fn startListOfflineRegions(self: *RuntimeHandle) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_regions_list_start(runtime_lease.native, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .regions_list, .region_list);
    }

    pub fn startMergeOfflineRegionsDatabase(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        side_database_path: []const u8,
    ) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const path = try nulTerminated(allocator, side_database_path, runtime_lease.diagnostic_store, "offline merge database path contains embedded NUL");
        defer allocator.free(path);
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_regions_merge_database_start(runtime_lease.native, path.ptr, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .regions_merge_database, .region_list);
    }

    pub fn startUpdateOfflineRegionMetadata(
        self: *RuntimeHandle,
        region_id: OfflineRegionId,
        metadata: []const u8,
    ) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_update_metadata_start(
                runtime_lease.native,
                region_id,
                if (metadata.len == 0) null else metadata.ptr,
                metadata.len,
                &operation_id,
            ),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_update_metadata, .region);
    }

    pub fn startGetOfflineRegionStatus(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_get_status_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_get_status, .region_status);
    }

    pub fn startSetOfflineRegionObserved(self: *RuntimeHandle, region_id: OfflineRegionId, observed: bool) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_set_observed_start(runtime_lease.native, region_id, observed, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_set_observed, .none);
    }

    pub fn startSetOfflineRegionDownloadState(
        self: *RuntimeHandle,
        region_id: OfflineRegionId,
        download_state: OfflineRegionDownloadState,
    ) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_set_download_state_start(runtime_lease.native, region_id, try download_state.toInputRaw(runtime_lease.diagnostic_store), &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_set_download_state, .none);
    }

    pub fn startInvalidateOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_invalidate_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_invalidate, .none);
    }

    pub fn startDeleteOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OfflineOperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_offline_operation_id = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_delete_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_delete, .none);
    }

    pub fn takeOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        operation: OfflineOperationHandle,
    ) status.Error!OwnedOfflineRegion {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.requireEither(self, .region_create, .region_update_metadata, .region);
        var snapshot: ?*c.mln_offline_region_snapshot = null;
        const native_status = switch (required.operation_kind) {
            .region_create => c.mln_runtime_offline_region_create_take_result(runtime_lease.native, required.operation_id, &snapshot),
            .region_update_metadata => c.mln_runtime_offline_region_update_metadata_take_result(runtime_lease.native, required.operation_id, &snapshot),
            else => c.MLN_STATUS_INVALID_STATE,
        };
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        operation.consume();
        const snapshot_handle = snapshot orelse return error.NativeError;
        defer destroyOfflineRegionSnapshot(snapshot_handle);
        return copyOfflineRegionSnapshot(allocator, snapshot_handle);
    }

    pub fn takeOptionalOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        operation: OfflineOperationHandle,
    ) status.Error!?OwnedOfflineRegion {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.require(self, .region_get, .optional_region);
        var snapshot: ?*c.mln_offline_region_snapshot = null;
        var found = false;
        const native_status = c.mln_runtime_offline_region_get_take_result(runtime_lease.native, required.operation_id, &snapshot, &found);
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        operation.consume();
        if (!found) return null;
        const snapshot_handle = snapshot orelse return error.NativeError;
        defer destroyOfflineRegionSnapshot(snapshot_handle);
        return try copyOfflineRegionSnapshot(allocator, snapshot_handle);
    }

    pub fn takeOfflineRegionList(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        operation: OfflineOperationHandle,
    ) status.Error!OfflineRegionList {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.requireEither(self, .regions_list, .regions_merge_database, .region_list);
        var list: ?*c.mln_offline_region_list = null;
        const native_status = switch (required.operation_kind) {
            .regions_list => c.mln_runtime_offline_regions_list_take_result(runtime_lease.native, required.operation_id, &list),
            .regions_merge_database => c.mln_runtime_offline_regions_merge_database_take_result(runtime_lease.native, required.operation_id, &list),
            else => c.MLN_STATUS_INVALID_STATE,
        };
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        operation.consume();
        const list_handle = list orelse return error.NativeError;
        defer destroyOfflineRegionList(list_handle);
        return copyOfflineRegionList(allocator, list_handle);
    }

    pub fn takeOfflineRegionStatus(self: *RuntimeHandle, operation: OfflineOperationHandle) status.Error!OfflineRegionStatus {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.require(self, .region_get_status, .region_status);
        var native_status_value: c.mln_offline_region_status = undefined;
        native_status_value.size = @sizeOf(c.mln_offline_region_status);
        const native_status = c.mln_runtime_offline_region_get_status_take_result(runtime_lease.native, required.operation_id, &native_status_value);
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        operation.consume();
        return offlineStatusFromNative(native_status_value);
    }

    pub fn setResourceTransform(self: *RuntimeHandle, transform: ?ResourceTransform) status.Error!void {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const runtime_state = runtime_lease.state;
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
                c.mln_runtime_set_resource_transform(runtime_lease.native, &native_transform),
                runtime_lease.diagnostic_store,
            );
            const previous = runtime_state.resource_transform;
            runtime_state.resource_transform = replacement;
            if (previous) |old| std.heap.smp_allocator.destroy(old);
            return;
        }

        try status.checkStatus(
            c.mln_runtime_clear_resource_transform(runtime_lease.native),
            runtime_lease.diagnostic_store,
        );
        if (runtime_state.resource_transform) |old| {
            runtime_state.resource_transform = null;
            std.heap.smp_allocator.destroy(old);
        }
    }

    pub fn setResourceProvider(self: *RuntimeHandle, provider: ResourceProvider) status.Error!void {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const runtime_state = runtime_lease.state;
        const replacement = try std.heap.smp_allocator.create(ResourceProviderState);
        errdefer std.heap.smp_allocator.destroy(replacement);
        replacement.* = .{ .provider = provider, .diagnostic_store = runtime_lease.diagnostic_store };
        var native_provider = c.mln_resource_provider{
            .size = @sizeOf(c.mln_resource_provider),
            .callback = resourceProviderTrampoline,
            .user_data = replacement,
        };
        try status.checkStatus(
            c.mln_runtime_set_resource_provider(runtime_lease.native, &native_provider),
            runtime_lease.diagnostic_store,
        );
        const previous = runtime_state.resource_provider;
        runtime_state.resource_provider = replacement;
        if (previous) |old_provider| std.heap.smp_allocator.destroy(old_provider);
    }

    pub fn close(self: *RuntimeHandle) status.Error!void {
        const runtime_close = try beginRuntimeClose(self.*) orelse return;
        status.checkStatus(c.mln_runtime_destroy(runtime_close.native), runtime_close.diagnostic_store) catch |err| {
            cancelRuntimeClose(runtime_close.state);
            return err;
        };
        if (runtime_close.state.resource_transform) |old| {
            runtime_close.state.resource_transform = null;
            std.heap.smp_allocator.destroy(old);
        }
        if (runtime_close.state.resource_provider) |old| {
            runtime_close.state.resource_provider = null;
            std.heap.smp_allocator.destroy(old);
        }
        runtime_close.registry.maps.deinit(std.heap.smp_allocator);
        std.heap.smp_allocator.destroy(runtime_close.registry);
        const runtime_state = finishRuntimeClose(self.*) orelse runtime_close.state;
        std.heap.smp_allocator.destroy(runtime_state);
    }
};

pub fn getNetworkStatus(diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!NetworkStatus {
    var raw: u32 = 0;
    try status.checkStatus(c.mln_network_status_get(&raw), diagnostic_store);
    return NetworkStatus.fromRaw(raw);
}

pub fn setNetworkStatus(network_status: NetworkStatus, diagnostic_store: ?*diagnostics.DiagnosticStore) status.Error!void {
    try status.checkStatus(c.mln_network_status_set(try network_status.toInputRaw(diagnostic_store)), diagnostic_store);
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
    runtime_registry.* = .{ .maps = .empty, .next_map_id = 1, .live_offline_operations = 0 };
    errdefer std.heap.smp_allocator.destroy(runtime_registry);

    const runtime_state = try std.heap.smp_allocator.create(RuntimeState);
    runtime_state.* = .{
        .native = @ptrCast(runtime.?),
        .diagnostic_store = diagnostic_store,
        .registry = runtime_registry,
        .resource_transform = null,
        .resource_provider = null,
        .active_leases = std.atomic.Value(usize).init(0),
        .closing = false,
    };
    errdefer std.heap.smp_allocator.destroy(runtime_state);

    return try registerRuntimeState(runtime_state);
}

fn copyRuntimeEventOwned(
    handle: *RuntimeHandle,
    allocator: std.mem.Allocator,
    native_event: c.mln_runtime_event,
) status.Error!OwnedRuntimeEvent {
    const event = runtimeEventFromNative(handle, native_event);
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

fn destroyOfflineRegionSnapshot(snapshot: ?*c.mln_offline_region_snapshot) void {
    offline_region_snapshot_destroy_for_testing(snapshot);
}

fn destroyOfflineRegionList(list: ?*c.mln_offline_region_list) void {
    offline_region_list_destroy_for_testing(list);
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

fn applyEventSideEffects(handle: *RuntimeHandle, native_event: c.mln_runtime_event) void {
    if (native_event.type != c.MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) return;
    if (native_event.source_type != c.MLN_RUNTIME_EVENT_SOURCE_MAP) return;
    const source_ptr = native_event.source orelse return;
    const registration = mapRegistrationForNativeSource(handle.*, source_ptr) orelse return;
    registration.style_loaded_handler(registration.native, registration.style_loaded_context);
}

fn mapRegistrationForNativeSource(handle: RuntimeHandle, source_ptr: *anyopaque) ?MapRegistration {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle) orelse return null;
    const runtime_registry = runtime_state.registry orelse return null;
    for (runtime_registry.maps.items) |registration| {
        if (@intFromPtr(registration.native) == @intFromPtr(source_ptr)) return registration;
    }
    return null;
}

fn mapIdForNativeSource(handle: *RuntimeHandle, source_type: u32, source: ?*anyopaque) ?values.MapId {
    if (source_type != c.MLN_RUNTIME_EVENT_SOURCE_MAP) return null;
    const source_ptr = source orelse return null;
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle.*) orelse return null;
    const runtime_registry = runtime_state.registry orelse return null;
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
    const native_response = out_response orelse return c.MLN_STATUS_INVALID_ARGUMENT;
    native_response.*.size = @sizeOf(c.mln_resource_transform_response);
    native_response.*.url = null;
    const copied_url = std.heap.smp_allocator.dupe(u8, if (url == null) "" else std.mem.span(url)) catch return c.MLN_STATUS_NATIVE_ERROR;
    defer std.heap.smp_allocator.free(copied_url);
    const response = transform.handler(transform.context, .{
        .kind = ResourceKind.fromRaw(kind),
        .url = copied_url,
    });
    if (response.replacement_url) |replacement_url| {
        return c.mln_resource_transform_response_set_url(
            native_response,
            replacement_url.ptr,
            replacement_url.len,
        );
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

fn registerRuntimeState(runtime_state: *RuntimeState) std.mem.Allocator.Error!RuntimeHandle {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    if (runtime_handle_free_list.items.len > 0) {
        const slot_index = runtime_handle_free_list.pop().?;
        runtime_handle_registry.items[slot_index].state = runtime_state;
        runtime_handle_registry.items[slot_index].generation = nextHandleGeneration();
        return runtimeHandle(slot_index + 1, runtime_handle_registry.items[slot_index].generation);
    }

    const generation = nextHandleGeneration();
    try runtime_handle_free_list.ensureTotalCapacity(std.heap.smp_allocator, runtime_handle_registry.items.len + 1);
    try runtime_handle_registry.append(std.heap.smp_allocator, .{ .state = runtime_state, .generation = generation });
    return runtimeHandle(runtime_handle_registry.items.len, generation);
}

fn runtimeHandle(index: usize, generation: u64) RuntimeHandle {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn runtimeHandleIndex(handle: RuntimeHandle) ?usize {
    const index = @intFromEnum(handle) & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn runtimeHandleGeneration(handle: RuntimeHandle) u64 {
    return @intCast(@intFromEnum(handle) >> 64);
}

fn runtimeState(handle: RuntimeHandle) ?*RuntimeState {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();
    return runtimeStateLocked(handle);
}

fn runtimeStateLocked(handle: RuntimeHandle) ?*RuntimeState {
    const index = runtimeHandleIndex(handle) orelse return null;
    if (index > runtime_handle_registry.items.len) return null;
    const slot = runtime_handle_registry.items[index - 1];
    if (slot.generation != runtimeHandleGeneration(handle)) return null;
    return slot.state;
}

pub fn lease(handle: *RuntimeHandle) status.BindingError!RuntimeLease {
    return runtimeLease(handle.*);
}

fn runtimeLease(handle: RuntimeHandle) status.BindingError!RuntimeLease {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle) orelse return error.ClosedHandle;
    if (runtime_state.closing) return error.ActiveBorrow;
    const runtime: *c.mln_runtime = @ptrCast(runtime_state.native orelse return error.ClosedHandle);
    _ = runtime_state.active_leases.fetchAdd(1, .seq_cst);
    return .{
        .state = runtime_state,
        .native = runtime,
        .diagnostic_store = runtime_state.diagnostic_store,
    };
}

const RuntimeClose = struct {
    state: *RuntimeState,
    native: *c.mln_runtime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    registry: *RuntimeRegistry,
};

fn beginRuntimeClose(handle: RuntimeHandle) status.Error!?RuntimeClose {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const index = runtimeHandleIndex(handle) orelse return null;
    if (index > runtime_handle_registry.items.len) return null;
    const slot = runtime_handle_registry.items[index - 1];
    if (slot.generation != runtimeHandleGeneration(handle)) return null;
    const runtime_state = slot.state orelse return null;
    if (runtime_state.closing) return error.ActiveBorrow;
    if (runtime_state.active_leases.load(.seq_cst) != 0) return error.ActiveBorrow;
    const runtime_registry = runtime_state.registry orelse return null;
    if (runtime_registry.maps.items.len != 0) {
        try status.setBindingDiagnostic(runtime_state.diagnostic_store, "runtime has live maps");
        return error.InvalidState;
    }
    if (runtime_registry.live_offline_operations != 0) {
        try status.setBindingDiagnostic(runtime_state.diagnostic_store, "runtime has live offline operations");
        return error.InvalidState;
    }
    const runtime: *c.mln_runtime = @ptrCast(runtime_state.native orelse return null);
    runtime_state.closing = true;
    return .{
        .state = runtime_state,
        .native = runtime,
        .diagnostic_store = runtime_state.diagnostic_store,
        .registry = runtime_registry,
    };
}

fn cancelRuntimeClose(runtime_state: *RuntimeState) void {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    runtime_state.closing = false;
}

fn finishRuntimeClose(handle: RuntimeHandle) ?*RuntimeState {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const index = runtimeHandleIndex(handle) orelse return null;
    if (index > runtime_handle_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &runtime_handle_registry.items[slot_index];
    if (slot.generation != runtimeHandleGeneration(handle)) return null;
    const runtime_state = slot.state orelse return null;
    slot.state = null;
    slot.generation = nextHandleGeneration();
    runtime_state.native = null;
    runtime_state.registry = null;
    runtime_handle_free_list.appendAssumeCapacity(slot_index);
    return runtime_state;
}

fn registerRuntimeOfflineOperation(handle: RuntimeHandle) status.BindingError!void {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle) orelse return error.ClosedHandle;
    if (runtime_state.closing) return error.ActiveBorrow;
    const runtime_registry = runtime_state.registry orelse return error.ClosedHandle;
    runtime_registry.live_offline_operations += 1;
}

fn unregisterRuntimeOfflineOperation(handle: RuntimeHandle) void {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle) orelse return;
    const runtime_registry = runtime_state.registry orelse return;
    if (runtime_registry.live_offline_operations > 0) runtime_registry.live_offline_operations -= 1;
}

fn lockRuntimeRegistry() void {
    while (runtime_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockRuntimeRegistry() void {
    runtime_registry_lock.store(false, .seq_cst);
}

fn registerResourceRequestState(request_state: *ResourceRequestState) std.mem.Allocator.Error!ResourceRequestHandle {
    lockResourceRequestRegistry();
    defer unlockResourceRequestRegistry();

    if (resource_request_free_list.items.len > 0) {
        const slot_index = resource_request_free_list.pop().?;
        resource_request_registry.items[slot_index].state = request_state;
        resource_request_registry.items[slot_index].generation = nextHandleGeneration();
        return resourceRequestHandle(slot_index + 1, resource_request_registry.items[slot_index].generation);
    }

    const generation = nextHandleGeneration();
    try resource_request_free_list.ensureTotalCapacity(std.heap.smp_allocator, resource_request_registry.items.len + 1);
    try resource_request_registry.append(std.heap.smp_allocator, .{ .state = request_state, .generation = generation });
    return resourceRequestHandle(resource_request_registry.items.len, generation);
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
    slot.generation = nextHandleGeneration();
    resource_request_free_list.appendAssumeCapacity(slot_index);
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

fn registerOfflineOperationState(operation_state: *OfflineOperationState) std.mem.Allocator.Error!OfflineOperationHandle {
    lockOfflineOperationRegistry();
    defer unlockOfflineOperationRegistry();

    if (offline_operation_free_list.items.len > 0) {
        const slot_index = offline_operation_free_list.pop().?;
        offline_operation_registry.items[slot_index].state = operation_state;
        offline_operation_registry.items[slot_index].generation = nextHandleGeneration();
        return offlineOperationHandle(slot_index + 1, offline_operation_registry.items[slot_index].generation);
    }

    const generation = nextHandleGeneration();
    try offline_operation_free_list.ensureTotalCapacity(std.heap.smp_allocator, offline_operation_registry.items.len + 1);
    try offline_operation_registry.append(std.heap.smp_allocator, .{ .state = operation_state, .generation = generation });
    return offlineOperationHandle(offline_operation_registry.items.len, generation);
}

fn offlineOperationHandle(index: usize, generation: u64) OfflineOperationHandle {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn offlineOperationIndex(handle: OfflineOperationHandle) ?usize {
    const index = @intFromEnum(handle) & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn offlineOperationGeneration(handle: OfflineOperationHandle) u64 {
    return @intCast(@intFromEnum(handle) >> 64);
}

fn offlineOperationState(handle: OfflineOperationHandle) ?*OfflineOperationState {
    const index = offlineOperationIndex(handle) orelse return null;
    if (index > offline_operation_registry.items.len) return null;
    const slot = offline_operation_registry.items[index - 1];
    if (slot.generation != offlineOperationGeneration(handle)) return null;
    return slot.state;
}

fn unregisterOfflineOperationState(handle: OfflineOperationHandle) ?*OfflineOperationState {
    lockOfflineOperationRegistry();
    defer unlockOfflineOperationRegistry();

    const index = offlineOperationIndex(handle) orelse return null;
    if (index > offline_operation_registry.items.len) return null;
    const slot_index = index - 1;
    const slot = &offline_operation_registry.items[slot_index];
    if (slot.generation != offlineOperationGeneration(handle)) return null;
    const operation_state = slot.state orelse return null;
    slot.state = null;
    slot.generation = nextHandleGeneration();
    offline_operation_free_list.appendAssumeCapacity(slot_index);
    return operation_state;
}

fn lockOfflineOperationRegistry() void {
    while (offline_operation_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockOfflineOperationRegistry() void {
    offline_operation_registry_lock.store(false, .seq_cst);
}

pub fn nextHandleGeneration() u64 {
    const seed = handleGenerationSeed();
    const counter = handle_generation_counter.fetchAdd(1, .seq_cst) +% 1;
    const generation = splitMix64(seed +% counter);
    if (generation == 0) return 1;
    return generation;
}

fn handleGenerationSeed() u64 {
    const existing = handle_generation_seed.load(.seq_cst);
    if (existing != 0) return existing;

    const candidate = splitMix64(
        @intFromPtr(&handle_generation_seed) ^ @intFromPtr(&handle_generation_counter) ^ 0x9e37_79b9_7f4a_7c15,
    );
    const seed = if (candidate == 0) 0x243f_6a88_85a3_08d3 else candidate;
    if (handle_generation_seed.cmpxchgStrong(0, seed, .seq_cst, .seq_cst)) |installed| {
        return installed;
    }
    return seed;
}

fn splitMix64(input: u64) u64 {
    var value = input +% 0x9e37_79b9_7f4a_7c15;
    value = (value ^ (value >> 30)) *% 0xbf58_476d_1ce4_e5b9;
    value = (value ^ (value >> 27)) *% 0x94d0_49bb_1331_11eb;
    return value ^ (value >> 31);
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

fn resourceResponseToNative(
    response: ResourceResponse,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!c.mln_resource_response {
    return .{
        .size = @sizeOf(c.mln_resource_response),
        .status = response.status.toRaw(),
        .error_reason = try response.error_reason.toInputRaw(diagnostic_store),
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
        c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED => blk: {
            const payload = try payloadAs(c.mln_runtime_event_offline_operation_completed, native_event.payload, native_event.payload_size);
            break :blk .{ .offline_operation_completed = .{
                .operation_id = payload.operation_id,
                .operation_kind = OfflineOperationKind.fromRaw(payload.operation_kind),
                .raw_operation_kind = payload.operation_kind,
                .result_kind = OfflineOperationResultKind.fromRaw(payload.result_kind),
                .raw_result_kind = payload.result_kind,
                .result_status = payload.result_status,
                .found = payload.found,
            } };
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

fn rawStatusError(raw_status: i32) status.NativeStatusError {
    return switch (raw_status) {
        c.MLN_STATUS_INVALID_ARGUMENT => error.InvalidArgument,
        c.MLN_STATUS_INVALID_STATE => error.InvalidState,
        c.MLN_STATUS_WRONG_THREAD => error.WrongThread,
        c.MLN_STATUS_UNSUPPORTED => error.Unsupported,
        c.MLN_STATUS_NATIVE_ERROR => error.NativeError,
        else => error.UnknownStatus,
    };
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
    const runtime_state = runtimeState(handle.*) orelse return error.ClosedHandle;
    if (runtime_state.closing) return error.ActiveBorrow;
    return @ptrCast(runtime_state.native orelse return error.ClosedHandle);
}

pub fn diagnosticStore(handle: *RuntimeHandle) ?*diagnostics.DiagnosticStore {
    const runtime_state = runtimeState(handle.*) orelse return null;
    return runtime_state.diagnostic_store;
}

pub fn registerMap(
    runtime: *RuntimeHandle,
    map: *c.mln_map,
    style_loaded_handler: MapStyleLoadedHandler,
    style_loaded_context: ?*anyopaque,
) status.Error!RegisteredMap {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(runtime.*) orelse return error.ClosedHandle;
    if (runtime_state.closing) return error.ActiveBorrow;
    const runtime_registry = runtime_state.registry orelse return error.ClosedHandle;
    const id = values.MapId{ .value = runtime_registry.next_map_id };
    runtime_registry.next_map_id += 1;
    try runtime_registry.maps.append(std.heap.smp_allocator, .{
        .native = map,
        .id = id,
        .style_loaded_handler = style_loaded_handler,
        .style_loaded_context = style_loaded_context,
    });
    return .{ .registry = runtime_registry, .id = id };
}

pub fn unregisterMap(runtime_registry: *RuntimeRegistry, map: *c.mln_map) void {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

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

fn nulTerminated(
    allocator: std.mem.Allocator,
    value: []const u8,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    diagnostic_message: []const u8,
) status.Error![:0]u8 {
    if (std.mem.indexOfScalar(u8, value, 0) != null) {
        try status.setBindingDiagnostic(diagnostic_store, diagnostic_message);
        return error.InvalidString;
    }
    return allocator.dupeZ(u8, value);
}

const offline_style_url_for_testing = "http://example.com/offline-style.json";

var snapshot_destroy_count_for_testing: usize = 0;
var list_destroy_count_for_testing: usize = 0;

fn countingOfflineRegionSnapshotDestroy(snapshot: ?*c.mln_offline_region_snapshot) callconv(.c) void {
    snapshot_destroy_count_for_testing += 1;
    c.mln_offline_region_snapshot_destroy(snapshot);
}

fn countingOfflineRegionListDestroy(list: ?*c.mln_offline_region_list) callconv(.c) void {
    list_destroy_count_for_testing += 1;
    c.mln_offline_region_list_destroy(list);
}

fn tempPathForTesting(allocator: std.mem.Allocator, sub_path: []const u8, filename: []const u8) ![]u8 {
    return std.fmt.allocPrint(allocator, ".zig-cache/tmp/{s}/{s}", .{ sub_path, filename });
}

fn offlineTileDefinitionForTesting() OfflineRegionDefinition {
    return .{ .tile_pyramid = .{
        .style_url = offline_style_url_for_testing,
        .bounds = .{
            .southwest = .{ .latitude = 1.0, .longitude = 2.0 },
            .northeast = .{ .latitude = 3.0, .longitude = 4.0 },
        },
        .min_zoom = 5.0,
        .max_zoom = 6.0,
        .pixel_ratio = 2.0,
    } };
}

fn waitForOfflineOperationForTesting(runtime: *RuntimeHandle, operation: OfflineOperationHandle) !void {
    const operation_id = try operation.operationId();
    for (0..5000) |_| {
        try runtime.runOnce();
        while (try runtime.pollEvent(std.testing.allocator)) |event| {
            var owned_event = event;
            defer owned_event.deinit();
            const payload = switch (owned_event.payload) {
                .offline_operation_completed => |completed| completed,
                else => continue,
            };
            if (payload.operation_id != operation_id) continue;
            try std.testing.expectEqual(@as(i32, c.MLN_STATUS_OK), payload.result_status);
            return;
        }
        try std.testing.io.sleep(.fromMilliseconds(1), .awake);
    }
    return error.EventNotObserved;
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

test "offline operation take-result failures preserve handle state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.operationHandle(9_999_999, .region_get_status, .region_status);
    try std.testing.expectError(error.InvalidArgument, runtime.takeOfflineRegionStatus(operation));
    try std.testing.expectEqual(@as(OfflineOperationId, 9_999_999), try operation.operationId());

    operation.consume();
}

test "offline operation discard failures preserve handle state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.operationHandle(9_999_997, .region_get_status, .region_status);
    try std.testing.expectError(error.InvalidArgument, operation.discard());
    try std.testing.expectEqual(@as(OfflineOperationId, 9_999_997), try operation.operationId());

    operation.consume();
}

test "runtime close rejects live offline operations" {
    var diagnostic_store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer diagnostic_store.deinit();

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, &diagnostic_store);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.operationHandle(9_999_998, .region_get_status, .region_status);
    try std.testing.expectError(error.InvalidState, runtime.close());
    try std.testing.expectEqualStrings("runtime has live offline operations", diagnostic_store.get().?.message);

    operation.consume();
    try runtime.close();
    runtime_open = false;
}

test "offline region snapshot destroy runs when copied output allocation fails" {
    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPathForTesting(std.testing.allocator, tmp.sub_path[0..], "snapshot-copy-failure-cache.db");
    defer std.testing.allocator.free(cache_path);

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{ .cache_path = cache_path }, null);
    defer runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.startCreateOfflineRegion(std.testing.allocator, offlineTileDefinitionForTesting(), &.{});
    try waitForOfflineOperationForTesting(&runtime, operation);

    var failing_allocator = std.testing.FailingAllocator.init(std.testing.allocator, .{ .fail_index = 0 });
    snapshot_destroy_count_for_testing = 0;
    offline_region_snapshot_destroy_for_testing = countingOfflineRegionSnapshotDestroy;
    defer offline_region_snapshot_destroy_for_testing = c.mln_offline_region_snapshot_destroy;

    try std.testing.expectError(error.OutOfMemory, runtime.takeOfflineRegion(failing_allocator.allocator(), operation));
    try std.testing.expectEqual(@as(usize, 1), snapshot_destroy_count_for_testing);
    try std.testing.expectError(error.ClosedHandle, operation.operationId());
}

test "offline region list destroy runs when copied output allocation fails" {
    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPathForTesting(std.testing.allocator, tmp.sub_path[0..], "list-copy-failure-cache.db");
    defer std.testing.allocator.free(cache_path);

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{ .cache_path = cache_path }, null);
    defer runtime.close() catch @panic("runtime close failed");

    const create_operation = try runtime.startCreateOfflineRegion(std.testing.allocator, offlineTileDefinitionForTesting(), &.{});
    try waitForOfflineOperationForTesting(&runtime, create_operation);
    var region = try runtime.takeOfflineRegion(std.testing.allocator, create_operation);
    defer region.deinit();

    const list_operation = try runtime.startListOfflineRegions();
    try waitForOfflineOperationForTesting(&runtime, list_operation);

    var failing_allocator = std.testing.FailingAllocator.init(std.testing.allocator, .{ .fail_index = 0 });
    list_destroy_count_for_testing = 0;
    offline_region_list_destroy_for_testing = countingOfflineRegionListDestroy;
    defer offline_region_list_destroy_for_testing = c.mln_offline_region_list_destroy;

    try std.testing.expectError(error.OutOfMemory, runtime.takeOfflineRegionList(failing_allocator.allocator(), list_operation));
    try std.testing.expectEqual(@as(usize, 1), list_destroy_count_for_testing);
    try std.testing.expectError(error.ClosedHandle, list_operation.operationId());
}
