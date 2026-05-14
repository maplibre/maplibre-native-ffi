const std = @import("std");

pub const style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-test",
    \\  "sources": {
    \\    "point": {
    \\      "type": "geojson",
    \\      "data": {
    \\        "type": "FeatureCollection",
    \\        "features": [
    \\          {"type":"Feature","id":"feature-1","geometry":{"type":"Point","coordinates":[-122.4194,37.7749]},"properties":{"visible":true,"kind":"capital"}}
    \\        ]
    \\      }
    \\    }
    \\  },
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}},
    \\    {"id":"point-circle","type":"circle","source":"point","paint":{"circle-color":"#f97316","circle-radius":12}}
    \\  ]
    \\}
;

pub fn typeNameContains(comptime T: type, comptime needle: []const u8) bool {
    if (std.mem.indexOf(u8, @typeName(T), needle) != null) return true;
    const info = @typeInfo(T);
    if (info != .@"struct") return false;
    inline for (info.@"struct".fields) |field| {
        if (std.mem.indexOf(u8, @typeName(field.type), needle) != null) return true;
    }
    return false;
}
