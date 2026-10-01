// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public readonly record struct StyleSourceTileUrlsResult
{
    public StyleSourceTileUrlsResult(string[] TileUrls)
    {
        this.TileUrls = TileUrls;
    }

    public string[] TileUrls
    {
        get => TileUrlsStorage.ToArray();
        init => TileUrlsStorage = ValueArray.Copy(value);
    }
    internal ValueArray<string> TileUrlsStorage { get; init; }
}
