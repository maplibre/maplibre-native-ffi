// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a render session gets its WebGL context.
/// </summary>
/// <remarks>
/// See <c>mln_web_gl_context_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum WebGlContextKind : uint
{
    /// <summary>
    /// The session uses a context that you created with
    /// <c>emscripten_webgl_create_context()</c>.
    /// </summary>
    Existing = 0,

    /// <summary>
    /// The library transfers the canvas that <c>canvas_selector</c> names to a
    /// worker and creates the context there.
    /// </summary>
    TransferredCanvas = 1,
}
