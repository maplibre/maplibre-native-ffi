// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record StyleSourceResult
{
    public required StyleSourceInfo Info { get; set; }
    public string? Attribution { get; set; }
    public string? Url { get; set; }
    public string[]? TileUrls
    {
        get => TileUrlsStorage?.ToArray();
        set => TileUrlsStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<string>? TileUrlsStorage { get; set; }
}
