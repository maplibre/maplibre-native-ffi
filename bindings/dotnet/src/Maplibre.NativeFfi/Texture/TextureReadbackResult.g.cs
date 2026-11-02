// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The pixels that <c>RenderSessionHandle.ReadTextureAsync()</c> reads.
/// </summary>
/// <remarks>
/// See <c>mln_texture_readback_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct TextureReadbackResult
{
    public TextureReadbackResult(ImmutableArray<byte> Data, TextureImageInfo Info)
    {
        this.Data = Data;
        this.Info = Info;
    }

    /// <summary>
    /// Premultiplied RGBA pixels with 8 bits per channel, in rows from top to
    /// bottom.
    /// </summary>
    public ImmutableArray<byte> Data
    {
        get => DataStorage.AsImmutable();
        init => DataStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> DataStorage { get; init; }
    public TextureImageInfo Info { get; init; }
}
