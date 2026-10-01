// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public readonly partial record struct MetalBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    NativePointer Texture
)
{
    public static MetalBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_metal_borrowed_texture_descriptor_default");
            return CopyMetalBorrowedTextureDescriptor(
                NativeMethods.mln_metal_borrowed_texture_descriptor_default()
            );
        }
    }
}
