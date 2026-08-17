const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const native_temp = @import("native_temp.zig");
const status = @import("status.zig");
const values = @import("values.zig");

const MapRegistration = struct {
    native: c.mln_map,
    id: values.MapId,
};

pub const RuntimeRegistry = struct {
    maps: std.ArrayList(MapRegistration),
    next_map_id: u64,
    live_operations: usize,
};

const ResourceProviderState = struct {
    provider: ResourceProvider,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

const RuntimeState = struct {
    notification_source: c.mln_notification_source,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    registry: ?*RuntimeRegistry,
    resource_roots_lock: std.Io.Mutex,
    resource_transforms: std.ArrayList(*ResourceTransform),
    http_header_transforms: std.ArrayList(*HttpHeaderTransform),
    resource_providers: std.ArrayList(*ResourceProviderState),
    active_leases: std.atomic.Value(usize),
    closing: bool,
    runtime_destroyed: bool,
};

pub const RuntimeLease = struct {
    state: *RuntimeState,
    native: c.mln_runtime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,

    pub fn release(self: RuntimeLease) void {
        _ = self.state.active_leases.fetchSub(1, .seq_cst);
    }
};

pub const RegisteredMap = struct {
    registry: *RuntimeRegistry,
    id: values.MapId,
};

const ResourceRequestState = struct {
    native: c.mln_resource_request_handle,
    completed: bool,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

const OperationState = struct {
    runtime: RuntimeHandle,
    operation_id: c.mln_operation,
    operation_kind: OperationKind,
    active_uses: std.atomic.Value(usize),
    closing: bool,
    result_kind: OperationResultKind,
};

const OperationRegistrySlot = struct {
    state: ?*OperationState,
    generation: u64,
};

pub const OperationLease = struct {
    state: *OperationState,
    native: c.mln_operation,

    pub fn release(self: OperationLease) void {
        _ = self.state.active_uses.fetchSub(1, .seq_cst);
    }
};

pub const RequiredOperation = struct {
    lease: OperationLease,
    operation_kind: OperationKind,
};

// The registry gives copied Zig handle values stable closed-handle detection
// while the C operation handle owns completion and result state.
var runtime_registry_lock = std.atomic.Value(bool).init(false);
var runtime_handle_registry: std.AutoHashMapUnmanaged(c.mln_runtime, *RuntimeState) = .empty;

var resource_request_registry_lock = std.atomic.Value(bool).init(false);
var resource_request_registry: std.AutoHashMapUnmanaged(c.mln_resource_request_handle, *ResourceRequestState) = .empty;

var operation_registry_lock = std.atomic.Value(bool).init(false);
var operation_registry: std.ArrayList(OperationRegistrySlot) = .empty;
var operation_free_list: std.ArrayList(usize) = .empty;

var handle_generation_counter = std.atomic.Value(u64).init(0);
var handle_generation_seed = std.atomic.Value(u64).init(0);

const OfflineRegionSnapshotDestroyFn = *const fn (c.mln_offline_region_snapshot) callconv(.c) void;
const OfflineRegionListDestroyFn = *const fn (c.mln_offline_region_list) callconv(.c) void;
const NotificationSourceCloseFn = *const fn (c.mln_notification_source) callconv(.c) c.mln_status;

var notification_source_close_for_testing: NotificationSourceCloseFn = c.mln_notification_source_close;

var offline_region_snapshot_destroy_for_testing: OfflineRegionSnapshotDestroyFn = c.mln_offline_region_snapshot_destroy;
var offline_region_list_destroy_for_testing: OfflineRegionListDestroyFn = c.mln_offline_region_list_destroy;

/// Runtime creation options. A null field takes the C API default.
pub const RuntimeOptions = struct {
    asset_path: ?[]const u8 = null,
    cache_path: ?[]const u8 = null,
    /// Runtime-originated event types this runtime queues. A null takes the C
    /// API default, which selects every type the library reports, including the
    /// types this binding does not name. See `RuntimeHandle.setEventMask`.
    event_mask: ?RuntimeEventMask = null,
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

pub const OperationKind = enum {
    runtime_barrier,
    map_camera_query,
    map_still_image,
    map_loaded_style_json,
    map_style_url,
    map_get_style_source_info,
    map_copy_style_source_attribution,
    map_copy_style_source_url,
    map_get_style_source_tile_urls,
    map_list_style_source_ids,
    map_get_style_layer_info,
    map_list_style_layer_ids,
    map_get_style_layer_json,
    map_get_style_light_property,
    map_get_style_transition_options,
    map_get_layer_property,
    map_get_layer_filter,
    map_copy_layer_source_layer,
    map_copy_layer_source_id,
    map_get_style_image_info,
    map_copy_style_image_pixels,
    map_copy_style_image_stretches,
    map_get_image_source_coordinates,
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
    set_maximum_ambient_cache_size,
    render_attach,
    render_resize,
    render_barrier,
    render_reduce_memory_use,
    render_clear_data,
    render_dump_debug_logs,
    render_set_feature_state,
    render_get_feature_state,
    render_remove_feature_state,
    render_query,
    render_readback,
    render_set_target,
    render_detach,
    acquired_frame_release,
};

pub const OperationResultKind = enum {
    none,
    camera,
    region,
    optional_style_source_info,
    optional_style_layer_info,
    optional_string,
    string,
    string_list,
    queried_feature_list,
    style_transition_options,
    optional_style_image_info,
    optional_image_stretches,
    optional_coordinates,
    optional_region,
    region_list,
    region_status,
    render_session,
    render_readback,
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
    geometry: []const u8,
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
    geometry: []u8,
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
                allocator.free(definition.geometry);
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
    /// Replacement URL, copied before the native callback returns. The storage
    /// only needs to stay valid for the handler call.
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

pub const HttpHeaderTransformRequest = struct {
    kind: ResourceKind,
    url: []const u8,
};

pub const HttpHeader = struct {
    name: []const u8,
    value: []const u8,
};

pub const HttpHeaderTransformHandler = *const fn (
    context: ?*anyopaque,
    request: HttpHeaderTransformRequest,
) []const HttpHeader;

pub const HttpHeaderTransform = struct {
    handler: HttpHeaderTransformHandler,
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
    /// URL entering the network layer, preserving configured scheme aliases.
    requested_url: []const u8,
    /// URL to fetch, after tile server normalization.
    resolved_url: []const u8,
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

pub const ResourceRequestHandle = enum(c.mln_resource_request_handle) {
    _,

    pub fn complete(self: ResourceRequestHandle, response: ResourceResponse) status.Error!void {
        lockResourceRequestRegistry();
        defer unlockResourceRequestRegistry();

        const request_state = resourceRequestState(self) orelse return error.ClosedHandle;
        if (request_state.completed) return error.AlreadyCompleted;
        if (request_state.native == 0) return error.ClosedHandle;
        const native_handle = request_state.native;
        var native_response = try resourceResponseToNative(response, request_state.diagnostic_store);
        try status.checkStatus(c.mln_resource_request_complete(native_handle, &native_response), request_state.diagnostic_store);
        request_state.completed = true;
    }

    pub fn cancelled(self: ResourceRequestHandle) status.Error!bool {
        lockResourceRequestRegistry();
        defer unlockResourceRequestRegistry();

        const request_state = resourceRequestState(self) orelse return error.ClosedHandle;
        if (request_state.native == 0) return error.ClosedHandle;
        const native_handle = request_state.native;
        var is_cancelled = false;
        try status.checkStatus(c.mln_resource_request_cancelled(native_handle, &is_cancelled), request_state.diagnostic_store);
        return is_cancelled;
    }

    pub fn release(self: ResourceRequestHandle) void {
        lockResourceRequestRegistry();
        defer unlockResourceRequestRegistry();

        const request_state = unregisterResourceRequestState(self) orelse return;
        defer std.heap.smp_allocator.destroy(request_state);
        c.mln_resource_request_release(request_state.native);
    }
};

/// One drained batch of copied runtime events.
///
/// The batch owns its events and remains valid after another drain or after the
/// runtime closes. Call `deinit` when finished with it.
pub const EventBatch = struct {
    allocator: std.mem.Allocator,
    events: []RuntimeEvent,

    /// Releases every event and the batch storage.
    pub fn deinit(self: *EventBatch) void {
        for (self.events) |*event| event.deinit();
        self.allocator.free(self.events);
        self.events = &.{};
    }

    /// Number of events this batch reports.
    pub fn len(self: EventBatch) usize {
        return self.events.len;
    }

    /// Reads the event at `index` in queue order.
    pub fn at(self: EventBatch, index: usize) status.Error!*const RuntimeEvent {
        if (index >= self.events.len) return error.InvalidArgument;
        return &self.events[index];
    }
};

const RuntimeEventView = struct {
    event_type: RuntimeEventType,
    source_type: RuntimeEventSourceType,
    /// Native identity of the object this event came from.
    source: RuntimeEventSourceId,
    source_id: ?values.MapId,
    payload_type: RuntimeEventPayloadType,
    /// Secondary detail whose meaning `event_type` selects.
    code: i32,
    /// Borrowed from the batch's message arena.
    message: []const u8,
    payload: RuntimeEventPayload,

    /// Copies this event into storage the returned value owns.
    fn toOwned(self: RuntimeEventView, allocator: std.mem.Allocator) status.Error!RuntimeEvent {
        const message = try allocator.dupe(u8, self.message);
        errdefer allocator.free(message);
        return .{
            .allocator = allocator,
            .event_type = self.event_type,
            .source_type = self.source_type,
            .source = self.source,
            .source_id = self.source_id,
            .payload_type = self.payload_type,
            .code = self.code,
            .message = message,
            .payload = try ownedPayload(allocator, self.payload),
        };
    }
};

/// One drained runtime event, copied into storage this value owns.
pub const RuntimeEvent = struct {
    allocator: std.mem.Allocator,
    event_type: RuntimeEventType,
    source_type: RuntimeEventSourceType,
    /// Native identity of the object this event came from, which `source_type`
    /// names. Every event carries it, including an event whose source type or
    /// source map this binding cannot resolve.
    source: RuntimeEventSourceId,
    /// Map this event came from, for a map-sourced event whose map is still
    /// open. Runtime-sourced events and maps this binding no longer tracks
    /// report null; `source` still names the object either way.
    source_id: ?values.MapId,
    payload_type: RuntimeEventPayloadType,
    /// Secondary detail whose meaning `event_type` selects: a raw
    /// `CameraChangeMode` for camera change events, MapLibre Native's map load
    /// error ordinal for `map_loading_failed`, the final raw status for
    /// `command_finished` (also carried typed on the payload), and 0 for every
    /// other event type.
    code: i32,
    message: []const u8,
    payload: RuntimeEventPayload,

    /// Copies this event into storage the returned value owns.
    pub fn clone(self: RuntimeEvent, allocator: std.mem.Allocator) status.Error!RuntimeEvent {
        const message = try allocator.dupe(u8, self.message);
        errdefer allocator.free(message);
        return .{
            .allocator = allocator,
            .event_type = self.event_type,
            .source_type = self.source_type,
            .source = self.source,
            .source_id = self.source_id,
            .payload_type = self.payload_type,
            .code = self.code,
            .message = message,
            .payload = try ownedPayload(allocator, self.payload),
        };
    }

    pub fn deinit(self: *RuntimeEvent) void {
        self.payload.deinit(self.allocator);
        self.allocator.free(self.message);
        self.message = "";
        self.payload = .none;
    }
};

/// Typed event payload the event's `payload_type` selects.
///
/// Every member is a value except the bytes of an `unknown` payload, which the
/// containing event owns.
pub const RuntimeEventPayload = union(enum) {
    none,
    render_frame: RenderFramePayload,
    render_map: RenderMapPayload,
    tile_action: TileActionPayload,
    offline_region_status: OfflineRegionStatusPayload,
    offline_region_response_error: OfflineRegionResponseErrorPayload,
    offline_region_tile_count_limit: OfflineRegionTileCountLimitPayload,
    command_finished: CommandFinishedPayload,
    camera_transition_finished: CameraTransitionFinishedPayload,
    unknown: UnknownPayload,

    fn deinit(self: *RuntimeEventPayload, allocator: std.mem.Allocator) void {
        switch (self.*) {
            .unknown => |payload| allocator.free(payload.bytes),
            else => {},
        }
        self.* = .none;
    }
};

/// Camera change kind carried by the `code` of camera change events.
pub const CameraChangeMode = union(enum) {
    /// The camera reached its new value without an animated transition.
    immediate,
    /// The camera moved as part of an animated transition.
    animated,
    /// A change mode this binding does not name yet, kept as its raw value.
    unknown: i32,

    /// Decodes the `code` of a `map_camera_will_change` or
    /// `map_camera_did_change` event.
    pub fn fromRaw(raw: i32) CameraChangeMode {
        return switch (raw) {
            c.MLN_CAMERA_CHANGE_MODE_IMMEDIATE => .immediate,
            c.MLN_CAMERA_CHANGE_MODE_ANIMATED => .animated,
            else => .{ .unknown = raw },
        };
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

/// Payload for `map_tile_action` events. The event message carries the source
/// ID.
pub const TileActionPayload = struct {
    operation: TileOperation,
    tile_id: TileId,
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

/// Payload for `map_camera_transition_finished` events.
///
/// A camera command carrying `AnimationOptions.transition_id` reports this
/// payload once, however the transition ends: completed, superseded, cancelled,
/// or an instant zero-duration jump. A rejected command starts no transition
/// and emits no event. The payload establishes transition identity, not a
/// completion reason.
pub const CameraTransitionFinishedPayload = struct {
    /// The `AnimationOptions.transition_id` of the command that started this
    /// transition.
    transition_id: u64,
};
pub const CommandDisposition = union(enum) {
    committed,
    superseded,
    failed,
    cancelled,
    unknown: u32,

    fn fromRaw(raw: u32) CommandDisposition {
        return switch (raw) {
            c.MLN_COMMAND_DISPOSITION_COMMITTED => .committed,
            c.MLN_COMMAND_DISPOSITION_SUPERSEDED => .superseded,
            c.MLN_COMMAND_DISPOSITION_FAILED => .failed,
            c.MLN_COMMAND_DISPOSITION_CANCELLED => .cancelled,
            else => .{ .unknown = raw },
        };
    }
};

pub const CommandFinishedPayload = struct {
    command_id: u64,
    disposition: CommandDisposition,
    /// The command's final status: void for a committed command, and the
    /// failure a failed disposition reports otherwise (for example
    /// `error.NotFound` for a style removal whose ID named nothing).
    status: status.NativeStatusError!void,
    /// The map snapshot generation the commit published, or zero when the
    /// command committed no generation. A later snapshot that reports this
    /// generation or a newer one observes the commit.
    generation: u64,
};

/// Payload of a type this binding does not name, kept as the event's fixed
/// payload window: the batch's event size less the payload's offset in the
/// event.
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
    map_camera_transition_finished,
    command_finished,
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
            c.MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED => .map_camera_transition_finished,
            c.MLN_RUNTIME_EVENT_COMMAND_FINISHED => .command_finished,
            else => .{ .unknown = raw },
        };
    }
};

/// Event types a map or a runtime queues.
///
/// Each field names the `RuntimeEventType` tag it selects, so a host reads a
/// mask, sets one field, and writes it back. A map reads the fields
/// `all_map_events` names and ignores the rest; a runtime reads the fields
/// `all_runtime_events` names and ignores the rest. So both setters accept
/// `all`.
pub const RuntimeEventMask = struct {
    map_camera_will_change: bool = false,
    map_camera_is_changing: bool = false,
    map_camera_did_change: bool = false,
    map_style_loaded: bool = false,
    map_loading_started: bool = false,
    map_loading_finished: bool = false,
    map_loading_failed: bool = false,
    map_idle: bool = false,
    map_render_update_available: bool = false,
    map_render_error: bool = false,
    map_still_image_finished: bool = false,
    map_still_image_failed: bool = false,
    map_render_frame_started: bool = false,
    map_render_frame_finished: bool = false,
    map_render_map_started: bool = false,
    map_render_map_finished: bool = false,
    map_style_image_missing: bool = false,
    map_tile_action: bool = false,
    map_camera_transition_finished: bool = false,
    offline_region_status_changed: bool = false,
    offline_region_response_error: bool = false,
    offline_region_tile_count_limit_exceeded: bool = false,
    command_finished: bool = false,
    /// Bits reported by a newer native library that this binding does not name.
    unknown_bits: u64 = 0,

    /// Selects no event type.
    pub const none = RuntimeEventMask{};
    /// Selects every map-originated event type this binding names.
    pub const all_map_events = fromRaw(c.MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS);
    /// Selects every runtime-originated event type this binding names.
    pub const all_runtime_events = fromRaw(c.MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS);
    /// Selects every event type this binding names.
    pub const all = fromRaw(c.MLN_RUNTIME_EVENT_MASK_ALL);

    /// Returns the mask selecting every type either mask selects.
    pub fn unionWith(self: RuntimeEventMask, other: RuntimeEventMask) RuntimeEventMask {
        return fromRaw(self.toRaw() | other.toRaw());
    }

    /// Reports whether this mask selects `event_type`.
    pub fn contains(self: RuntimeEventMask, event_type: RuntimeEventType) bool {
        switch (event_type) {
            .unknown => |raw| return raw < 64 and self.toRaw() & (@as(u64, 1) << @intCast(raw)) != 0,
            inline else => |_, tag| return @field(self, @tagName(tag)),
        }
    }

    /// Reports whether this mask selects no event type.
    pub fn isEmpty(self: RuntimeEventMask) bool {
        return self.toRaw() == 0;
    }

    fn toRaw(self: RuntimeEventMask) u64 {
        @setEvalBranchQuota(4000);
        var raw = self.unknown_bits;
        inline for (@typeInfo(RuntimeEventMask).@"struct".fields) |field| {
            if (field.type == bool) {
                if (@field(self, field.name)) raw |= maskBitForField(field.name);
            }
        }
        return raw;
    }

    fn fromRaw(raw: u64) RuntimeEventMask {
        @setEvalBranchQuota(4000);
        var mask = RuntimeEventMask{};
        mask.unknown_bits = raw & ~@as(u64, c.MLN_RUNTIME_EVENT_MASK_ALL);
        inline for (@typeInfo(RuntimeEventMask).@"struct".fields) |field| {
            if (field.type == bool) {
                if (raw & maskBitForField(field.name) != 0) {
                    @field(mask, field.name) = true;
                }
            }
        }
        return mask;
    }

    // Each bit comes from the generated constant its field is named after, so
    // the mask cannot drift from the event type enum.
    fn maskBitForField(comptime field_name: []const u8) u64 {
        const upper_name = comptime blk: {
            var buffer: [field_name.len]u8 = undefined;
            for (field_name, 0..) |character, index| {
                buffer[index] = std.ascii.toUpper(character);
            }
            break :blk buffer;
        };
        return @field(c, "MLN_RUNTIME_EVENT_MASK_" ++ upper_name);
    }
};

/// Identity of the object a runtime event came from, as native reported it.
///
/// The value names one object for the life of the process, so comparing two
/// events' identities is meaningful even after the object it names is closed.
/// It is an identity only: this binding builds no handle from it.
pub const RuntimeEventSourceId = enum(u64) {
    /// What an event with no source reports.
    none = 0,
    _,
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

pub const NotificationEndpointKind = union(enum) {
    runtime_events,
    operation,
    adapter_resource_requests,
    adapter_log_records,
    render_frames,
    driver_work,
    unknown: u32,

    fn fromRaw(raw: u32) NotificationEndpointKind {
        return switch (raw) {
            c.MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS => .runtime_events,
            c.MLN_NOTIFICATION_ENDPOINT_OPERATION => .operation,
            c.MLN_NOTIFICATION_ENDPOINT_ADAPTER_RESOURCE_REQUESTS => .adapter_resource_requests,
            c.MLN_NOTIFICATION_ENDPOINT_ADAPTER_LOG_RECORDS => .adapter_log_records,
            c.MLN_NOTIFICATION_ENDPOINT_RENDER_FRAMES => .render_frames,
            c.MLN_NOTIFICATION_ENDPOINT_DRIVER_WORK => .driver_work,
            else => .{ .unknown = raw },
        };
    }
};

/// Stable identity for matching an operation with notification readiness.
/// This value cannot be used to invoke the native operation API.
pub const OperationId = enum(u64) {
    _,
};

pub const NotificationEndpointId = union(enum) {
    runtime_events: u64,
    operation: OperationId,
    adapter_resource_requests: u64,
    adapter_log_records: u64,
    render_frames: u64,
    driver_work: u64,
    unknown: u64,

    fn fromRaw(kind: NotificationEndpointKind, raw: u64) NotificationEndpointId {
        return switch (kind) {
            .runtime_events => .{ .runtime_events = raw },
            .operation => .{ .operation = @enumFromInt(raw) },
            .adapter_resource_requests => .{ .adapter_resource_requests = raw },
            .adapter_log_records => .{ .adapter_log_records = raw },
            .render_frames => .{ .render_frames = raw },
            .driver_work => .{ .driver_work = raw },
            .unknown => .{ .unknown = raw },
        };
    }
};

pub const ReadyEndpoint = struct {
    kind: NotificationEndpointKind,
    id: NotificationEndpointId,
};

pub const ReadyBatch = struct {
    allocator: std.mem.Allocator,
    endpoints: []ReadyEndpoint,

    pub fn deinit(self: *ReadyBatch) void {
        self.allocator.free(self.endpoints);
        self.endpoints = &.{};
    }
};

/// Native callback that schedules a later ready-endpoint drain.
///
/// The callback may run on any thread. It must not call the C API.
pub const NotificationCallback = *const fn (?*anyopaque) callconv(.c) void;

pub const OperationHandle = enum(u128) {
    _,

    pub fn init(
        runtime: *const RuntimeHandle,
        operation_id: c.mln_operation,
        operation_kind: OperationKind,
        result_kind: OperationResultKind,
    ) status.Error!OperationHandle {
        if (operation_id == 0) return error.InvalidArgument;
        try registerRuntimeOperation(runtime.*);
        errdefer unregisterRuntimeOperation(runtime.*);
        const operation_state = try std.heap.smp_allocator.create(OperationState);
        operation_state.* = .{
            .runtime = runtime.*,
            .operation_id = operation_id,
            .operation_kind = operation_kind,
            .active_uses = std.atomic.Value(usize).init(0),
            .closing = false,
            .result_kind = result_kind,
        };
        errdefer std.heap.smp_allocator.destroy(operation_state);
        return try registerOperationState(operation_state);
    }

    /// Returns a stable, non-operable identity for matching ready endpoints.
    pub fn id(self: OperationHandle) status.BindingError!OperationId {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        return @enumFromInt(operation_lease.native);
    }

    pub fn require(
        self: OperationHandle,
        expected_runtime: *const RuntimeHandle,
        operation_kind: OperationKind,
        result_kind: OperationResultKind,
    ) status.Error!RequiredOperation {
        const operation_lease = try operationLease(self);
        errdefer operation_lease.release();
        if (operation_lease.state.runtime != expected_runtime.*) return error.InvalidState;
        if (!std.meta.eql(operation_lease.state.operation_kind, operation_kind) or
            !std.meta.eql(operation_lease.state.result_kind, result_kind))
        {
            return error.InvalidState;
        }
        return .{ .lease = operation_lease, .operation_kind = operation_lease.state.operation_kind };
    }

    /// Reports whether this operation has reached a terminal disposition.
    pub fn poll(self: OperationHandle) status.Error!bool {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        var completed = false;
        try status.checkStatus(c.mln_operation_poll(operation_lease.native, &completed), null);
        return completed;
    }

    /// Waits for this operation to complete within `timeout_ms`.
    pub fn wait(self: OperationHandle, timeout_ms: i64) status.Error!bool {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        var completed = false;
        try status.checkStatus(c.mln_operation_wait(operation_lease.native, timeout_ms, &completed), null);
        return completed;
    }
    /// Waits for terminal completion and reports the operation's own failure.
    ///
    /// A timeout returns `false`. A terminal non-OK status is mapped to a Zig
    /// error after its copied diagnostic replaces the runtime diagnostic.
    pub fn waitForSuccess(self: OperationHandle, timeout_ms: i64) status.Error!bool {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        var completed = false;
        try status.checkStatus(
            c.mln_operation_wait(operation_lease.native, timeout_ms, &completed),
            null,
        );
        if (!completed) return false;
        try checkNativeOperationResult(
            operation_lease.native,
            operation_lease.state.runtime,
        );
        return true;
    }

    /// Requests cancellation of this operation. Cancellation may run while
    /// another thread waits on the same operation.
    pub fn cancel(self: OperationHandle) status.Error!void {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        try status.checkStatus(c.mln_operation_cancel(operation_lease.native), null);
    }

    /// Returns the terminal native status of this completed operation.
    pub fn resultStatus(self: OperationHandle) status.Error!i32 {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        var result_status: c.mln_status = c.MLN_STATUS_NATIVE_ERROR;
        try status.checkStatus(
            c.mln_operation_get_status(operation_lease.native, &result_status),
            null,
        );
        return result_status;
    }

    /// Copies this completed operation's diagnostic into caller-owned storage.
    pub fn copyDiagnostic(
        self: OperationHandle,
        allocator: std.mem.Allocator,
    ) status.Error!values.OwnedString {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        var required: usize = 0;
        try status.checkStatus(
            c.mln_operation_copy_diagnostic(operation_lease.native, null, 0, &required),
            null,
        );
        const message = try allocator.alloc(u8, required);
        errdefer allocator.free(message);
        var copied: usize = 0;
        try status.checkStatus(
            c.mln_operation_copy_diagnostic(
                operation_lease.native,
                if (message.len == 0) null else message.ptr,
                message.len,
                &copied,
            ),
            null,
        );
        if (copied != message.len) return error.NativeError;
        return .{ .allocator = allocator, .value = message };
    }

    /// Releases this operation observer and any untaken completed result.
    /// A release begun during another native call waits for that use to finish.
    pub fn release(self: OperationHandle) void {
        const operation_state = beginOperationRelease(self) orelse return;
        while (operation_state.active_uses.load(.seq_cst) != 0) {
            std.Thread.yield() catch {};
        }
        finishOperationRelease(self, operation_state);
        c.mln_operation_release(operation_state.operation_id);
        unregisterRuntimeOperation(operation_state.runtime);
        std.heap.smp_allocator.destroy(operation_state);
    }

    fn requireEither(
        self: OperationHandle,
        expected_runtime: *RuntimeHandle,
        first_kind: OperationKind,
        second_kind: OperationKind,
        result_kind: OperationResultKind,
    ) status.Error!RequiredOperation {
        const operation_lease = try operationLease(self);
        errdefer operation_lease.release();
        if (operation_lease.state.runtime != expected_runtime.*) return error.InvalidState;
        if ((!std.meta.eql(operation_lease.state.operation_kind, first_kind) and
            !std.meta.eql(operation_lease.state.operation_kind, second_kind)) or
            !std.meta.eql(operation_lease.state.result_kind, result_kind))
        {
            return error.InvalidState;
        }
        return .{ .lease = operation_lease, .operation_kind = operation_lease.state.operation_kind };
    }

    pub fn discard(self: OperationHandle) status.Error!void {
        const operation_lease = try operationLease(self);
        defer operation_lease.release();
        try status.checkStatus(
            c.mln_operation_discard_result(operation_lease.native),
            null,
        );
    }
};

pub const RuntimeEventPayloadType = union(enum) {
    none,
    render_frame,
    render_map,
    tile_action,
    offline_region_status,
    offline_region_response_error,
    offline_region_tile_count_limit,
    camera_transition_finished,
    command_finished,
    unknown: u32,

    fn fromRaw(raw: u32) RuntimeEventPayloadType {
        return switch (raw) {
            c.MLN_RUNTIME_EVENT_PAYLOAD_NONE => .none,
            c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => .render_frame,
            c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => .render_map,
            c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => .tile_action,
            c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => .offline_region_status,
            c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => .offline_region_response_error,
            c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => .offline_region_tile_count_limit,
            c.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED => .camera_transition_finished,
            c.MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED => .command_finished,
            else => .{ .unknown = raw },
        };
    }
};

pub const RuntimeHandle = enum(c.mln_runtime) {
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
        if (options.event_mask) |value| native_options.event_mask = value.toRaw();

        return createNative(&native_options, diagnostic_store);
    }

    pub fn barrierStart(self: *RuntimeHandle) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_barrier_start(runtime_lease.native, &operation),
            runtime_lease.diagnostic_store,
        );
        return OperationHandle.init(self, operation, .runtime_barrier, .none) catch |err| {
            c.mln_operation_release(operation);
            return err;
        };
    }

    /// Installs or replaces the receiver scheduling callback.
    ///
    /// `user_data` must remain valid until this callback is replaced, cleared,
    /// or the runtime closes.
    pub fn setNotificationCallback(
        self: *RuntimeHandle,
        callback: NotificationCallback,
        user_data: ?*anyopaque,
    ) status.Error!void {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        try status.checkStatus(
            c.mln_notification_source_set_callback(
                runtime_lease.state.notification_source,
                callback,
                user_data,
            ),
            runtime_lease.diagnostic_store,
        );
    }

    /// Clears the receiver scheduling callback after in-flight entries return.
    pub fn clearNotificationCallback(self: *RuntimeHandle) status.Error!void {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        try status.checkStatus(
            c.mln_notification_source_clear_callback(
                runtime_lease.state.notification_source,
            ),
            runtime_lease.diagnostic_store,
        );
    }

    /// Drains and copies the endpoints currently ready on this runtime's
    /// receiver-scoped notification source.
    pub fn drainReady(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
    ) status.Error!ReadyBatch {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();

        var native_batch: c.mln_ready_batch = 0;
        try status.checkStatus(
            c.mln_notification_source_drain_ready(
                runtime_lease.state.notification_source,
                &native_batch,
            ),
            runtime_lease.diagnostic_store,
        );
        defer c.mln_ready_batch_release(native_batch);

        var view: c.mln_ready_batch_view = std.mem.zeroes(c.mln_ready_batch_view);
        view.size = @sizeOf(c.mln_ready_batch_view);
        try status.checkStatus(
            c.mln_ready_batch_get(native_batch, &view),
            runtime_lease.diagnostic_store,
        );
        const endpoint_size: usize = view.endpoint_size;
        const endpoint_bytes: []const u8 = if (view.endpoint_count == 0 or view.endpoints == null)
            &.{}
        else
            @as([*]const u8, @ptrCast(view.endpoints))[0 .. view.endpoint_count * endpoint_size];
        const endpoints = try allocator.alloc(ReadyEndpoint, view.endpoint_count);
        errdefer allocator.free(endpoints);
        for (endpoints, 0..) |*endpoint, index| {
            var native_endpoint = std.mem.zeroes(c.mln_ready_endpoint);
            const copied = @min(endpoint_size, @sizeOf(c.mln_ready_endpoint));
            @memcpy(
                std.mem.asBytes(&native_endpoint)[0..copied],
                endpoint_bytes[index * endpoint_size ..][0..copied],
            );
            const kind = NotificationEndpointKind.fromRaw(native_endpoint.kind);
            endpoint.* = .{
                .kind = kind,
                .id = NotificationEndpointId.fromRaw(kind, native_endpoint.id),
            };
        }
        return .{ .allocator = allocator, .endpoints = endpoints };
    }

    /// Drains this runtime's queued events into one copied batch.
    ///
    /// Narrowing a subscription gates later events and keeps queued ones, so a
    /// batch still reports the events a host already caused.
    pub fn drainEvents(self: *RuntimeHandle, allocator: std.mem.Allocator) status.Error!EventBatch {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const runtime_state = runtime_lease.state;

        var native_batch: c.mln_event_batch = 0;
        try status.checkStatus(
            c.mln_runtime_drain_events(runtime_lease.native, &native_batch),
            runtime_lease.diagnostic_store,
        );
        defer c.mln_event_batch_release(native_batch);
        var view: c.mln_runtime_event_batch_view = std.mem.zeroes(c.mln_runtime_event_batch_view);
        view.size = @sizeOf(c.mln_runtime_event_batch_view);
        try status.checkStatus(
            c.mln_event_batch_get(native_batch, &view),
            runtime_lease.diagnostic_store,
        );
        const event_size: usize = view.event_size;
        const events: []const u8 = if (view.event_count == 0 or view.events == null)
            &.{}
        else
            @as([*]const u8, @ptrCast(view.events))[0 .. view.event_count * event_size];
        const messages: []const u8 = if (view.messages_size == 0 or view.messages == null)
            ""
        else
            view.messages[0..view.messages_size];
        const copied = try allocator.alloc(RuntimeEvent, view.event_count);
        var initialized: usize = 0;
        errdefer {
            for (copied[0..initialized]) |*event| event.deinit();
            allocator.free(copied);
        }
        for (copied, 0..) |*event, index| {
            event.* = try eventViewAt(runtime_state, events, event_size, messages, index).toOwned(allocator);
            initialized += 1;
        }
        return .{ .allocator = allocator, .events = copied };
    }

    /// Selects which runtime-originated event types this runtime queues.
    ///
    /// A runtime reads the fields `RuntimeEventMask.all_runtime_events` names
    /// and ignores the rest. Narrowing gates later events and keeps queued ones,
    /// so a host drains what it already caused. Offline region events also
    /// require the region to be observed.
    pub fn setEventMask(self: *RuntimeHandle, mask: RuntimeEventMask) status.Error!void {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        try status.checkStatus(
            c.mln_runtime_set_event_mask(runtime_lease.native, mask.toRaw()),
            runtime_lease.diagnostic_store,
        );
    }

    /// Reports which runtime-originated event types this runtime queues. A
    /// runtime that has not been narrowed reports `RuntimeEventMask.all`.
    pub fn eventMask(self: *RuntimeHandle) status.Error!RuntimeEventMask {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var raw: u64 = 0;
        try status.checkStatus(
            c.mln_runtime_get_event_mask(runtime_lease.native, &raw),
            runtime_lease.diagnostic_store,
        );
        return RuntimeEventMask.fromRaw(raw);
    }

    fn operationHandle(
        self: *RuntimeHandle,
        operation_id: c.mln_operation,
        operation_kind: OperationKind,
        result_kind: OperationResultKind,
    ) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, operation_kind, result_kind);
    }

    fn operationHandleWithRuntime(
        self: *RuntimeHandle,
        _: c.mln_runtime,
        operation_id: c.mln_operation,
        operation_kind: OperationKind,
        result_kind: OperationResultKind,
    ) status.Error!OperationHandle {
        return OperationHandle.init(self, operation_id, operation_kind, result_kind) catch |err| {
            if (operation_id != 0) c.mln_operation_release(operation_id);
            return err;
        };
    }

    pub fn startAmbientCacheOperation(self: *RuntimeHandle, operation: AmbientCacheOperation) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_run_ambient_cache_operation_start(runtime_lease.native, operation.toRaw(), &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .ambient_cache, .none);
    }

    /// Starts a change to this runtime's maximum ambient cache size. Lowering
    /// it evicts ambient resources; offline regions are unaffected.
    pub fn startSetMaximumAmbientCacheSize(self: *RuntimeHandle, size: u64) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_set_maximum_ambient_cache_size_start(runtime_lease.native, size, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .set_maximum_ambient_cache_size, .none);
    }

    pub fn startCreateOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        definition: OfflineRegionDefinition,
        metadata: []const u8,
    ) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var temp = native_temp.TempStorage.initWithDiagnostics(allocator, runtime_lease.diagnostic_store);
        defer temp.deinit();
        const native_definition = try temp.offlineRegionDefinition(definition);
        var operation_id: c.mln_operation = 0;
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

    pub fn startGetOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_get_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_get, .optional_region);
    }

    pub fn startListOfflineRegions(self: *RuntimeHandle) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
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
    ) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const path = try nulTerminated(allocator, side_database_path, runtime_lease.diagnostic_store, "offline merge database path contains embedded NUL");
        defer allocator.free(path);
        var operation_id: c.mln_operation = 0;
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
    ) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
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

    pub fn startGetOfflineRegionStatus(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_get_status_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_get_status, .region_status);
    }

    pub fn startSetOfflineRegionObserved(self: *RuntimeHandle, region_id: OfflineRegionId, observed: bool) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
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
    ) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_set_download_state_start(runtime_lease.native, region_id, try download_state.toInputRaw(runtime_lease.diagnostic_store), &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_set_download_state, .none);
    }

    pub fn startInvalidateOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_invalidate_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_invalidate, .none);
    }

    pub fn startDeleteOfflineRegion(self: *RuntimeHandle, region_id: OfflineRegionId) status.Error!OperationHandle {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var operation_id: c.mln_operation = 0;
        try status.checkStatus(
            c.mln_runtime_offline_region_delete_start(runtime_lease.native, region_id, &operation_id),
            runtime_lease.diagnostic_store,
        );
        return self.operationHandleWithRuntime(runtime_lease.native, operation_id, .region_delete, .none);
    }

    pub fn takeOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        operation: OperationHandle,
    ) status.Error!OwnedOfflineRegion {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.requireEither(self, .region_create, .region_update_metadata, .region);
        defer required.lease.release();
        var snapshot: c.mln_offline_region_snapshot = 0;
        const native_status = switch (required.operation_kind) {
            .region_create => c.mln_runtime_offline_region_create_take_result(required.lease.native, &snapshot),
            .region_update_metadata => c.mln_runtime_offline_region_update_metadata_take_result(required.lease.native, &snapshot),
            else => c.MLN_STATUS_INVALID_STATE,
        };
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        if (snapshot == 0) return error.NativeError;
        const snapshot_handle = snapshot;
        defer destroyOfflineRegionSnapshot(snapshot_handle);
        return copyOfflineRegionSnapshot(allocator, snapshot_handle);
    }

    pub fn takeOptionalOfflineRegion(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        operation: OperationHandle,
    ) status.Error!?OwnedOfflineRegion {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.require(self, .region_get, .optional_region);
        defer required.lease.release();
        var snapshot: c.mln_offline_region_snapshot = 0;
        var found = false;
        const native_status = c.mln_runtime_offline_region_get_take_result(required.lease.native, &snapshot, &found);
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        if (!found) return null;
        if (snapshot == 0) return error.NativeError;
        const snapshot_handle = snapshot;
        defer destroyOfflineRegionSnapshot(snapshot_handle);
        return try copyOfflineRegionSnapshot(allocator, snapshot_handle);
    }

    pub fn takeOfflineRegionList(
        self: *RuntimeHandle,
        allocator: std.mem.Allocator,
        operation: OperationHandle,
    ) status.Error!OfflineRegionList {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.requireEither(self, .regions_list, .regions_merge_database, .region_list);
        defer required.lease.release();
        var list: c.mln_offline_region_list = 0;
        const native_status = switch (required.operation_kind) {
            .regions_list => c.mln_runtime_offline_regions_list_take_result(required.lease.native, &list),
            .regions_merge_database => c.mln_runtime_offline_regions_merge_database_take_result(required.lease.native, &list),
            else => c.MLN_STATUS_INVALID_STATE,
        };
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        if (list == 0) return error.NativeError;
        const list_handle = list;
        defer destroyOfflineRegionList(list_handle);
        return copyOfflineRegionList(allocator, list_handle);
    }

    pub fn takeOfflineRegionStatus(self: *RuntimeHandle, operation: OperationHandle) status.Error!OfflineRegionStatus {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        const required = try operation.require(self, .region_get_status, .region_status);
        defer required.lease.release();
        var native_status_value: c.mln_offline_region_status = undefined;
        native_status_value.size = @sizeOf(c.mln_offline_region_status);
        const native_status = c.mln_runtime_offline_region_get_status_take_result(required.lease.native, &native_status_value);
        try status.checkStatus(native_status, runtime_lease.diagnostic_store);
        return offlineStatusFromNative(native_status_value);
    }

    /// Registers, replaces, or clears the runtime-scoped URL transform.
    /// The binding retains accepted callback roots through every terminal
    /// command disposition and releases them when the runtime closes.
    pub fn setResourceTransform(self: *RuntimeHandle, transform: ?ResourceTransform) status.Error!u64 {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var command_id: u64 = 0;
        if (transform) |value| {
            const replacement = try std.heap.smp_allocator.create(ResourceTransform);
            errdefer std.heap.smp_allocator.destroy(replacement);
            replacement.* = value;
            std.Io.Threaded.mutexLock(&runtime_lease.state.resource_roots_lock);
            defer std.Io.Threaded.mutexUnlock(&runtime_lease.state.resource_roots_lock);
            try runtime_lease.state.resource_transforms.append(std.heap.smp_allocator, replacement);
            errdefer _ = runtime_lease.state.resource_transforms.pop();
            var native_transform = c.mln_resource_transform{
                .size = @sizeOf(c.mln_resource_transform),
                .callback = resourceTransformTrampoline,
                .user_data = replacement,
            };
            try status.checkStatus(c.mln_runtime_set_resource_transform(
                runtime_lease.native,
                &native_transform,
                &command_id,
            ), runtime_lease.diagnostic_store);
            return command_id;
        }
        try status.checkStatus(c.mln_runtime_clear_resource_transform(
            runtime_lease.native,
            &command_id,
        ), runtime_lease.diagnostic_store);
        return command_id;
    }

    pub fn setResourceProvider(self: *RuntimeHandle, provider: ?ResourceProvider) status.Error!u64 {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var command_id: u64 = 0;
        if (provider) |value| {
            const replacement = try std.heap.smp_allocator.create(ResourceProviderState);
            errdefer std.heap.smp_allocator.destroy(replacement);
            replacement.* = .{ .provider = value, .diagnostic_store = runtime_lease.diagnostic_store };
            std.Io.Threaded.mutexLock(&runtime_lease.state.resource_roots_lock);
            defer std.Io.Threaded.mutexUnlock(&runtime_lease.state.resource_roots_lock);
            try runtime_lease.state.resource_providers.append(std.heap.smp_allocator, replacement);
            errdefer _ = runtime_lease.state.resource_providers.pop();
            var native_provider = c.mln_resource_provider{
                .size = @sizeOf(c.mln_resource_provider),
                .callback = resourceProviderTrampoline,
                .user_data = replacement,
            };
            try status.checkStatus(c.mln_runtime_set_resource_provider(
                runtime_lease.native,
                &native_provider,
                &command_id,
            ), runtime_lease.diagnostic_store);
            return command_id;
        }
        try status.checkStatus(c.mln_runtime_clear_resource_provider(
            runtime_lease.native,
            &command_id,
        ), runtime_lease.diagnostic_store);
        return command_id;
    }

    pub fn setHttpHeaderTransform(self: *RuntimeHandle, transform: ?HttpHeaderTransform) status.Error!u64 {
        const runtime_lease = try lease(self);
        defer runtime_lease.release();
        var command_id: u64 = 0;
        if (transform) |value| {
            const replacement = try std.heap.smp_allocator.create(HttpHeaderTransform);
            errdefer std.heap.smp_allocator.destroy(replacement);
            replacement.* = value;
            std.Io.Threaded.mutexLock(&runtime_lease.state.resource_roots_lock);
            defer std.Io.Threaded.mutexUnlock(&runtime_lease.state.resource_roots_lock);
            try runtime_lease.state.http_header_transforms.append(std.heap.smp_allocator, replacement);
            errdefer _ = runtime_lease.state.http_header_transforms.pop();
            var native_transform = c.mln_http_header_transform{
                .size = @sizeOf(c.mln_http_header_transform),
                .callback = httpHeaderTransformTrampoline,
                .user_data = replacement,
            };
            try status.checkStatus(c.mln_runtime_set_http_header_transform(
                runtime_lease.native,
                &native_transform,
                &command_id,
            ), runtime_lease.diagnostic_store);
            return command_id;
        }
        try status.checkStatus(c.mln_runtime_clear_http_header_transform(
            runtime_lease.native,
            &command_id,
        ), runtime_lease.diagnostic_store);
        return command_id;
    }

    pub fn close(self: *RuntimeHandle) status.Error!void {
        const runtime_close = try beginRuntimeClose(self.*) orelse return;
        if (!runtime_close.state.runtime_destroyed) {
            var operation: c.mln_operation = 0;
            status.checkStatus(
                c.mln_runtime_close_start(runtime_close.native, &operation),
                runtime_close.diagnostic_store,
            ) catch |err| {
                cancelRuntimeClose(runtime_close.state);
                return err;
            };
            waitNativeOperation(operation, runtime_close.diagnostic_store) catch |err| {
                c.mln_operation_release(operation);
                cancelRuntimeClose(runtime_close.state);
                return err;
            };
            c.mln_operation_release(operation);
            runtime_close.state.runtime_destroyed = true;
        }
        try status.checkStatus(
            notification_source_close_for_testing(runtime_close.state.notification_source),
            runtime_close.diagnostic_store,
        );
        for (runtime_close.state.resource_transforms.items) |root| std.heap.smp_allocator.destroy(root);
        runtime_close.state.resource_transforms.deinit(std.heap.smp_allocator);
        for (runtime_close.state.http_header_transforms.items) |root| std.heap.smp_allocator.destroy(root);
        runtime_close.state.http_header_transforms.deinit(std.heap.smp_allocator);
        for (runtime_close.state.resource_providers.items) |root| std.heap.smp_allocator.destroy(root);
        runtime_close.state.resource_providers.deinit(std.heap.smp_allocator);
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
    var notification_source: c.mln_notification_source = 0;
    try status.checkStatus(
        c.mln_notification_source_create(&notification_source),
        diagnostic_store,
    );
    errdefer _ = notification_source_close_for_testing(notification_source);
    native_options.notification_source = notification_source;
    var operation: c.mln_operation = 0;
    try status.checkStatus(c.mln_runtime_create_start(native_options, &operation), diagnostic_store);
    defer c.mln_operation_release(operation);
    try waitNativeOperation(operation, diagnostic_store);
    var runtime: c.mln_runtime = 0;
    try status.checkStatus(c.mln_runtime_create_take_result(operation, &runtime), diagnostic_store);

    const runtime_registry = try std.heap.smp_allocator.create(RuntimeRegistry);
    runtime_registry.* = .{ .maps = .empty, .next_map_id = 1, .live_operations = 0 };
    errdefer std.heap.smp_allocator.destroy(runtime_registry);

    const runtime_state = try std.heap.smp_allocator.create(RuntimeState);
    runtime_state.* = .{
        .notification_source = notification_source,
        .diagnostic_store = diagnostic_store,
        .registry = runtime_registry,
        .resource_roots_lock = std.Io.Mutex.init,
        .resource_transforms = .empty,
        .http_header_transforms = .empty,
        .resource_providers = .empty,
        .active_leases = std.atomic.Value(usize).init(0),
        .closing = false,
        .runtime_destroyed = false,
    };
    errdefer std.heap.smp_allocator.destroy(runtime_state);

    return try registerRuntimeState(runtime, runtime_state);
}

