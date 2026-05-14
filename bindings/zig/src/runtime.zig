const std = @import("std");

const c = @import("c.zig").raw;
const diagnostics = @import("diagnostics.zig");
const status = @import("status.zig");

const RuntimeStateHandle = opaque {};
const RuntimeState = struct {
    native: ?*c.mln_runtime,
    diagnostic_store: ?*diagnostics.DiagnosticStore,
};

pub const RuntimeOptions = struct {
    asset_path: ?[]const u8 = null,
    cache_path: ?[]const u8 = null,
    maximum_cache_size: ?u64 = null,
};

pub const RuntimeEvent = struct {
    event_type: RuntimeEventType,
    source_type: RuntimeEventSourceType,
    payload_type: RuntimeEventPayloadType,
    code: i32,
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
        return .{
            .event_type = RuntimeEventType.fromRaw(native_event.type),
            .source_type = RuntimeEventSourceType.fromRaw(native_event.source_type),
            .payload_type = RuntimeEventPayloadType.fromRaw(native_event.payload_type),
            .code = native_event.code,
        };
    }

    pub fn close(self: RuntimeHandle) status.Error!void {
        const runtime_state = state(self);
        const runtime = runtime_state.native orelse return;
        try status.checkStatus(c.mln_runtime_destroy(runtime), runtime_state.diagnostic_store);
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
    runtime_state.* = .{ .native = runtime.?, .diagnostic_store = diagnostic_store };
    return .{ .state = @ptrCast(runtime_state) };
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
