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

public readonly partial record struct VulkanBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    VulkanContextDescriptor Context,
    ulong Image,
    ulong ImageView,
    uint Format,
    uint InitialLayout,
    uint FinalLayout
)
{
    public static VulkanBorrowedTextureDescriptor Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_vulkan_borrowed_texture_descriptor_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyVulkanBorrowedTextureDescriptor(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_vulkan_borrowed_texture_descriptor_default()
            );
        }
    }
}
