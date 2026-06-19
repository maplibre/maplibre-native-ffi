const build_options = @import("build_options");

pub const c = if (build_options.supports_metal or build_options.supports_vulkan or build_options.supports_opengl)
    @import("c")
else
    @compileError("zig-map currently supports Metal, OpenGL, and Vulkan variants");
