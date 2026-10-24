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

test "generated keyword parameters keep their order" {
    const entry = try api.keywordCombine(5, 2, 7, 11, null);
    try std.testing.expectEqual(api.KeywordEntry{ .type = 3, .@"defer" = 7, .raw = 11 }, entry);
}

test "generated calls report native failures through the diagnostic" {
    var right: [9]api.ProbePoint = @splat(point);
    var diagnostic: api.Diagnostic = .{};
    try std.testing.expectError(error.InvalidArgument, api.probeRoundtrip(std.testing.allocator, .{ .right = &right }, &diagnostic));
    try std.testing.expectEqual(.invalid_argument, diagnostic.status);
    try std.testing.expectEqualStrings("right holds more than 8 points", diagnostic.message());
}

test "generated record defaults equal the native default" {
    try api.probeSettingsCheck(api.probeSettingsDefault(), null);
}

test "generated default copies keep values and leave registrations unset" {
    var hooks = try api.probeHooksDefault(std.testing.allocator);
    defer hooks.deinit();
    try std.testing.expectEqual(@as(u32, 4), hooks.value.limit);
    try std.testing.expect(hooks.value.signal.callback == null);
}

test "generated absence returns null and other failures still report" {
    try std.testing.expect(try api.probeTakeParcel(null) == null);
    var parcel = (try api.probeTakeParcel(null)).?;
    try std.testing.expectEqual(@as(u64, 42), parcel.raw);
    parcel.deinit();
    var diagnostic: api.Diagnostic = .{};
    try std.testing.expectError(error.InvalidArgument, api.probeTakeParcel(&diagnostic));
    try std.testing.expectEqualStrings("exhausted", diagnostic.message());
    try std.testing.expect(try api.probeReadLevel(null) == null);
    try std.testing.expectEqual(@as(?f64, 0.5), try api.probeReadLevel(null));
}

const callbacks = @import("callback.zig");
var released_query: u64 = 0;

export fn mln_query_release(query: u64) void {
    released_query = query;
}

const QueryProbe = struct {
    calls: usize = 0,
    fail: bool = false,
    saved: ?api.Query = null,

    fn receive(context: ?*anyopaque, _: []const u8, query: api.Query) api.Error!void {
        const self: *QueryProbe = @ptrCast(@alignCast(context.?));
        self.calls += 1;
        if (self.fail) return error.NativeError;
        self.saved = query;
    }
};

test "generated owned callback arguments are released on conversion and host errors" {
    var roots: callbacks.Roots = .{};
    defer roots.deinit();
    var probe = QueryProbe{};
    const handler = api.QueryHandler{ .callback = QueryProbe.receive, .context = &probe };
    const raw = try handler.toNative(&roots);
    released_query = 0;
    raw.callback.?(raw.context, null, 41);
    try std.testing.expectEqual(@as(u64, 41), released_query);
    try std.testing.expectEqual(@as(usize, 0), probe.calls);

    probe.fail = true;
    raw.callback.?(raw.context, "query", 42);
    try std.testing.expectEqual(@as(u64, 42), released_query);
    try std.testing.expectEqual(@as(usize, 1), probe.calls);

    probe.fail = false;
    raw.callback.?(raw.context, "query", 43);
    try std.testing.expectEqual(@as(usize, 2), probe.calls);
    try std.testing.expectEqual(@as(u64, 42), released_query);
    var saved = probe.saved.?;
    api.queryRelease(&saved);
    try std.testing.expectEqual(@as(u64, 43), released_query);
    // The release closed the value, so a second one does not reach native.
    try std.testing.expect(saved.isClosed());
    released_query = 0;
    api.queryRelease(&saved);
    try std.testing.expectEqual(@as(u64, 0), released_query);
}

test "generated caller-sized output arrays" {
    const allocator = std.testing.allocator;
    const points = [_]api.ProbeBatchPoint{ .{ .x = 2, .y = 5 }, .{ .x = 7, .y = 11 }, .{ .x = -3, .y = 17 } };
    var result = try api.probeBatchPoints(allocator, &points, null);
    defer result.deinit();
    const expected = [_]api.ProbeBatchPoint{ .{ .x = 5, .y = 3 }, .{ .x = 11, .y = 8 }, .{ .x = 17, .y = -2 } };
    try std.testing.expectEqualSlices(api.ProbeBatchPoint, &expected, result.value);
    var scores = try api.probeBatchScores(allocator, &points, null);
    defer scores.deinit();
    try std.testing.expectEqualSlices(f64, &.{ 7, 18, 14 }, scores.value);
    var empty = try api.probeBatchPoints(allocator, &.{}, null);
    defer empty.deinit();
    try std.testing.expectEqual(@as(usize, 0), empty.value.len);
}

test "generated caller-sized text" {
    const allocator = std.testing.allocator;
    var empty = try api.probeZerosJson(allocator, 0, null);
    defer empty.deinit();
    try std.testing.expectEqualStrings("[]", empty.value);
    var short = try api.probeZerosJson(allocator, 3, null);
    defer short.deinit();
    try std.testing.expectEqualStrings("[0,0,0]", short.value);
    // Text longer than the call's first storage, which it fetches again.
    var long = try api.probeZerosJson(allocator, 300, null);
    defer long.deinit();
    try std.testing.expectEqual(@as(usize, 601), long.value.len);
    try std.testing.expect(std.mem.startsWith(u8, long.value, "[0,0,"));
    try std.testing.expect(std.mem.endsWith(u8, long.value, ",0]"));
}
