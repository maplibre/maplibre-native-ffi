const std = @import("std");
const maplibre = @import("maplibre_native");

pub fn logError(message: []const u8, err: anyerror) void {
    std.debug.print("{s}: {s}\n", .{ message, @errorName(err) });
}

pub fn logRecord(_: ?*anyopaque, record: maplibre.LogRecord) bool {
    std.debug.print("[{s}] {s}\n", .{ @tagName(record.severity), record.message });
    return true;
}
