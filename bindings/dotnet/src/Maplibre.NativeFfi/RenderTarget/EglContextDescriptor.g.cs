// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The EGL objects that a render session makes its OpenGL context from.
/// </summary>
/// <remarks>
/// See <c>mln_egl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct EglContextDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public EglContextDescriptor(
        NativePointer Display,
        NativePointer Config,
        NativePointer ShareContext,
        OpenGlClientApi ClientApi,
        NativePointer GetProcAddress
    )
    {
        this.Display = Display;
        this.Config = Config;
        this.ShareContext = ShareContext;
        this.ClientApi = ClientApi;
        this.GetProcAddress = GetProcAddress;
    }

    /// <summary>
    /// An <c>EGLDisplay</c>.
    /// </summary>
    public required NativePointer Display { get; init; }

    /// <summary>
    /// An <c>EGLConfig</c>.
    /// </summary>
    public required NativePointer Config { get; init; }

    /// <summary>
    /// Your <c>EGLContext</c>, for a shared context.
    /// </summary>
    public NativePointer ShareContext { get; init; }

    /// <summary>
    /// The API of a dedicated context. A shared context uses the API of yours.
    /// </summary>
    public OpenGlClientApi ClientApi { get; init; }

    /// <summary>
    /// Your <c>eglGetProcAddress</c>, or null to use the system's.
    /// </summary>
    public NativePointer GetProcAddress { get; init; }
}
