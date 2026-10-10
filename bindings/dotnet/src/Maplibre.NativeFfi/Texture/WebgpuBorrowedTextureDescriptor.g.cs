// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGPU attachment options for a borrowed texture target.
/// </summary>
/// <remarks>
/// See <c>mln_webgpu_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// Logical texture extent.
/// </param>
/// <param name="PhysicalWidth">
/// Physical texture width in device pixels. Defaults to 256.
/// </param>
/// <param name="PhysicalHeight">
/// Physical texture height in device pixels. Defaults to 256.
/// </param>
/// <param name="Context">
/// Borrowed WebGPU context. device is required.
/// </param>
/// <param name="Texture">
/// Borrowed WGPUTexture. Required.
/// </param>
/// <param name="TextureView">
/// Borrowed WGPUTextureView for texture. Required.
/// </param>
/// <param name="Format">
/// Backend-native WGPUTextureFormat value. Undefined is invalid.
/// </param>
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
    public WebgpuBorrowedTextureDescriptor()
        : this(new RenderTargetExtent(), 256, 256, default, default, default, default) { }

    public static WebgpuBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(
                null,
                "mln_webgpu_borrowed_texture_descriptor_default"
            );
            return GeneratedValues.CopyWebgpuBorrowedTextureDescriptor(
                NativeMethods.mln_webgpu_borrowed_texture_descriptor_default()
            );
        }
    }
}
