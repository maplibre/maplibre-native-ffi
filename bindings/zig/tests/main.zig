const maplibre = @import("maplibre_native_ffi");

// Analyzing every non-generic generated function and method checks that the
// whole generated surface compiles against the C headers, including the
// functions no test calls.
comptime {
    @setEvalBranchQuota(1000000);
    analyzeFunctions(maplibre);
    for (@typeInfo(maplibre).@"struct".decl_names) |decl| {
        const value = @field(maplibre, decl);
        if (@TypeOf(value) == type and @typeInfo(value) == .@"struct") analyzeFunctions(value);
    }
    _ = @import("generated_workflows.zig");
    _ = @import("diagnostics.zig");
    _ = @import("lifecycle.zig");
    _ = @import("callbacks.zig");
    _ = @import("values.zig");
    _ = @import("rendering.zig");
}

fn analyzeFunctions(comptime Container: type) void {
    for (@typeInfo(Container).@"struct".decl_names) |decl| {
        const value = @field(Container, decl);
        if (@typeInfo(@TypeOf(value)) == .@"fn" and !@typeInfo(@TypeOf(value)).@"fn".is_generic) _ = &value;
    }
}

// The suite links the library the way the package does, through build.zig, and
// the library it loads reports the C ABI version this binding expects.
test "package validates the supported C ABI version" {
    var diagnostic: maplibre.Diagnostic = .{};
    try maplibre.validateAbiVersion(&diagnostic);
}
