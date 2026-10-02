const std = @import("std");
const api = @import("maplibre_native_ffi.zig");

const point = api.ProbePoint{ .type = 9.5, .gain = 3.25 };

test "generated values round trip through native" {
    const allocator = std.testing.allocator;
    const left = [_]api.ProbePoint{point};
    const right = [_]api.ProbePoint{ point, point };
    var output = try api.probeRoundtrip(allocator, .{ .title = "", .point = point, .left = &left, .right = &right }, null);
    defer output.deinit();
    try std.testing.expectEqualStrings("", output.value.title.?);
    try std.testing.expectEqual(point, output.value.point.?);
    try std.testing.expectEqualSlices(api.ProbePoint, &left, output.value.left.?);
    try std.testing.expectEqualSlices(api.ProbePoint, &right, output.value.right);

    var empty = try api.probeRoundtrip(allocator, .{ .left = &.{} }, null);
    defer empty.deinit();
    try std.testing.expectEqual(@as(usize, 0), empty.value.left.?.len);

    var absent = try api.probeRoundtrip(allocator, .{}, null);
    defer absent.deinit();
    try std.testing.expect(absent.value.title == null and absent.value.point == null and absent.value.left == null);
    try std.testing.expectEqual(@as(usize, 0), absent.value.right.len);
}

test "generated nullable text keeps absence and emptiness" {
    for ([_]?[]const u8{ null, "", "text" }) |text| {
        var result = try api.probeNullableText(std.testing.allocator, text, null);
        defer result.deinit();
        try std.testing.expectEqual(text == null, result.value.text == null);
        if (text) |expected| try std.testing.expectEqualStrings(expected, result.value.text.?);
    }
}

test "generated keyword parameters keep their order" {
    const entry = try api.keywordCombine(5, 2, 7, 11, null);
    try std.testing.expectEqual(api.KeywordEntry{ .type = 3, .@"defer" = 7, .raw = 11 }, entry);
}

test "generated calls report native failures through the diagnostic" {
    var right: [9]api.ProbePoint = @splat(point);
    var diagnostic: api.Diagnostic = .{};
    try std.testing.expectError(error.InvalidArgument, api.probeRoundtrip(std.testing.allocator, .{ .right = &right }, &diagnostic));
    try std.testing.expectEqual(@as(?i32, -1), diagnostic.raw_status);
    try std.testing.expectEqualStrings("right holds more than 8 points", diagnostic.message());
}
