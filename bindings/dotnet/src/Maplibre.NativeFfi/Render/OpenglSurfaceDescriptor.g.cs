// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public readonly partial record struct OpenglSurfaceDescriptor(
    RenderTargetExtent Extent,
    OpenglContextDescriptor Context,
    NativePointer Surface
)
{
    public static OpenglSurfaceDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_opengl_surface_descriptor_default");
            return CopyOpenglSurfaceDescriptor(
                NativeMethods.mln_opengl_surface_descriptor_default()
            );
        }
    }
}
