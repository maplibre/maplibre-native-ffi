const testing = @import("std").testing;

const maplibre = @import("maplibre_native");

test "diagnostics capture public lifecycle failures and keep copied messages" {
    var diagnostics = maplibre.DiagnosticStore.init(testing.allocator);
    defer diagnostics.deinit();

    var runtime = try maplibre.RuntimeHandle.init(&diagnostics);
    var map = try maplibre.MapHandle.create(&runtime, .{});

    try testing.expectError(error.InvalidState, runtime.close());
    const first = diagnostics.get().?;
    try testing.expectEqual(@as(?i32, -2), first.raw_status);
    try testing.expect(first.message.len > 0);
    const copied = try testing.allocator.dupe(u8, first.message);
    defer testing.allocator.free(copied);

    try map.close();
    try testing.expectEqualStrings(copied, diagnostics.get().?.message);
    try runtime.close();
}
