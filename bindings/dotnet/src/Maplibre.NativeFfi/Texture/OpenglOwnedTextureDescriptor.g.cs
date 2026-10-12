// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL attachment options for an owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_owned_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// Logical texture extent. A scale_factor that differs from the map's is
/// accepted and logged as a warning.
/// </param>
/// <param name="Context">
/// Borrowed OpenGL context provider data. Shared ownership creates a context
/// whose texture frames the host can acquire. Dedicated EGL or transferred
/// WebGL ownership creates a private core-worker context for CPU readback.
/// </param>
public readonly partial record struct OpenglOwnedTextureDescriptor(
    LogicalExtent Extent,
    OpenglContextDescriptor Context
)
{
    public OpenglOwnedTextureDescriptor()
        : this(new LogicalExtent(), default) { }

    public static OpenglOwnedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_opengl_owned_texture_descriptor_default");
            return GeneratedValues.CopyOpenglOwnedTextureDescriptor(
                NativeMethods.mln_opengl_owned_texture_descriptor_default()
            );
        }
    }
}
