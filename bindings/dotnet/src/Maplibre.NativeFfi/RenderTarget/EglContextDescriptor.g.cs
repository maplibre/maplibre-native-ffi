// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your EGL context, which each session's context shares objects with.
/// </summary>
/// <remarks>
/// <para>Each session makes its context on <c>display</c> with <c>config</c>
/// and the client API and version of <c>share_context</c>.</para>
/// <para>See <c>mln_egl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct EglContextDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public EglContextDescriptor(
        NativePointer Display,
        NativePointer Config,
        NativePointer ShareContext,
        NativePointer GetProcAddress
    )
    {
        this.Display = Display;
        this.Config = Config;
        this.ShareContext = ShareContext;
        this.GetProcAddress = GetProcAddress;
    }

    /// <summary>
    /// An <c>EGLDisplay</c>, which you initialized.
    /// </summary>
    public required NativePointer Display { get; init; }

    /// <summary>
    /// The <c>EGLConfig</c> of <c>share_context</c>.
    /// </summary>
    public required NativePointer Config { get; init; }

    /// <summary>
    /// Your <c>EGLContext</c>.
    /// </summary>
    public required NativePointer ShareContext { get; init; }

    /// <summary>
    /// Your <c>eglGetProcAddress</c>, or null to use the system's.
    /// </summary>
    public NativePointer GetProcAddress { get; init; }
}
