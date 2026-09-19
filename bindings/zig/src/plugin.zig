const std = @import("std");

const c = @import("c.zig").raw;
const native_temp = @import("native_temp.zig");
const status = @import("status.zig");

/// Loads a layer plugin shared library and registers its layer types.
///
/// The library at `path` must export `entry_point` as
/// `mln_plugin_status (*)(mln_plugin_register_function_v1, char*, size_t)`;
/// the C API opens the library, resolves the entry point, and calls it with
/// the process-wide register function. The plugin binary never links the host
/// library, so any prebuilt plugin that follows the entry-point contract loads
/// here. The library is never unloaded, because registration retains the
/// plugin's callbacks for the rest of the process.
///
/// Registration is process-wide, so this is a plain function rather than a
/// map method: call it on any thread, before any style that uses the plugin's
/// layer types loads. Loading the same plugin twice succeeds. A load failure
/// (missing file, missing symbol, rejected registration) reports
/// `error.NativeError` with a thread diagnostic.
pub fn loadPlugin(
    allocator: std.mem.Allocator,
    path: []const u8,
    entry_point: []const u8,
) status.Error!void {
    var temp = native_temp.TempStorage.init(allocator);
    defer temp.deinit();
    try status.checkStatus(
        c.mln_plugin_load_library(try temp.stringView(path), try temp.stringView(entry_point)),
        null,
    );
}
