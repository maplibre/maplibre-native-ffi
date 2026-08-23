const testing = @import("std").testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

test "diagnostics capture public lifecycle failures and keep copied messages" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.create(testing.allocator, .{}, &diagnostics);
    var map_future = try maplibre.MapHandle.create(&runtime, .{});
    defer map_future.deinit();
    var map = try map_future.wait(null);

    try testing.expectError(error.InvalidState, support.closeRuntime(&runtime));
    const first = diagnostics.get().?;
    try testing.expectEqual(@as(?i32, null), first.raw_status);
    try testing.expectEqualStrings("runtime has live maps", first.message);
    const copied = try testing.allocator.dupe(u8, first.message);
    defer testing.allocator.free(copied);

    try map.close();
    try testing.expectEqualStrings(copied, diagnostics.get().?.message);
    try support.closeRuntime(&runtime);
}
