const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native_ffi");
const support = @import("fixture.zig");

// A command that native accepts and then fails returns its status's error from
// `wait`. The wait's diagnostic then carries the failure and the generation the
// command published, replacing whatever an earlier call left in it.
test "a failed command returns its error and fills the wait's diagnostic" {
    var diagnostic: maplibre.Diagnostic = .{};
    try testing.expectError(error.InvalidArgument, maplibre.projectedMetersForLatLng(.{ .latitude = std.math.inf(f64), .longitude = 0.0 }, &diagnostic));
    const stale = try testing.allocator.dupe(u8, diagnostic.message());
    defer testing.allocator.free(stale);

    const fixture = try support.Fixture.create(.{});
    defer fixture.destroy();
    const committed = try support.resolve(try fixture.map.setStyleJson(support.style_json, null));
    var future = try fixture.map.removeStyleSource("missing", null);
    defer future.deinit();
    try testing.expectError(error.NotFound, future.wait(&diagnostic));
    try testing.expectEqual(.not_found, diagnostic.status);
    try testing.expect(diagnostic.generation > committed.generation);
    try testing.expect(diagnostic.message().len != 0);
    try testing.expect(!std.mem.eql(u8, stale, diagnostic.message()));
}
