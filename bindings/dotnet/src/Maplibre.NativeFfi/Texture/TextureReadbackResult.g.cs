// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Texture readback borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_texture_readback_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct TextureReadbackResult
{
    public TextureReadbackResult(ImmutableArray<byte> Data, TextureImageInfo Info)
    {
        this.Data = Data;
        this.Info = Info;
    }

    /// <summary>
    /// Borrowed pixel bytes, valid only during the callback.
    /// </summary>
    public ImmutableArray<byte> Data
    {
        get => DataStorage.AsImmutable();
        init => DataStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> DataStorage { get; init; }
    public TextureImageInfo Info { get; init; }
}
