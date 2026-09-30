const testing = @import("std").testing;

const maplibre = @import("maplibre_native_ffi");

comptime {
    @setEvalBranchQuota(100000);
    for (@typeInfo(maplibre.generated).@"struct".decls) |decl| {
        const value = @field(maplibre.generated, decl.name);
        if (@typeInfo(@TypeOf(value)) == .@"fn" and !@typeInfo(@TypeOf(value)).@"fn".is_generic) _ = &value;
    }
    _ = @import("generated_workflows.zig");
    _ = @import("diagnostics.zig");
    _ = @import("lifecycle.zig");
    _ = @import("callbacks.zig");
    _ = @import("values.zig");
    _ = @import("runtime.zig");
    _ = @import("map_lifecycle.zig");
    _ = @import("camera.zig");
    _ = @import("projection.zig");
    _ = @import("map_tuning.zig");
    _ = @import("style_values.zig");
    _ = @import("geojson.zig");
    _ = @import("style_sources.zig");
    _ = @import("resources.zig");
    _ = @import("logging.zig");
    _ = @import("render.zig");
    _ = @import("surface.zig");
}

test "package validates the supported C ABI version" {
    var diagnostic: maplibre.Diagnostic = .{};
    try maplibre.validateAbiVersion(&diagnostic);
}
