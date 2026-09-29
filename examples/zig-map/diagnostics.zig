const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

pub fn logError(
    message: []const u8,
    err: anyerror,
    diagnostic: ?*const maplibre.Diagnostic,
) void {
    std.debug.print("{s}: {s}\n", .{ message, @errorName(err) });
    const details = diagnostic orelse return;
    if (details.message().len == 0) return;
    std.debug.print("diagnostic", .{});
    if (details.raw_status) |raw_status| std.debug.print(" ({d})", .{raw_status});
    std.debug.print(": {s}\n", .{details.message()});
}

pub fn logRecord(_: ?*anyopaque, severity: maplibre.LogSeverity, _: maplibre.LogEvent, _: i64, message: []const u8) maplibre.Error!u32 {
    std.debug.print("[{s}] {s}\n", .{ @tagName(severity), message });
    return 1;
}
