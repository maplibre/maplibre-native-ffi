const build_options = @import("build_options");

pub const c = if (build_options.supports_metal) @cImport({
    @cInclude("maplibre_native_c.h");
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3/SDL_metal.h");
}) else if (build_options.supports_vulkan) @cImport({
    @cInclude("maplibre_native_c.h");
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3/SDL_vulkan.h");
    @cInclude("vulkan/vulkan.h");
}) else if (build_options.supports_opengl) @cImport({
    @cInclude("maplibre_native_c.h");
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3/SDL_egl.h");
}) else @compileError("zig-map currently supports Metal, Vulkan, and OpenGL variants");
