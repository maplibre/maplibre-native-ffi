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
    {
        this = Default;
    }

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

    /// <summary>
    /// Logical texture extent. The map viewport uses width and height and the
    /// renderer uses scale_factor; the physical size is stated separately
    /// below. A scale_factor that differs from the map's is accepted and logged
    /// as a warning.
    /// </summary>
    public LogicalExtent Extent { get; init; }

    /// <summary>
    /// Physical texture width in device pixels. Defaults to 256.
    /// </summary>
    public uint PhysicalWidth { get; init; }

    /// <summary>
    /// Physical texture height in device pixels. Defaults to 256.
    /// </summary>
    public uint PhysicalHeight { get; init; }

    /// <summary>
    /// Borrowed WebGPU context. device is required. Rendering is submitted
    /// through context.queue or that device's default queue.
    /// </summary>
    public WebgpuContextDescriptor Context { get; init; }

    /// <summary>
    /// The ring's textures, one per slot, in slot order. Required.
    /// </summary>
    public WebgpuBorrowedTexture[] Textures
    {
        get => TexturesStorage.ToArray();
        init => TexturesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<WebgpuBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// Backend-native WGPUTextureFormat value of every texture. Undefined is
    /// invalid.
    /// </summary>
    public uint Format { get; init; }
    public static WebgpuBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyWebgpuBorrowedTextureDescriptor(
                NativeMethods.mln_webgpu_borrowed_texture_descriptor_default()
            );
        }
    }
}
