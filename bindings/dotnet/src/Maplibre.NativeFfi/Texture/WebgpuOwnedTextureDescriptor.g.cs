// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGPU attachment options for an owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_webgpu_owned_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WebgpuOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    WebgpuContextDescriptor Context
)
{
    public WebgpuOwnedTextureDescriptor()
        : this(new RenderTargetExtent(), default) { }

    public static WebgpuOwnedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_webgpu_owned_texture_descriptor_default");
            return GeneratedValues.CopyWebgpuOwnedTextureDescriptor(
                NativeMethods.mln_webgpu_owned_texture_descriptor_default()
            );
        }
    }
}
