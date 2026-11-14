// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A render target kind, as a bit in a set.
/// </summary>
/// <remarks>
/// See <c>mln_render_target_kind_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum RenderTargetKindFlag : uint
{
    Offscreen = 1,
    MetalLayer = 2,
    Win32Window = 4,
    XlibWindow = 8,
    WaylandSurface = 16,
    AndroidWindow = 32,
    OhosWindow = 64,
    VulkanSurface = 512,
    OpenGlSurface = 1024,
    MetalBorrowedTextures = 2048,
    VulkanBorrowedTextures = 4096,
    OpenGlBorrowedTextures = 8192,
    WebGpuBorrowedTextures = 16384,
    ExportedRing = 32768,
    WebCanvas = 65536,
}
