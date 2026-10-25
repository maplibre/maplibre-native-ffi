// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// EGL context fields shared by OpenGL render targets.
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
    /// Borrowed EGLDisplay. Required and kept initialized through teardown.
    /// eglGetDisplay(EGL_DEFAULT_DISPLAY) returns the same display for the
    /// whole process, so a shared display must stay initialized until every
    /// session using it has detached or been destroyed. The host must not call
    /// eglTerminate per session.
    /// </summary>
    public required NativePointer Display { get; init; }

    /// <summary>
    /// Borrowed EGLConfig used to create the session context. Required. OpenGL
    /// texture targets require EGL_SURFACE_TYPE to include EGL_PBUFFER_BIT.
    /// </summary>
    public required NativePointer Config { get; init; }

    /// <summary>
    /// Borrowed EGLContext whose share group the session context joins.
    /// Required under shared ownership, where the session also takes its client
    /// API from this context. A dedicated session joins no share group, so it
    /// must be null there and names client_api instead.
    /// </summary>
    public NativePointer ShareContext { get; init; }

    /// <summary>
    /// Client API the session creates its context for. Required under dedicated
    /// ownership. A shared session queries share_context for it, so this is
    /// ignored there.
    /// </summary>
    public OpenGlClientApi ClientApi { get; init; }

    /// <summary>
    /// Optional eglGetProcAddress-compatible function for the host loader.
    /// </summary>
    public NativePointer GetProcAddress { get; init; }
}
