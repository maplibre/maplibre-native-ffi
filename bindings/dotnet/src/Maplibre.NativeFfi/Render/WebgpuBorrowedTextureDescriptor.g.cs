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

public readonly partial record struct WebgpuBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    WebgpuContextDescriptor Context,
    NativePointer Texture,
    NativePointer TextureView,
    uint Format
)
{
    public static WebgpuBorrowedTextureDescriptor Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_webgpu_borrowed_texture_descriptor_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyWebgpuBorrowedTextureDescriptor(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_webgpu_borrowed_texture_descriptor_default()
            );
        }
    }
}
