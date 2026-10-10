// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct OpenglSurfaceDescriptor(
    RenderTargetExtent Extent,
    OpenglContextDescriptor Context,
    NativePointer Surface
)
{
    public OpenglSurfaceDescriptor()
        : this(new RenderTargetExtent(), default, default) { }

    public static OpenglSurfaceDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_opengl_surface_descriptor_default");
            return GeneratedValues.CopyOpenglSurfaceDescriptor(
                NativeMethods.mln_opengl_surface_descriptor_default()
            );
        }
    }
}
