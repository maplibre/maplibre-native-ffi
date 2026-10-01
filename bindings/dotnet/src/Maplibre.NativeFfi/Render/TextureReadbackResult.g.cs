// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

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
