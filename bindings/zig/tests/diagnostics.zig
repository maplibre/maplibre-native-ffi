const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

// A command that native accepts and then fails completes with a failed
// disposition, which `wait` returns rather than raising. The wait's diagnostic
// then carries the failure, replacing whatever an earlier call left in it.
test "a failed command arrives as data and fills the wait's diagnostic" {
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidArgument, maplibre.projectedMetersForLatLng(.{ .latitude = std.math.inf(f64), .longitude = 0.0 }, &diagnostic));
    const stale = try testing.allocator.dupe(u8, diagnostic.message());
    defer testing.allocator.free(stale);

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    var future = try maplibre.mapSetStyleJson(fixture.map, "{", null);
    defer future.deinit();
    const completion = try future.wait(&diagnostic);
    try testing.expectEqual(maplibre.CommandDisposition.failed, completion.disposition);
    try testing.expectError(error.NativeError, completion.statusError());
    try testing.expectEqual(completion.raw_status, @backingInt(diagnostic.status.?));
    try testing.expect(diagnostic.message().len != 0);
    try testing.expect(!std.mem.eql(u8, stale, diagnostic.message()));
}
