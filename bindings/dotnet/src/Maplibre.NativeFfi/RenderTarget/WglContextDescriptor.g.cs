// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your WGL context, which each session's context shares objects with.
/// </summary>
/// <remarks>
/// See <c>mln_wgl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WglContextDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WglContextDescriptor(
        NativePointer DeviceContext,
        NativePointer ShareContext,
        NativePointer GetProcAddress
    )
    {
        this.DeviceContext = DeviceContext;
        this.ShareContext = ShareContext;
        this.GetProcAddress = GetProcAddress;
    }

    /// <summary>
    /// An <c>HDC</c> with the pixel format of <c>share_context</c>.
    /// </summary>
    public required NativePointer DeviceContext { get; init; }

    /// <summary>
    /// Your <c>HGLRC</c>.
    /// </summary>
    public required NativePointer ShareContext { get; init; }

    /// <summary>
    /// Your <c>wglGetProcAddress</c>, or null to use the system's.
    /// </summary>
    public NativePointer GetProcAddress { get; init; }
}
