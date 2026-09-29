const testing = @import("std").testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

test "diagnostics capture native lifecycle failures" {
    var runtime = try support.createRuntime(.{});
    var map = try support.createMap(&runtime, .{});

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.runtimeRelease(support.handle(runtime), &diagnostic));
    try testing.expectEqual(@as(?i32, -2), diagnostic.raw_status);
    try testing.expect(diagnostic.message().len > 0);

    try support.closeMap(&map);
    try support.closeRuntime(&runtime);
}

test "diagnostics describe failures that the binding detects" {
    var runtime = try support.createRuntime(.{});
    const released = runtime;
    try support.closeRuntime(&runtime);

    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidState, maplibre.runtimeBarrier(released, &diagnostic));
    try testing.expectEqual(@as(?i32, null), diagnostic.raw_status);
    try testing.expect(diagnostic.message().len > 0);
}
