// Raw C ABI coverage: core runtime/map/style/event/diagnostic tests for unsafe inputs, stale handles, and thread-local diagnostics hidden by the Zig binding.

const std = @import("std");
const testing = std.testing;
const support = @import("support.zig");
const c = support.c;

const stale_style_json = "{}";

fn emptyEvent() c.mln_runtime_event {
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

test "runtime rejects invalid arguments" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_create(null, null));
    try testing.expect(std.mem.len(c.mln_thread_last_error_message()) > 0);

    var small_options = c.mln_runtime_options_default();
    small_options.size = @sizeOf(c.mln_runtime_options) - 1;
    var runtime: ?*c.mln_runtime = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_create(&small_options, &runtime));
    try testing.expectEqual(@as(?*c.mln_runtime, null), runtime);

    runtime = @ptrFromInt(1);
    var options = c.mln_runtime_options_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_create(&options, &runtime));

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_destroy(null));
    try testing.expect(std.mem.len(c.mln_thread_last_error_message()) > 0);
}

test "runtime rejects unknown flags" {
    var options = c.mln_runtime_options_default();
    options.flags = 1 << 31;

    var runtime: ?*c.mln_runtime = null;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_create(&options, &runtime));
    try testing.expectEqual(@as(?*c.mln_runtime, null), runtime);
}

test "runtime rejects stale handles" {
    const runtime = try support.createRuntime();
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_destroy(runtime));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_destroy(runtime));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_run_once(runtime));
}

test "runtime run once rejects null runtime" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_run_once(null));
}

test "runtime event polling rejects null runtime" {
    var event = emptyEvent();
    var has_event = false;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_poll_event(null, &event, &has_event));
}

test "map create rejects invalid arguments" {
    var map: ?*c.mln_map = null;
    var options = c.mln_map_options_default();

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_create(null, &options, &map));
    try testing.expectEqual(@as(?*c.mln_map, null), map);

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_create(runtime, &options, null));

    map = @ptrFromInt(1);
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_create(runtime, &options, &map));

    map = null;
    var small_options = c.mln_map_options_default();
    small_options.size = @sizeOf(c.mln_map_options) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_create(runtime, &small_options, &map));

    var invalid_options = c.mln_map_options_default();
    invalid_options.map_mode = 999;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_create(runtime, &invalid_options, &map));
}

test "map lifecycle rejects invalid state and stale handles" {
    const runtime = try support.createRuntime();

    const map = try support.createMap(runtime);

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_destroy(map));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_destroy(map));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_style_json(map, stale_style_json));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_request_repaint(map));
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_request_still_image(map));

    var camera = c.mln_camera_options_default();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_get_camera(map, &camera));

    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_destroy(runtime));
}

test "style functions reject null inputs" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    const map = try support.createMap(runtime);
    defer support.destroyMap(map);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_style_json(map, null));
    try testing.expect(std.mem.len(c.mln_thread_last_error_message()) > 0);

    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_map_set_style_url(map, null));
    try testing.expect(std.mem.len(c.mln_thread_last_error_message()) > 0);
}

test "runtime event polling rejects invalid outputs" {
    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);

    var has_event = false;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_poll_event(runtime, null, &has_event));

    var event = emptyEvent();
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_poll_event(runtime, &event, null));

    event.size = @sizeOf(c.mln_runtime_event) - 1;
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_poll_event(runtime, &event, &has_event));
}

fn failOnThread(out_len: *usize) void {
    std.debug.assert(c.mln_runtime_destroy(null) == c.MLN_STATUS_INVALID_ARGUMENT);
    out_len.* = std.mem.len(c.mln_thread_last_error_message());
}

test "failing status sets and successful status clears diagnostics" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_destroy(null));
    try testing.expect(std.mem.len(c.mln_thread_last_error_message()) > 0);

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    try testing.expectEqual(@as(usize, 0), std.mem.len(c.mln_thread_last_error_message()));
}

test "diagnostics are thread local" {
    try testing.expectEqual(c.MLN_STATUS_INVALID_ARGUMENT, c.mln_runtime_destroy(null));
    const main_message = std.mem.span(c.mln_thread_last_error_message());
    try testing.expect(main_message.len > 0);

    var worker_len: usize = 0;
    const thread = try std.Thread.spawn(.{}, failOnThread, .{&worker_len});
    thread.join();
    try testing.expect(worker_len > 0);

    try testing.expectEqualStrings(main_message, std.mem.span(c.mln_thread_last_error_message()));

    const runtime = try support.createRuntime();
    defer support.destroyRuntime(runtime);
    try testing.expectEqual(@as(usize, 0), std.mem.len(c.mln_thread_last_error_message()));
}
