// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Complete source metadata borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
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
