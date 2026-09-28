const std = @import("std");
const maplibre = @import("maplibre_native_ffi");

pub fn logError(
    message: []const u8,
    err: anyerror,
    diagnostic_store: ?*const maplibre.DiagnosticStore,
) void {
    std.debug.print("{s}: {s}\n", .{ message, @errorName(err) });
    const diagnostic = if (diagnostic_store) |store| store.get() else null;
    if (diagnostic) |details| {
        std.debug.print("native diagnostic", .{});
        if (details.raw_status) |raw_status| std.debug.print(" ({d})", .{raw_status});
        std.debug.print(": {s}\n", .{details.message});
    }
}

pub fn logRecord(_: ?*anyopaque, severity: maplibre.LogSeverity, _: maplibre.LogEvent, _: i64, message: []const u8) maplibre.Error!u32 {
    std.debug.print("[{s}] {s}\n", .{ @tagName(severity), message });
    return 1;
}
