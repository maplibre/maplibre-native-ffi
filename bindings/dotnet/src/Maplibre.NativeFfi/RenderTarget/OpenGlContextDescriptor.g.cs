// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your OpenGL context, for <c>GpuDeviceHandle.WrapOpenGl()</c>: the root of
/// the share group that the device's sessions join.
/// </summary>
/// <remarks>
/// See <c>mln_open_gl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public abstract record OpenGlContextDescriptor
{
    private OpenGlContextDescriptor() { }

    public sealed record Wgl(WglContextDescriptor Value) : OpenGlContextDescriptor;

    public sealed record Egl(EglContextDescriptor Value) : OpenGlContextDescriptor;

    public sealed record WebGl(WebGlContextDescriptor Value) : OpenGlContextDescriptor;
}
