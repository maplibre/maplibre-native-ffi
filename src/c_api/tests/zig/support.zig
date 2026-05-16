const testing = @import("std").testing;

pub const c = @cImport({
    @cInclude("maplibre_native_c.h");
});

pub fn createRuntime() !*c.mln_runtime {
    var runtime: ?*c.mln_runtime = null;
    var options = c.mln_runtime_options_default();
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_create(&options, &runtime));
    return runtime orelse error.RuntimeCreateFailed;
}

pub fn createMap(runtime: *c.mln_runtime) !*c.mln_map {
    var map: ?*c.mln_map = null;
    var options = c.mln_map_options_default();
    options.width = 512;
    options.height = 512;
    try testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_create(runtime, &options, &map));
    return map orelse error.MapCreateFailed;
}

pub fn destroyRuntime(runtime: *c.mln_runtime) void {
    testing.expectEqual(c.MLN_STATUS_OK, c.mln_runtime_destroy(runtime)) catch @panic("runtime destroy failed");
}

pub fn destroyMap(map: *c.mln_map) void {
    testing.expectEqual(c.MLN_STATUS_OK, c.mln_map_destroy(map)) catch @panic("map destroy failed");
}
