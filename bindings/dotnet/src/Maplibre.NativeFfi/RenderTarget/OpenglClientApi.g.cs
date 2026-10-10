// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL client API a dedicated EGL session creates its context for.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_client_api</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum OpenglClientApi : uint
{
    /// <summary>
    /// No client API is named.
    /// </summary>
    Unspecified = 0,

    /// <summary>
    /// Desktop OpenGL, as EGL_OPENGL_API names it.
    /// </summary>
    Gl = 1,

    /// <summary>
    /// OpenGL ES, as EGL_OPENGL_ES_API names it.
    /// </summary>
    Gles = 2,
}
