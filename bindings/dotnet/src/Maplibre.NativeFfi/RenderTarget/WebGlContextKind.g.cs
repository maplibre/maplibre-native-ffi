// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGL context placement.
/// </summary>
/// <remarks>
/// See <c>mln_web_gl_context_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum WebGlContextKind : uint
{
    /// <summary>
    /// Use a host-created context on its current browser agent.
    /// </summary>
    Existing = 0,

    /// <summary>
    /// Transfer the canvas named by canvas_selector to a native worker and
    /// create a WebGL 2 context there.
    /// </summary>
    TransferredCanvas = 1,
}
