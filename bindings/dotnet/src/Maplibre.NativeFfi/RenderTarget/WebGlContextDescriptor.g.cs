// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The WebGL2 context that an OpenGL device wraps.
/// </summary>
/// <remarks>
/// See <c>mln_web_gl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WebGlContextDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WebGlContextDescriptor(uint Context)
    {
        this.Context = Context;
    }

    /// <summary>
    /// Your <c>WebGL2RenderingContext</c>.
    /// </summary>
    public required uint Context { get; init; }
}
