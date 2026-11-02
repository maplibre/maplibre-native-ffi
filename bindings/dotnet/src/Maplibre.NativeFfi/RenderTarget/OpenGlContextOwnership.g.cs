// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Whether a render session's OpenGL context shares objects with yours.
/// </summary>
/// <remarks>
/// See <c>mln_open_gl_context_ownership</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum OpenGlContextOwnership : uint
{
    /// <summary>
    /// The session's context shares objects with your <c>share_context</c>.
    /// </summary>
    Shared = 0,

    /// <summary>
    /// The session's context shares objects with no other context.
    /// </summary>
    Dedicated = 1,
}
