// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGL context fields shared by OpenGL render targets in the browser.
/// </summary>
/// <remarks>
/// See <c>mln_webgl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Kind">
/// One <c>mln_webgl_context_kind</c> value.
/// </param>
/// <param name="Context">
/// Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
/// </param>
/// <param name="CanvasSelector">
/// Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
/// canvas must still be transferable when attachment starts.
/// </param>
public readonly partial record struct WebglContextDescriptor(
    WebglContextKind Kind,
    int Context,
    string CanvasSelector
);
