const build_options = @import("build_options");

comptime {
    _ = @import("core_abi.zig");
    _ = @import("map_options_abi.zig");
    _ = @import("resources_abi.zig");
    _ = @import("style_values_abi.zig");
    // owned texture and query tests require a GPU-backed texture session;
    // OpenGL uses EGL surface sessions and has no owned texture attach API.
    if (!build_options.supports_opengl) {
        _ = @import("query_abi.zig");
        _ = @import("owned_texture_abi.zig");
    }
    _ = @import("metal_backend_abi.zig");
    _ = @import("vulkan_backend_abi.zig");
}
