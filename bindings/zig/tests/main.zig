const maplibre = @import("maplibre_native_ffi");

// Analyzing every non-generic generated function checks that the whole
// generated surface compiles against the C headers, including the functions no
// test calls.
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
    _ = @import("rendering.zig");
    _ = @import("runtime.zig");
    _ = @import("map_lifecycle.zig");
    _ = @import("resources.zig");
    _ = @import("logging.zig");
}

// The suite links the library the way the package does, through build.zig, and
// the library it loads reports the C ABI version this binding expects.
test "package validates the supported C ABI version" {
    var diagnostic: maplibre.Diagnostic = .{};
    try maplibre.validateAbiVersion(&diagnostic);
}
