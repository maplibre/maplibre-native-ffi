// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a session's OpenGL context relates to its driver thread and host
/// graphics state.
/// </summary>
/// <remarks>
/// See <c>mln_open_gl_context_ownership</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum OpenGlContextOwnership : uint
{
    /// <summary>
    /// The session shares its thread with host graphics work.
    /// </summary>
    Shared = 0,

    /// <summary>
    /// The session keeps its own OpenGL context current on its thread.
    /// </summary>
    Dedicated = 1,

    /// <summary>
    /// The session takes turns rendering in a host EGL context.
    /// </summary>
    Pooled = 2,
}
