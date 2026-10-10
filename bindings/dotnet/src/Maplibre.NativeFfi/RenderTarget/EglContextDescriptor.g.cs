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
/// <param name="Display">
/// Borrowed EGLDisplay. Required and kept initialized through teardown.
/// </param>
/// <param name="Config">
/// Borrowed EGLConfig used to create the session context. Required. OpenGL
/// texture targets require EGL_SURFACE_TYPE to include EGL_PBUFFER_BIT.
/// </param>
/// <param name="ShareContext">
/// Borrowed EGLContext whose share group the session context joins. Required
/// under shared ownership, where the session also takes its client API from
/// this context. A dedicated session joins no share group, so it must be null
/// there and names client_api instead.
/// </param>
/// <param name="ClientApi">
/// Client API the session creates its context for. Required under dedicated
/// ownership. A shared session queries share_context for it, so this is ignored
/// there.
/// </param>
/// <param name="GetProcAddress">
/// Optional eglGetProcAddress-compatible function for the host loader.
/// </param>
public readonly partial record struct EglContextDescriptor(
    NativePointer Display,
    NativePointer Config,
    NativePointer ShareContext,
    OpenglClientApi ClientApi,
    NativePointer GetProcAddress
);
