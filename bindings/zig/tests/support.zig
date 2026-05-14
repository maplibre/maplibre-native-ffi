const std = @import("std");

pub const style_json =
    \\{
    \\  "version": 8,
    \\  "name": "zig-binding-test",
    \\  "sources": {},
    \\  "layers": [
    \\    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
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
