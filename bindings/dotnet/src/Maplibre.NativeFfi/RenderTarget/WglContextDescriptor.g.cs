// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WGL context fields shared by OpenGL render targets on Windows.
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
    /// Borrowed HDC used to create the session context. Required. A dedicated
    /// owned texture makes its context current on this HDC from its core
    /// worker.
    /// </summary>
    public required NativePointer DeviceContext { get; init; }

    /// <summary>
    /// Borrowed HGLRC whose share group the session context joins. Required
    /// under shared ownership. A dedicated session joins no share group, so it
    /// must be null there.
    /// </summary>
    public NativePointer ShareContext { get; init; }

    /// <summary>
    /// Optional wglGetProcAddress-compatible function for the host loader.
    /// </summary>
    public NativePointer GetProcAddress { get; init; }
}
