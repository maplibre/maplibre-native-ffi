const build_options = @import("build_options");

pub const Backend = if (build_options.supports_metal)
    @import("metal/mod.zig").MetalBackend
else if (build_options.supports_opengl)
    @import("opengl/mod.zig").OpenGLBackend
else if (build_options.supports_vulkan)
    @import("vulkan/mod.zig").VulkanBackend
else
    @compileError("zig-map currently supports Metal, OpenGL, and Vulkan variants");
