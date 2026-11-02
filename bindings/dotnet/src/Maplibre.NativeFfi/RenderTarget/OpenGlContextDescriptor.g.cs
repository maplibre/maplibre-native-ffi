// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a render session gets the OpenGL context that it renders with.
/// </summary>
/// <remarks>
/// <para>Keep the objects that it names alive until a detach or disposal of the
/// session completes, or an abandon of it returns.</para>
/// <para>See <c>mln_open_gl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
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
