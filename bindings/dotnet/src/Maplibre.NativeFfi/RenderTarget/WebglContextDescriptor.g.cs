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
public readonly partial record struct WebglContextDescriptor(
    WebglContextKind Kind,
    int Context,
    string CanvasSelector
);
