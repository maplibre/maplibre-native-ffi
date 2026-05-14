const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

fn typeNameContains(comptime T: type, comptime needle: []const u8) bool {
    if (std.mem.indexOf(u8, @typeName(T), needle) != null) return true;
    const info = @typeInfo(T);
    if (info != .@"struct") return false;
    inline for (info.@"struct".fields) |field| {
        if (std.mem.indexOf(u8, @typeName(field.type), needle) != null) return true;
    }
    return false;
}

const style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-test",
    \\  "sources": {},
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
    \\  ]
    \\}
;

fn runRuntimeOnThread(runtime: maplibre.RuntimeHandle, out_error: *?anyerror) void {
    runtime.runOnce() catch |err| {
        out_error.* = err;
        return;
    };
    out_error.* = null;
}

test "package root hides raw C declarations" {
    try testing.expect(!@hasDecl(maplibre, "c"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime_create"));
    try testing.expect(!typeNameContains(maplibre.RuntimeHandle, "mln_"));
    try testing.expect(!typeNameContains(maplibre.MapHandle, "mln_"));
    try testing.expect(!typeNameContains(maplibre.MapProjectionHandle, "mln_"));
    try testing.expect(!typeNameContains(maplibre.RuntimeHandle, "anyopaque"));
    try testing.expect(!typeNameContains(maplibre.MapHandle, "anyopaque"));
    try testing.expect(!typeNameContains(maplibre.MapProjectionHandle, "anyopaque"));
}

test "package links the native C library" {
    try testing.expectEqual(@as(u32, 0), maplibre.cAbiVersion());
}

test "package validates the supported C ABI version" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    try maplibre.validateAbiVersion(&diagnostics);
    try testing.expect(diagnostics.get() == null);
}

test "runtime and map vertical slice" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");

    try runtime.runOnce();
    try testing.expectEqual(@as(?maplibre.RuntimeEvent, null), try runtime.pollEvent());

    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.setStyleJson(testing.allocator, style_json);
    try runtime.runOnce();

    const projection = try maplibre.MapProjectionHandle.create(map);
    try projection.close();
    try projection.close();
}

test "successful close is idempotent and closed handles fail before C calls" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    const map = try maplibre.MapHandle.create(runtime, .{});

    try map.close();
    try map.close();
    try testing.expectError(error.ClosedHandle, map.requestRepaint());

    try runtime.close();
    try runtime.close();
    try testing.expectError(error.ClosedHandle, runtime.runOnce());
}

test "failed close remains retryable" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    const map = try maplibre.MapHandle.create(runtime, .{});

    try testing.expectError(error.InvalidState, runtime.close());
    try map.close();
    try runtime.close();
}

test "wrong-thread failures propagate diagnostics" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");

    var thread_error: ?anyerror = null;
    const thread = try std.Thread.spawn(.{}, runRuntimeOnThread, .{ runtime, &thread_error });
    thread.join();

    try testing.expectEqual(error.WrongThread, thread_error.?);
    try testing.expect(diagnostics.get().?.message.len > 0);
}

test "strings reject embedded NUL before C calls" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    const runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    defer runtime.close() catch @panic("runtime close failed");
    const map = try maplibre.MapHandle.create(runtime, .{});
    defer map.close() catch @panic("map close failed");

    try map.close();
    try testing.expectError(error.ClosedHandle, map.setStyleJson(testing.allocator, "{\x00}"));
}
