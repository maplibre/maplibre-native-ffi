// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// An OpenGL context platform, as a bit in a set.
/// </summary>
/// <remarks>
/// See <c>mln_open_gl_context_provider_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum OpenGlContextProviderFlag : uint
{
    Wgl = 1,
    Egl = 2,
    WebGl = 4,
}
