// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

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
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_opengl_surface_descriptor_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyOpenglSurfaceDescriptor(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_opengl_surface_descriptor_default()
            );
        }
    }
}
