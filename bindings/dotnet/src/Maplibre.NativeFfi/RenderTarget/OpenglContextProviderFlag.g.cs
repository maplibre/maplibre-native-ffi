// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL context providers supported by this build.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_context_provider_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum OpenglContextProviderFlag : uint
{
    Wgl = 1,
    Egl = 2,

    /// <summary>
    /// Browser WebGL context imported into an Emscripten module.
    /// </summary>
    Webgl = 4,
}
