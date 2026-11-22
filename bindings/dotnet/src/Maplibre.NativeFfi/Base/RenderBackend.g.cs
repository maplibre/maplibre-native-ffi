// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A render backend.
/// </summary>
/// <remarks>
/// See <c>mln_render_backend</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public enum RenderBackend : uint
{
    Metal = 1,
    Vulkan = 2,
    OpenGl = 3,
    WebGpu = 4,

    /// <summary>
    /// D3D12, on Windows. It compiles its shaders at run time with DXC, which
    /// your app ships: <c>dxcompiler.dll</c>, 1.8.2502 or later, beside your
    /// app's executable or in another directory of the DLL search path, which
    /// doesn't take the library's own directory. Without it, the library
    /// renders on another backend.
    /// </summary>
    D3d12 = 5,
}
