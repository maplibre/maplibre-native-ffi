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
    public TextureReadbackResult(byte[] Data, TextureImageInfo Info)
    {
        this.Data = Data;
        this.Info = Info;
    }

    public byte[] Data
    {
        get => DataStorage.ToArray();
        init => DataStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> DataStorage { get; init; }
    public TextureImageInfo Info { get; init; }
}
