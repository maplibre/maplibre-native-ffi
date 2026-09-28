const testing = @import("std").testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

test "diagnostics capture public lifecycle failures and keep copied messages" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try support.createRuntime(.{});
    var map = try support.createMap(&runtime, .{});

    try testing.expectError(error.InvalidState, support.closeRuntime(&runtime));
    var first = try maplibre.threadLastErrorMessage(testing.allocator);
    defer first.deinit();
    try testing.expect(first.value.len > 0);
    const copied = try testing.allocator.dupe(u8, first.value);
    defer testing.allocator.free(copied);

    try support.closeMap(&map);
    try testing.expectEqualStrings(copied, first.value);
    try support.closeRuntime(&runtime);
}
