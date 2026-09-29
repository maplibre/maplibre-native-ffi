const testing = @import("std").testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("support.zig");

test "diagnostics capture public lifecycle failures and keep copied messages" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try support.createRuntimeWithDiagnostics(.{}, &diagnostics);
    var map = try support.createMap(&runtime, .{});

    try testing.expectError(error.InvalidState, support.closeRuntime(&runtime));
    const first = diagnostics.get().?;
    try testing.expectEqual(@as(?i32, -2), first.raw_status);
    try testing.expect(first.message.len > 0);
    const copied = try testing.allocator.dupe(u8, first.message);
    defer testing.allocator.free(copied);

    // The map inherits the runtime's store, and its successful close leaves
    // the copied failure in place.
    try support.closeMap(&map);
    try testing.expectEqualStrings(copied, diagnostics.get().?.message);
    try support.closeRuntime(&runtime);
}
