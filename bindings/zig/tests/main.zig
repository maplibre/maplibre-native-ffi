const std = @import("std");
const testing = std.testing;

const maplibre = @import("maplibre_native");

test "package root hides raw C declarations" {
    try testing.expect(!@hasDecl(maplibre, "c"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime"));
    try testing.expect(!@hasDecl(maplibre, "mln_runtime_create"));
}

test "package links the native C library" {
    try testing.expectEqual(@as(u32, 0), maplibre.cAbiVersion());
}
