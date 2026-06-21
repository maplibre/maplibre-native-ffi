const build_options = @import("build_options");

pub const RenderTarget = if (build_options.supports_metal)
    @import("metal/mod.zig").MetalRenderTarget
else if (build_options.supports_opengl)
    @import("opengl/mod.zig").OpenGLRenderTarget
else if (build_options.supports_vulkan)
    @import("vulkan/mod.zig").VulkanRenderTarget
else
    @compileError("zig-map currently supports Metal, OpenGL, and Vulkan variants");
