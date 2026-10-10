// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Borrowed inline TileJSON tile URLs available during a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_tile_urls_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
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
