// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL backend context fields shared by OpenGL render targets.
/// </summary>
/// <remarks>
/// See <c>mln_open_gl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OpenGlContextDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlContextDescriptor(
        OpenGlContextOwnership Ownership,
        OpenGlContextDescriptor.DataValue Data
    )
    {
        this.Ownership = Ownership;
        this.Data = Data;
    }

    /// <summary>
    /// Whether the session shares its driver thread and graphics objects with
    /// the host. A private EGL or WGL owned texture and a transferred WebGL
    /// canvas are dedicated to their core worker.
    /// </summary>
    public OpenGlContextOwnership Ownership { get; init; }
    public required OpenGlContextDescriptor.DataValue Data { get; init; }

    public abstract record DataValue
    {
        private DataValue() { }

        public sealed record Wgl(WglContextDescriptor Value) : DataValue;

        public sealed record Egl(EglContextDescriptor Value) : DataValue;

        public sealed record WebGl(WebGlContextDescriptor Value) : DataValue;
    }
}
