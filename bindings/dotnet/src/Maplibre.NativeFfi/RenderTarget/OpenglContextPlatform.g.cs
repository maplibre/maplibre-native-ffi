// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL platform context provider used by a context descriptor.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_context_platform</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum OpenglContextPlatform : uint
{
    /// <summary>
    /// No OpenGL context provider is selected.
    /// </summary>
    Unspecified = 0,
    Wgl = 1,
    Egl = 2,

    /// <summary>
    /// Emscripten WebGL context handle.
    /// </summary>
    Webgl = 3,
}
