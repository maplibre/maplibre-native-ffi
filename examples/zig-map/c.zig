const build_options = @import("build_options");

pub const c = if (build_options.supports_metal) @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3/SDL_metal.h");
}) else if (build_options.supports_vulkan) @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3/SDL_vulkan.h");
    @cInclude("vulkan/vulkan.h");
}) else @compileError("zig-map currently supports Metal and Vulkan variants");