fn checkNativeOperationResult(
    operation: c.mln_operation,
    runtime_handle: RuntimeHandle,
) status.Error!void {
    var result_status: c.mln_status = c.MLN_STATUS_NATIVE_ERROR;
    try status.checkStatus(c.mln_operation_get_status(operation, &result_status), null);
    if (result_status == c.MLN_STATUS_OK) return;

    if (runtimeState(runtime_handle)) |runtime_state| {
        if (runtime_state.diagnostic_store) |store| {
            var required: usize = 0;
            try status.checkStatus(
                c.mln_operation_copy_diagnostic(operation, null, 0, &required),
                null,
            );
            const message = try std.heap.smp_allocator.alloc(u8, required);
            defer std.heap.smp_allocator.free(message);
            var copied: usize = 0;
            try status.checkStatus(
                c.mln_operation_copy_diagnostic(
                    operation,
                    if (message.len == 0) null else message.ptr,
                    message.len,
                    &copied,
                ),
                null,
            );
            try store.set(result_status, message[0..copied]);
        }
    }
    try status.checkStatus(result_status, null);
}

pub fn waitNativeOperation(
    operation: c.mln_operation,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!void {
    var completed = false;
    try status.checkStatus(c.mln_operation_wait(operation, -1, &completed), diagnostic_store);
    if (!completed) return error.NativeError;
    var result_status: c.mln_status = c.MLN_STATUS_NATIVE_ERROR;
    try status.checkStatus(c.mln_operation_get_status(operation, &result_status), diagnostic_store);
    if (result_status != c.MLN_STATUS_OK) {
        if (diagnostic_store) |store| {
            var required: usize = 0;
            try status.checkStatus(c.mln_operation_copy_diagnostic(operation, null, 0, &required), null);
            const message = try std.heap.smp_allocator.alloc(u8, required);
            defer std.heap.smp_allocator.free(message);
            var copied: usize = 0;
            try status.checkStatus(c.mln_operation_copy_diagnostic(
                operation,
                if (message.len == 0) null else message.ptr,
                message.len,
                &copied,
            ), null);
            try store.set(result_status, message[0..copied]);
        }
        try status.checkStatus(result_status, null);
    }
}
// Events index by the batch's stride, which a later C API version may widen
// past this binding's compiled event size, so the fixed prefix is copied out
// rather than read through a pointer cast.
fn nativeEventAt(events: []const u8, event_size: usize, offset: usize) c.mln_runtime_event {
    var native_event = std.mem.zeroes(c.mln_runtime_event);
    const copied = @min(event_size, @sizeOf(c.mln_runtime_event));
    @memcpy(std.mem.asBytes(&native_event)[0..copied], events[offset..][0..copied]);
    return native_event;
}

fn eventViewAt(
    runtime_state: ?*RuntimeState,
    events: []const u8,
    event_size: usize,
    messages: []const u8,
    index: usize,
) RuntimeEventView {
    const offset = index * event_size;
    const native_event = nativeEventAt(events, event_size, offset);
    return .{
        .event_type = RuntimeEventType.fromRaw(native_event.type),
        .source_type = RuntimeEventSourceType.fromRaw(native_event.source_type),
        .source = @enumFromInt(native_event.source),
        .source_id = mapIdForNativeSource(runtime_state, native_event.source_type, native_event.source),
        .payload_type = RuntimeEventPayloadType.fromRaw(native_event.payload_type),
        .code = native_event.code,
        .message = messages[@intCast(native_event.message_offset)..][0..native_event.message_size],
        .payload = payloadFromNative(native_event, payloadWindow(events, event_size, offset)),
    };
}

// The payload window is the batch's event size less the payload's offset in the
// event, which is what a payload type this binding does not name reports.
fn payloadWindow(events: []const u8, event_size: usize, offset: usize) []const u8 {
    const payload_offset = @offsetOf(c.mln_runtime_event, "payload");
    if (event_size <= payload_offset) return "";
    return events[offset + payload_offset ..][0 .. event_size - payload_offset];
}

fn destroyOfflineRegionSnapshot(snapshot: c.mln_offline_region_snapshot) void {
    offline_region_snapshot_destroy_for_testing(snapshot);
}

fn destroyOfflineRegionList(list: c.mln_offline_region_list) void {
    offline_region_list_destroy_for_testing(list);
}

// A map handle names one map for the life of the process, so a registration
// matches on handle equality.
fn mapIdForNativeSource(
    runtime_state: ?*RuntimeState,
    source_type: u32,
    source: u64,
) ?values.MapId {
    if (source_type != c.MLN_RUNTIME_EVENT_SOURCE_MAP) return null;
    if (source == 0) return null;
    const state = runtime_state orelse return null;

    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_registry = state.registry orelse return null;
    for (runtime_registry.maps.items) |registration| {
        if (registration.native == source) return registration.id;
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

fn httpHeaderTransformTrampoline(
    user_data: ?*anyopaque,
    kind: u32,
    url: [*c]const u8,
    out_response: [*c]c.mln_http_header_transform_response,
) callconv(.c) c.mln_status {
    const transform: *HttpHeaderTransform = @ptrCast(@alignCast(user_data orelse return c.MLN_STATUS_INVALID_ARGUMENT));
    const native_response = out_response orelse return c.MLN_STATUS_INVALID_ARGUMENT;
    native_response.*.size = @sizeOf(c.mln_http_header_transform_response);
    const copied_url = std.heap.smp_allocator.dupe(u8, if (url == null) "" else std.mem.span(url)) catch return c.MLN_STATUS_NATIVE_ERROR;
    defer std.heap.smp_allocator.free(copied_url);
    const headers = transform.handler(transform.context, .{
        .kind = ResourceKind.fromRaw(kind),
        .url = copied_url,
    });
    for (headers, 0..) |header, index| {
        for (headers[0..index]) |previous| {
            if (std.ascii.eqlIgnoreCase(previous.name, header.name)) return c.MLN_STATUS_INVALID_ARGUMENT;
        }
        const native_status = c.mln_http_header_transform_response_set(
            native_response,
            header.name.ptr,
            header.name.len,
            header.value.ptr,
            header.value.len,
        );
        if (native_status != c.MLN_STATUS_OK) return native_status;
    }
    return c.MLN_STATUS_OK;
}

fn resourceProviderTrampoline(
    user_data: ?*anyopaque,
    request: ?*const c.mln_resource_request,
    native_handle: c.mln_resource_request_handle,
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
    native_handle: c.mln_resource_request_handle,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) std.mem.Allocator.Error!?ResourceRequestHandle {
    if (native_handle == 0) return null;
    const request_handle = native_handle;
    const request_state = try std.heap.smp_allocator.create(ResourceRequestState);
    request_state.* = .{ .native = request_handle, .completed = false, .diagnostic_store = diagnostic_store };
    errdefer std.heap.smp_allocator.destroy(request_state);
    return try registerResourceRequestState(request_handle, request_state);
}

fn destroyUnreleasedResourceRequestHandle(handle: ResourceRequestHandle) void {
    lockResourceRequestRegistry();
    defer unlockResourceRequestRegistry();

    const request_state = unregisterResourceRequestState(handle) orelse return;
    std.heap.smp_allocator.destroy(request_state);
}

fn registerRuntimeState(native_handle: c.mln_runtime, runtime_state: *RuntimeState) std.mem.Allocator.Error!RuntimeHandle {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    try runtime_handle_registry.put(std.heap.smp_allocator, native_handle, runtime_state);
    return @enumFromInt(native_handle);
}

fn runtimeState(handle: RuntimeHandle) ?*RuntimeState {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();
    return runtimeStateLocked(handle);
}

fn runtimeStateLocked(handle: RuntimeHandle) ?*RuntimeState {
    return runtime_handle_registry.get(@intFromEnum(handle));
}

pub fn lease(handle: *RuntimeHandle) status.BindingError!RuntimeLease {
    return runtimeLease(handle.*);
}

fn runtimeLease(handle: RuntimeHandle) status.BindingError!RuntimeLease {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle) orelse return error.ClosedHandle;
    if (runtime_state.closing) return error.ActiveBorrow;
    const runtime: c.mln_runtime = @intFromEnum(handle);
    _ = runtime_state.active_leases.fetchAdd(1, .seq_cst);
    return .{
        .state = runtime_state,
        .native = runtime,
        .diagnostic_store = runtime_state.diagnostic_store,
    };
}

const RuntimeClose = struct {
    state: *RuntimeState,
    native: c.mln_runtime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    registry: *RuntimeRegistry,
};

fn beginRuntimeClose(handle: RuntimeHandle) status.Error!?RuntimeClose {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtime_handle_registry.get(@intFromEnum(handle)) orelse return null;
    if (runtime_state.closing and !runtime_state.runtime_destroyed) return error.ActiveBorrow;
    if (runtime_state.active_leases.load(.seq_cst) != 0) return error.ActiveBorrow;
    const runtime_registry = runtime_state.registry orelse return null;
    if (!runtime_state.runtime_destroyed) {
        if (runtime_registry.maps.items.len != 0) {
            try status.setBindingDiagnostic(runtime_state.diagnostic_store, "runtime has live maps");
            return error.InvalidState;
        }
        if (runtime_registry.live_operations != 0) {
            try status.setBindingDiagnostic(runtime_state.diagnostic_store, "runtime has live operations");
            return error.InvalidState;
        }
    }
    const runtime: c.mln_runtime = @intFromEnum(handle);
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

    const entry = runtime_handle_registry.fetchRemove(@intFromEnum(handle)) orelse return null;
    const runtime_state = entry.value;
    runtime_state.registry = null;
    return runtime_state;
}

fn registerRuntimeOperation(handle: RuntimeHandle) status.BindingError!void {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle) orelse return error.ClosedHandle;
    if (runtime_state.closing) return error.ActiveBorrow;
    const runtime_registry = runtime_state.registry orelse return error.ClosedHandle;
    runtime_registry.live_operations += 1;
}

fn unregisterRuntimeOperation(handle: RuntimeHandle) void {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(handle) orelse return;
    const runtime_registry = runtime_state.registry orelse return;
    if (runtime_registry.live_operations > 0) runtime_registry.live_operations -= 1;
}

fn lockRuntimeRegistry() void {
    while (runtime_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockRuntimeRegistry() void {
    runtime_registry_lock.store(false, .seq_cst);
}

fn registerResourceRequestState(
    native_handle: c.mln_resource_request_handle,
    request_state: *ResourceRequestState,
) std.mem.Allocator.Error!ResourceRequestHandle {
    lockResourceRequestRegistry();
    defer unlockResourceRequestRegistry();

    try resource_request_registry.put(std.heap.smp_allocator, native_handle, request_state);
    return @enumFromInt(native_handle);
}

fn resourceRequestState(handle: ResourceRequestHandle) ?*ResourceRequestState {
    return resource_request_registry.get(@intFromEnum(handle));
}

fn unregisterResourceRequestState(handle: ResourceRequestHandle) ?*ResourceRequestState {
    const entry = resource_request_registry.fetchRemove(@intFromEnum(handle)) orelse return null;
    return entry.value;
}

fn lockResourceRequestRegistry() void {
    while (resource_request_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockResourceRequestRegistry() void {
    resource_request_registry_lock.store(false, .seq_cst);
}

fn registerOperationState(operation_state: *OperationState) std.mem.Allocator.Error!OperationHandle {
    lockOperationRegistry();
    defer unlockOperationRegistry();

    if (operation_free_list.items.len > 0) {
        const slot_index = operation_free_list.pop().?;
        operation_registry.items[slot_index].state = operation_state;
        operation_registry.items[slot_index].generation = nextHandleGeneration();
        return operationHandleValue(slot_index + 1, operation_registry.items[slot_index].generation);
    }

    const generation = nextHandleGeneration();
    try operation_free_list.ensureTotalCapacity(std.heap.smp_allocator, operation_registry.items.len + 1);
    try operation_registry.append(std.heap.smp_allocator, .{ .state = operation_state, .generation = generation });
    return operationHandleValue(operation_registry.items.len, generation);
}

fn operationHandleValue(index: usize, generation: u64) OperationHandle {
    return @enumFromInt((@as(u128, generation) << 64) | @as(u128, @intCast(index)));
}

fn operationIndex(handle: OperationHandle) ?usize {
    const index = @intFromEnum(handle) & std.math.maxInt(u64);
    if (index == 0 or index > std.math.maxInt(usize)) return null;
    return @intCast(index);
}

fn operationGeneration(handle: OperationHandle) u64 {
    return @intCast(@intFromEnum(handle) >> 64);
}

fn operationState(handle: OperationHandle) ?*OperationState {
    const index = operationIndex(handle) orelse return null;
    if (index > operation_registry.items.len) return null;
    const slot = operation_registry.items[index - 1];
    if (slot.generation != operationGeneration(handle)) return null;
    return slot.state;
}

fn operationLease(handle: OperationHandle) status.BindingError!OperationLease {
    lockOperationRegistry();
    defer unlockOperationRegistry();
    const operation_state = operationState(handle) orelse return error.ClosedHandle;
    if (operation_state.closing) return error.ClosedHandle;
    _ = operation_state.active_uses.fetchAdd(1, .seq_cst);
    return .{ .state = operation_state, .native = operation_state.operation_id };
}

fn beginOperationRelease(handle: OperationHandle) ?*OperationState {
    lockOperationRegistry();
    defer unlockOperationRegistry();
    const operation_state = operationState(handle) orelse return null;
    if (operation_state.closing) return null;
    operation_state.closing = true;
    return operation_state;
}

fn finishOperationRelease(handle: OperationHandle, operation_state: *OperationState) void {
    lockOperationRegistry();
    defer unlockOperationRegistry();
    const index = operationIndex(handle) orelse unreachable;
    const slot_index = index - 1;
    const slot = &operation_registry.items[slot_index];
    std.debug.assert(slot.state == operation_state);
    std.debug.assert(slot.generation == operationGeneration(handle));
    slot.state = null;
    slot.generation = nextHandleGeneration();
    operation_free_list.appendAssumeCapacity(slot_index);
}

fn lockOperationRegistry() void {
    while (operation_registry_lock.cmpxchgWeak(false, true, .seq_cst, .seq_cst) != null) {
        std.Thread.yield() catch {};
    }
}

fn unlockOperationRegistry() void {
    operation_registry_lock.store(false, .seq_cst);
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
    const requested_url = try allocator.dupe(u8, if (request.requested_url == null) "" else std.mem.span(request.requested_url));
    errdefer allocator.free(requested_url);
    const resolved_url = try allocator.dupe(u8, if (request.resolved_url == null) "" else std.mem.span(request.resolved_url));
    errdefer allocator.free(resolved_url);
    const prior_etag = if (request.prior_etag == null) null else try allocator.dupe(u8, std.mem.span(request.prior_etag));
    errdefer if (prior_etag) |value| allocator.free(value);
    const prior_data = try allocator.dupe(u8, if (request.prior_data_size == 0) "" else request.prior_data[0..request.prior_data_size]);
    errdefer allocator.free(prior_data);

    return .{
        .requested_url = requested_url,
        .resolved_url = resolved_url,
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
    allocator.free(request.requested_url);
    allocator.free(request.resolved_url);
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

// The payload union sits at a fixed offset in the event and every member is a
// value, so a payload needs no size gate and no alignment check.
fn payloadFromNative(native_event: c.mln_runtime_event, window: []const u8) RuntimeEventPayload {
    return switch (native_event.payload_type) {
        c.MLN_RUNTIME_EVENT_PAYLOAD_NONE => .none,
        c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME => blk: {
            const payload = native_event.payload.render_frame;
            break :blk .{ .render_frame = .{
                .mode = RenderMode.fromRaw(payload.mode),
                .needs_repaint = payload.needs_repaint,
                .placement_changed = payload.placement_changed,
                .stats = renderingStatsFromNative(payload.stats),
            } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => .{
            .render_map = .{ .mode = RenderMode.fromRaw(native_event.payload.render_map.mode) },
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => blk: {
            const payload = native_event.payload.tile_action;
            break :blk .{ .tile_action = .{
                .operation = TileOperation.fromRaw(payload.operation),
                .tile_id = tileIdFromNative(payload.tile_id),
            } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS => blk: {
            const payload = native_event.payload.offline_region_status;
            break :blk .{ .offline_region_status = .{
                .region_id = payload.region_id,
                .status = offlineStatusFromNative(payload.status),
            } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR => blk: {
            const payload = native_event.payload.offline_region_response_error;
            break :blk .{ .offline_region_response_error = .{
                .region_id = payload.region_id,
                .reason = ResourceErrorReason.fromRaw(payload.reason),
            } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT => blk: {
            const payload = native_event.payload.offline_region_tile_count_limit;
            break :blk .{ .offline_region_tile_count_limit = .{
                .region_id = payload.region_id,
                .limit = payload.limit,
            } };
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED => .{
            .camera_transition_finished = .{
                .transition_id = native_event.payload.camera_transition_finished.transition_id,
            },
        },
        c.MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED => .{
            .command_finished = .{
                .command_id = native_event.payload.command_finished.command_id,
                .disposition = CommandDisposition.fromRaw(native_event.payload.command_finished.disposition),
                .status = status.errorFromRawStatus(native_event.code),
                .generation = native_event.payload.command_finished.generation,
            },
        },
        else => .{ .unknown = .{
            .payload_type = native_event.payload_type,
            .bytes = window,
        } },
    };
}

fn ownedPayload(
    allocator: std.mem.Allocator,
    payload: RuntimeEventPayload,
) std.mem.Allocator.Error!RuntimeEventPayload {
    return switch (payload) {
        .unknown => |unknown| .{ .unknown = .{
            .payload_type = unknown.payload_type,
            .bytes = try allocator.dupe(u8, unknown.bytes),
        } },
        else => payload,
    };
}

fn copyOptionalBytes(allocator: std.mem.Allocator, data: ?[*]const u8, size: usize) status.Error![]const u8 {
    if (size == 0) return allocator.dupe(u8, "");
    const bytes = data orelse return error.NativeError;
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
    snapshot: c.mln_offline_region_snapshot,
) status.Error!OwnedOfflineRegion {
    var info: c.mln_offline_region_info = undefined;
    info.size = @sizeOf(c.mln_offline_region_info);
    try status.checkStatus(c.mln_offline_region_snapshot_get(snapshot, &info), null);
    return copyOfflineRegionInfo(allocator, info);
}

fn copyOfflineRegionList(allocator: std.mem.Allocator, list: c.mln_offline_region_list) status.Error!OfflineRegionList {
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
            const geometry = if (definition.geometry.size == 0)
                try allocator.dupe(u8, "")
            else blk_geometry: {
                const data: [*]const u8 = @ptrCast(definition.geometry.data orelse return error.NativeError);
                break :blk_geometry try allocator.dupe(u8, data[0..definition.geometry.size]);
            };
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

/// Encodes an event mask for the C ABI. The map handle installs its own mask, so
/// this seam is package-internal rather than part of the public surface.
pub fn eventMaskToRaw(mask: RuntimeEventMask) u64 {
    return mask.toRaw();
}

/// Decodes an event mask the C ABI reported. See `eventMaskToRaw`.
pub fn eventMaskFromRaw(raw: u64) RuntimeEventMask {
    return RuntimeEventMask.fromRaw(raw);
}

pub fn diagnosticStore(handle: *RuntimeHandle) ?*diagnostics.DiagnosticStore {
    const runtime_state = runtimeState(handle.*) orelse return null;
    return runtime_state.diagnostic_store;
}

pub fn registerMap(
    runtime: *RuntimeHandle,
    map: c.mln_map,
) status.Error!RegisteredMap {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    const runtime_state = runtimeStateLocked(runtime.*) orelse return error.ClosedHandle;
    if (runtime_state.closing) return error.ActiveBorrow;
    const runtime_registry = runtime_state.registry orelse return error.ClosedHandle;
    const id = values.MapId{ .value = runtime_registry.next_map_id };
    runtime_registry.next_map_id += 1;
    try runtime_registry.maps.append(std.heap.smp_allocator, .{ .native = map, .id = id });
    return .{ .registry = runtime_registry, .id = id };
}

pub fn unregisterMap(runtime_registry: *RuntimeRegistry, map: c.mln_map) void {
    lockRuntimeRegistry();
    defer unlockRuntimeRegistry();

    for (runtime_registry.maps.items, 0..) |registration, index| {
        if (registration.native == map) {
            _ = runtime_registry.maps.orderedRemove(index);
            return;
        }
    }
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
var notification_source_close_attempts_for_testing: usize = 0;

fn failNotificationSourceCloseOnce(source: c.mln_notification_source) callconv(.c) c.mln_status {
    notification_source_close_attempts_for_testing += 1;
    if (notification_source_close_attempts_for_testing == 1) {
        return c.MLN_STATUS_INVALID_STATE;
    }
    return c.mln_notification_source_close(source);
}

fn countingOfflineRegionSnapshotDestroy(snapshot: c.mln_offline_region_snapshot) callconv(.c) void {
    snapshot_destroy_count_for_testing += 1;
    c.mln_offline_region_snapshot_destroy(snapshot);
}

fn countingOfflineRegionListDestroy(list: c.mln_offline_region_list) callconv(.c) void {
    list_destroy_count_for_testing += 1;
    c.mln_offline_region_list_destroy(list);
}

fn markNotification(user_data: ?*anyopaque) callconv(.c) void {
    const notified: *std.atomic.Value(bool) = @ptrCast(@alignCast(user_data orelse return));
    notified.store(true, .seq_cst);
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
fn waitForOfflineOperationForTesting(_: *RuntimeHandle, operation: OperationHandle) !void {
    for (0..5000) |_| {
        if (try operation.poll()) {
            try std.testing.expectEqual(@as(i32, c.MLN_STATUS_OK), try operation.resultStatus());
            return;
        }
        try std.testing.io.sleep(.fromMilliseconds(1), .awake);
    }
    return error.EventNotObserved;
}

const OperationReleaseProbe = struct {
    operation: OperationHandle,
    entered: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    finished: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
};

fn releaseOperationForTesting(probe: *OperationReleaseProbe) void {
    probe.entered.store(true, .seq_cst);
    probe.operation.release();
    probe.finished.store(true, .seq_cst);
}

test "runtime event raw domains preserve unknown values" {
    try std.testing.expect(std.meta.eql(RuntimeEventType.fromRaw(0xfeed), RuntimeEventType{ .unknown = 0xfeed }));
    try std.testing.expect(std.meta.eql(RuntimeEventSourceType.fromRaw(0xbeef), RuntimeEventSourceType{ .unknown = 0xbeef }));
    try std.testing.expect(std.meta.eql(RuntimeEventPayloadType.fromRaw(0xace), RuntimeEventPayloadType{ .unknown = 0xace }));
    try std.testing.expect(std.meta.eql(CameraChangeMode.fromRaw(0x7ace), CameraChangeMode{ .unknown = 0x7ace }));
    try std.testing.expect(std.meta.eql(RenderMode.fromRaw(0xbad), RenderMode{ .unknown = 0xbad }));
    try std.testing.expect(std.meta.eql(TileOperation.fromRaw(0xcafe), TileOperation{ .unknown = 0xcafe }));
    try std.testing.expect(std.meta.eql(OfflineRegionDownloadState.fromRaw(0xd00d), OfflineRegionDownloadState{ .unknown = 0xd00d }));
    try std.testing.expect(std.meta.eql(ResourceErrorReason.fromRaw(0xf00d), ResourceErrorReason{ .unknown = 0xf00d }));
}

// A stride wider than this binding's compiled event size is what a later C API
// version reports, so the synthetic batch below uses one.
const testing_event_size = @sizeOf(c.mln_runtime_event) + 8;
const testing_messages = "composite-source";

fn writeTestingEvent(events: []u8, index: usize, native_event: c.mln_runtime_event) void {
    const offset = index * testing_event_size;
    @memcpy(events[offset..][0..@sizeOf(c.mln_runtime_event)], std.mem.asBytes(&native_event));
}

fn testingEventView(events: []const u8, index: usize) RuntimeEventView {
    return eventViewAt(null, events, testing_event_size, testing_messages, index);
}

test "drained events decode the payload union at the reported stride" {
    var events = [_]u8{0} ** (3 * testing_event_size);

    var tile_action = std.mem.zeroes(c.mln_runtime_event);
    tile_action.type = c.MLN_RUNTIME_EVENT_MAP_TILE_ACTION;
    tile_action.source_type = c.MLN_RUNTIME_EVENT_SOURCE_MAP;
    tile_action.payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION;
    tile_action.message_size = testing_messages.len;
    tile_action.payload.tile_action = .{
        .operation = c.MLN_TILE_OPERATION_LOAD_FROM_NETWORK,
        .tile_id = .{ .overscaled_z = 3, .wrap = -1, .canonical_z = 2, .canonical_x = 1, .canonical_y = 0 },
    };
    writeTestingEvent(&events, 0, tile_action);

    var render_frame = std.mem.zeroes(c.mln_runtime_event);
    render_frame.type = c.MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED;
    render_frame.source_type = c.MLN_RUNTIME_EVENT_SOURCE_MAP;
    render_frame.payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;
    render_frame.payload.render_frame = .{
        .mode = c.MLN_RENDER_MODE_FULL,
        .needs_repaint = true,
        .placement_changed = false,
        .stats = .{
            .encoding_time = 0.5,
            .rendering_time = 0.25,
            .frame_count = 7,
            .draw_call_count = 11,
            .total_draw_call_count = 13,
        },
    };
    writeTestingEvent(&events, 1, render_frame);

    var transition = std.mem.zeroes(c.mln_runtime_event);
    transition.type = c.MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED;
    transition.source_type = c.MLN_RUNTIME_EVENT_SOURCE_MAP;
    transition.code = c.MLN_CAMERA_CHANGE_MODE_ANIMATED;
    transition.payload_type = c.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED;
    transition.payload.camera_transition_finished = .{ .transition_id = 4242 };
    writeTestingEvent(&events, 2, transition);

    const first = testingEventView(&events, 0);
    try std.testing.expect(std.meta.eql(first.event_type, RuntimeEventType.map_tile_action));
    try std.testing.expect(std.meta.eql(first.payload.tile_action.operation, TileOperation.load_from_network));
    try std.testing.expectEqual(@as(u32, 3), first.payload.tile_action.tile_id.overscaled_z);
    try std.testing.expectEqual(@as(i32, -1), first.payload.tile_action.tile_id.wrap);
    try std.testing.expectEqualStrings("composite-source", first.message);

    const second = testingEventView(&events, 1);
    try std.testing.expect(std.meta.eql(second.payload.render_frame.mode, RenderMode.full));
    try std.testing.expect(second.payload.render_frame.needs_repaint);
    try std.testing.expectEqual(@as(i64, 7), second.payload.render_frame.stats.frame_count);
    try std.testing.expectEqualStrings("", second.message);

    // A decoder stepping by its own compiled size would read this event's
    // payload eight bytes early.
    const third = testingEventView(&events, 2);
    try std.testing.expect(std.meta.eql(
        third.event_type,
        RuntimeEventType.map_camera_transition_finished,
    ));
    try std.testing.expect(std.meta.eql(CameraChangeMode.fromRaw(third.code), CameraChangeMode.animated));
    try std.testing.expectEqual(@as(u64, 4242), third.payload.camera_transition_finished.transition_id);
}

test "an unknown drained event preserves its raw domains and payload window" {
    var events = [_]u8{0} ** testing_event_size;

    var unknown = std.mem.zeroes(c.mln_runtime_event);
    unknown.type = 0xffff;
    unknown.source_type = 0xbeef;
    unknown.source = 0x5eed_1234;
    unknown.payload_type = 0xfeed;
    writeTestingEvent(&events, 0, unknown);
    const payload_offset = @offsetOf(c.mln_runtime_event, "payload");
    const window_size = testing_event_size - payload_offset;
    @memset(events[payload_offset..], 0xa5);

    const view = testingEventView(&events, 0);
    try std.testing.expect(std.meta.eql(view.event_type, RuntimeEventType{ .unknown = 0xffff }));
    try std.testing.expect(std.meta.eql(view.source_type, RuntimeEventSourceType{ .unknown = 0xbeef }));
    // A source type this build cannot name still names its source.
    try std.testing.expectEqual(@as(u64, 0x5eed_1234), @intFromEnum(view.source));
    try std.testing.expect(std.meta.eql(view.payload_type, RuntimeEventPayloadType{ .unknown = 0xfeed }));
    try std.testing.expectEqual(@as(u32, 0xfeed), view.payload.unknown.payload_type);
    try std.testing.expectEqual(window_size, view.payload.unknown.bytes.len);

    // The view borrows the window; the copy owns it.
    var owned = try view.toOwned(std.testing.allocator);
    defer owned.deinit();
    @memset(events[payload_offset..], 0);
    try std.testing.expectEqualSlices(u8, &[_]u8{0xa5} ** window_size, owned.payload.unknown.bytes);
    try std.testing.expectEqualSlices(u8, &[_]u8{0} ** window_size, view.payload.unknown.bytes);
}

test "a map event whose map this binding does not track still names its source" {
    var events = [_]u8{0} ** testing_event_size;

    var orphaned = std.mem.zeroes(c.mln_runtime_event);
    orphaned.type = c.MLN_RUNTIME_EVENT_MAP_LOADING_FAILED;
    orphaned.source_type = c.MLN_RUNTIME_EVENT_SOURCE_MAP;
    orphaned.source = 0xfeed_face;
    writeTestingEvent(&events, 0, orphaned);

    const view = testingEventView(&events, 0);
    try std.testing.expect(std.meta.eql(view.source_type, RuntimeEventSourceType.map));
    try std.testing.expectEqual(@as(?values.MapId, null), view.source_id);
    try std.testing.expectEqual(@as(u64, 0xfeed_face), @intFromEnum(view.source));

    // The copy keeps the identity the borrow reported.
    var owned = try view.toOwned(std.testing.allocator);
    defer owned.deinit();
    try std.testing.expectEqual(view.source, owned.source);
}

test "raw event masks reject bits outside the mask enum" {
    var store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer store.deinit();

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, &store);
    defer runtime.close() catch @panic("runtime close failed");
    const native_runtime: c.mln_runtime = @intFromEnum(runtime);

    // The binding can retain an unnamed bit reported by a newer library, but
    // this native library still rejects bits it does not support.
    try std.testing.expectError(
        error.InvalidArgument,
        status.checkStatus(
            c.mln_runtime_set_event_mask(native_runtime, @as(u64, 1) << 63),
            &store,
        ),
    );
    const diagnostic = store.get().?;
    try std.testing.expectEqual(@as(?i32, c.MLN_STATUS_INVALID_ARGUMENT), diagnostic.raw_status);
    try std.testing.expect(diagnostic.message.len > 0);
}

test "event mask conversion preserves unnamed bits" {
    const unnamed = @as(u64, 1) << 63;
    const raw = unnamed | @as(u64, c.MLN_RUNTIME_EVENT_MASK_MAP_IDLE);
    const mask = eventMaskFromRaw(raw);

    try std.testing.expect(mask.map_idle);
    try std.testing.expect(mask.contains(.{ .unknown = 63 }));
    try std.testing.expectEqual(raw, eventMaskToRaw(mask));
}

test "operation release waits for active native uses" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.operationHandle(9_999_996, .region_get_status, .region_status);
    const operation_lease = try operationLease(operation);
    var probe = OperationReleaseProbe{ .operation = operation };
    const release_thread = try std.Thread.spawn(.{}, releaseOperationForTesting, .{&probe});
    while (true) {
        _ = operation.id() catch |err| {
            try std.testing.expectEqual(error.ClosedHandle, err);
            break;
        };
        std.Thread.yield() catch {};
    }
    try std.testing.expect(!probe.finished.load(.seq_cst));

    operation_lease.release();
    release_thread.join();
    try std.testing.expect(probe.finished.load(.seq_cst));
    try std.testing.expectError(error.ClosedHandle, operation.id());
}

test "operation take-result failures preserve handle state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.operationHandle(9_999_999, .region_get_status, .region_status);
    try std.testing.expectError(error.InvalidArgument, runtime.takeOfflineRegionStatus(operation));
    try std.testing.expectEqual(@as(OperationId, @enumFromInt(9_999_999)), try operation.id());

    operation.release();
}

test "operation discard failures preserve handle state" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.operationHandle(9_999_997, .region_get_status, .region_status);
    try std.testing.expectError(error.InvalidArgument, operation.discard());
    try std.testing.expectEqual(@as(OperationId, @enumFromInt(9_999_997)), try operation.id());

    operation.release();
}

test "runtime close rejects live operations" {
    var diagnostic_store = diagnostics.DiagnosticStore.init(std.testing.allocator);
    defer diagnostic_store.deinit();

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, &diagnostic_store);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.operationHandle(9_999_998, .region_get_status, .region_status);
    try std.testing.expectError(error.InvalidState, runtime.close());
    try std.testing.expectEqualStrings("runtime has live operations", diagnostic_store.get().?.message);

    operation.release();
    try runtime.close();
    runtime_open = false;
}

test "runtime notification callback exposes ready operation endpoint" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    defer runtime.close() catch @panic("runtime close failed");
    var notified = std.atomic.Value(bool).init(false);
    try runtime.setNotificationCallback(markNotification, &notified);

    const operation = try runtime.barrierStart();
    defer operation.release();
    try std.testing.expect(try operation.wait(-1));
    for (0..1000) |_| {
        if (notified.load(.seq_cst)) break;
        std.Thread.yield() catch {};
    }
    try std.testing.expect(notified.load(.seq_cst));

    var ready = try runtime.drainReady(std.testing.allocator);
    defer ready.deinit();
    const operation_id = try operation.id();
    var found = false;
    for (ready.endpoints) |endpoint| {
        if (endpoint.kind == .operation and endpoint.id.operation == operation_id) {
            found = true;
            break;
        }
    }
    try std.testing.expect(found);
    try runtime.clearNotificationCallback();
}

test "runtime close preserves notification source ownership for retry" {
    var runtime = try RuntimeHandle.create(std.testing.allocator, .{}, null);
    var runtime_open = true;
    defer if (runtime_open) runtime.close() catch @panic("runtime close failed");

    notification_source_close_attempts_for_testing = 0;
    notification_source_close_for_testing = failNotificationSourceCloseOnce;
    defer notification_source_close_for_testing = c.mln_notification_source_close;

    try std.testing.expectError(error.InvalidState, runtime.close());
    try runtime.close();
    runtime_open = false;
    try std.testing.expectEqual(@as(usize, 2), notification_source_close_attempts_for_testing);
}

test "offline region snapshot destroy runs when copied output allocation fails" {
    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPathForTesting(std.testing.allocator, tmp.sub_path[0..], "snapshot-copy-failure-cache.db");
    defer std.testing.allocator.free(cache_path);

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{ .cache_path = cache_path }, null);
    defer runtime.close() catch @panic("runtime close failed");

    const operation = try runtime.startCreateOfflineRegion(std.testing.allocator, offlineTileDefinitionForTesting(), &.{});
    defer operation.release();
    try waitForOfflineOperationForTesting(&runtime, operation);
    var operation_diagnostic = try operation.copyDiagnostic(std.testing.allocator);
    defer operation_diagnostic.deinit();
    try std.testing.expectEqualStrings("", operation_diagnostic.value);

    var failing_allocator = std.testing.FailingAllocator.init(std.testing.allocator, .{ .fail_index = 0 });
    snapshot_destroy_count_for_testing = 0;
    offline_region_snapshot_destroy_for_testing = countingOfflineRegionSnapshotDestroy;
    defer offline_region_snapshot_destroy_for_testing = c.mln_offline_region_snapshot_destroy;

    try std.testing.expectError(error.OutOfMemory, runtime.takeOfflineRegion(failing_allocator.allocator(), operation));
    try std.testing.expectEqual(@as(usize, 1), snapshot_destroy_count_for_testing);
    _ = try operation.id();
}

test "offline region list destroy runs when copied output allocation fails" {
    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();
    const cache_path = try tempPathForTesting(std.testing.allocator, tmp.sub_path[0..], "list-copy-failure-cache.db");
    defer std.testing.allocator.free(cache_path);

    var runtime = try RuntimeHandle.create(std.testing.allocator, .{ .cache_path = cache_path }, null);
    defer runtime.close() catch @panic("runtime close failed");

    const create_operation = try runtime.startCreateOfflineRegion(std.testing.allocator, offlineTileDefinitionForTesting(), &.{});
    defer create_operation.release();
    try waitForOfflineOperationForTesting(&runtime, create_operation);
    var region = try runtime.takeOfflineRegion(std.testing.allocator, create_operation);
    defer region.deinit();

    const list_operation = try runtime.startListOfflineRegions();
    defer list_operation.release();
    try waitForOfflineOperationForTesting(&runtime, list_operation);

    var failing_allocator = std.testing.FailingAllocator.init(std.testing.allocator, .{ .fail_index = 0 });
    list_destroy_count_for_testing = 0;
    offline_region_list_destroy_for_testing = countingOfflineRegionListDestroy;
    defer offline_region_list_destroy_for_testing = c.mln_offline_region_list_destroy;

    try std.testing.expectError(error.OutOfMemory, runtime.takeOfflineRegionList(failing_allocator.allocator(), list_operation));
    try std.testing.expectEqual(@as(usize, 1), list_destroy_count_for_testing);
    _ = try list_operation.id();
}
