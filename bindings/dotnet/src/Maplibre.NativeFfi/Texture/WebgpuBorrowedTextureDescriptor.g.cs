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
public readonly record struct WebgpuBorrowedTextureDescriptor
{
    public WebgpuBorrowedTextureDescriptor()
        : this(new LogicalExtent(), 256, 256, default, default!, default) { }

    public WebgpuBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        WebgpuContextDescriptor Context,
        WebgpuBorrowedTexture[] Textures,
        uint Format
    )
    {
        this.Extent = Extent;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Context = Context;
        this.Textures = Textures;
        this.Format = Format;
    }

    public LogicalExtent Extent { get; init; }
    public uint PhysicalWidth { get; init; }
    public uint PhysicalHeight { get; init; }
    public WebgpuContextDescriptor Context { get; init; }
    public WebgpuBorrowedTexture[] Textures
    {
        get => TexturesStorage.ToArray();
        init => TexturesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<WebgpuBorrowedTexture> TexturesStorage { get; init; }
    public uint Format { get; init; }
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
