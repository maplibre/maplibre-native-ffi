// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGL context placement.
/// </summary>
/// <remarks>
/// See <c>mln_webgl_context_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum WebglContextKind : uint
{
    /// <summary>
    /// Use a host-created context on its current browser agent.
    /// </summary>
    Existing = 0,

    /// <summary>
    /// Create a WebGL 2 context on a native worker whose pthread creation
    /// claims canvas_selector through Emscripten's transferred-canvases
    /// attribute.
    /// </summary>
    TransferredCanvas = 1,
}
