const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");
const values = @import("values.zig");

pub const RuntimeStateHandle = opaque {};
const MapRegistration = struct {
    native: *c.mln_map,
    id: values.MapId,
};
const RuntimeState = struct {
    native: ?*c.mln_runtime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
    maps: std.ArrayList(MapRegistration),
    next_map_id: u64,
};

pub const RuntimeOptions = struct {
    asset_path: ?[]const u8 = null,
    cache_path: ?[]const u8 = null,
    maximum_cache_size: ?u64 = null,
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
    state: *RuntimeStateHandle,

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

    pub fn runOnce(self: RuntimeHandle) status.Error!void {
        try status.checkStatus(c.mln_runtime_run_once(try native(self)), state(self).diagnostic_store);
    }

    pub fn pollEvent(self: RuntimeHandle) status.Error!?RuntimeEvent {
        var native_event = emptyNativeEvent();
        var has_event = false;
        try status.checkStatus(
            c.mln_runtime_poll_event(try native(self), &native_event, &has_event),
            state(self).diagnostic_store,
        );
        if (!has_event) return null;
        return runtimeEventFromNative(self, native_event);
    }

    pub fn pollEventOwned(self: RuntimeHandle, allocator: std.mem.Allocator) status.Error!?OwnedRuntimeEvent {
        var native_event = emptyNativeEvent();
        var has_event = false;
        try status.checkStatus(
            c.mln_runtime_poll_event(try native(self), &native_event, &has_event),
            state(self).diagnostic_store,
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

    pub fn close(self: RuntimeHandle) status.Error!void {
        const runtime_state = state(self);
        const runtime = runtime_state.native orelse return;
        try status.checkStatus(c.mln_runtime_destroy(runtime), runtime_state.diagnostic_store);
        runtime_state.maps.deinit(std.heap.smp_allocator);
        runtime_state.maps = .empty;
        runtime_state.native = null;
    }
};

fn createNative(
    native_options: *c.mln_runtime_options,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
) status.Error!RuntimeHandle {
    var runtime: ?*c.mln_runtime = null;
    try status.checkStatus(c.mln_runtime_create(native_options, &runtime), diagnostic_store);
    errdefer {
        if (runtime) |handle| _ = c.mln_runtime_destroy(handle);
    }

    const runtime_state = try std.heap.smp_allocator.create(RuntimeState);
    runtime_state.* = .{
        .native = runtime.?,
        .diagnostic_store = diagnostic_store,
        .maps = .empty,
        .next_map_id = 1,
    };
    return .{ .state = @ptrCast(runtime_state) };
}

fn runtimeEventFromNative(handle: RuntimeHandle, native_event: c.mln_runtime_event) RuntimeEvent {
    return .{
        .event_type = RuntimeEventType.fromRaw(native_event.type),
        .source_type = RuntimeEventSourceType.fromRaw(native_event.source_type),
        .source_id = mapIdForNativeSource(handle, native_event.source_type, native_event.source),
        .payload_type = RuntimeEventPayloadType.fromRaw(native_event.payload_type),
        .code = native_event.code,
    };
}

fn mapIdForNativeSource(handle: RuntimeHandle, source_type: u32, source: ?*anyopaque) ?values.MapId {
    if (source_type != c.MLN_RUNTIME_EVENT_SOURCE_MAP) return null;
    const source_ptr = source orelse return null;
    for (state(handle).maps.items) |registration| {
        if (@intFromPtr(registration.native) == @intFromPtr(source_ptr)) return registration.id;
    }
    return null;
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

fn state(handle: RuntimeHandle) *RuntimeState {
    return @ptrCast(@alignCast(handle.state));
}

pub fn native(handle: RuntimeHandle) status.BindingError!*c.mln_runtime {
    return state(handle).native orelse error.ClosedHandle;
}

pub fn diagnosticStore(handle: RuntimeHandle) ?*diagnostics.DiagnosticStore {
    return state(handle).diagnostic_store;
}

pub fn registerMap(handle: RuntimeHandle, map: *c.mln_map) std.mem.Allocator.Error!values.MapId {
    const runtime_state = state(handle);
    const id = values.MapId{ .value = runtime_state.next_map_id };
    runtime_state.next_map_id += 1;
    try runtime_state.maps.append(std.heap.smp_allocator, .{ .native = map, .id = id });
    return id;
}

pub fn unregisterMap(handle: RuntimeHandle, map: *c.mln_map) void {
    const runtime_state = state(handle);
    for (runtime_state.maps.items, 0..) |registration, index| {
        if (@intFromPtr(registration.native) == @intFromPtr(map)) {
            _ = runtime_state.maps.orderedRemove(index);
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
    var misaligned_storage: [@sizeOf(AlignedPayload) + 1]u8 align(1) = undefined;
    try std.testing.expectError(
        error.NativeError,
        payloadAs(AlignedPayload, @ptrCast(&misaligned_storage[1]), @sizeOf(AlignedPayload)),
    );
}
