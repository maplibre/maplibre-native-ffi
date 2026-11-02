// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The WebGL context or canvas that a render session uses.
/// </summary>
/// <remarks>
/// See <c>mln_web_gl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Context">
/// Your <c>EMSCRIPTEN_WEBGL_CONTEXT_HANDLE</c>, for an existing context.
/// </param>
/// <param name="CanvasSelector">
/// The canvas's CSS selector, for a transferred canvas.
/// </param>
public readonly partial record struct WebGlContextDescriptor(
    WebGlContextKind Kind,
    int Context,
    string CanvasSelector
)
{
    public WebGlContextDescriptor()
        : this(default, default, "") { }
}
