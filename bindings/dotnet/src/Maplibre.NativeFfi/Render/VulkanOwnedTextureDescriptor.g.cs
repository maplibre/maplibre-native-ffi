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

public readonly partial record struct VulkanOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    VulkanContextDescriptor Context
)
{
    public static VulkanOwnedTextureDescriptor Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_vulkan_owned_texture_descriptor_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyVulkanOwnedTextureDescriptor(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_vulkan_owned_texture_descriptor_default()
            );
        }
    }
}
