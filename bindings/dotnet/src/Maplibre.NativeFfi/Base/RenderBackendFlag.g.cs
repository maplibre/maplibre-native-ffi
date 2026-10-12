// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Render backend support flags reported by this native library build.
/// </summary>
/// <remarks>
/// See <c>mln_render_backend_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum RenderBackendFlag : uint
{
    Metal = 1,
    Vulkan = 2,
    Opengl = 4,
    Webgpu = 8,
}
