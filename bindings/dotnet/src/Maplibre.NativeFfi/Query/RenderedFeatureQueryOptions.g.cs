// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Query;

public sealed record RenderedFeatureQueryOptions
{
    public string[]? LayerIds
    {
        get => LayerIdsStorage?.ToArray();
        set => LayerIdsStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<string>? LayerIdsStorage { get; set; }
    public byte[]? Filter
    {
        get => FilterStorage?.ToArray();
        set => FilterStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? FilterStorage { get; set; }
    public static RenderedFeatureQueryOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_rendered_feature_query_options_default");
            return CopyRenderedFeatureQueryOptions(
                NativeMethods.mln_rendered_feature_query_options_default()
            );
        }
    }
}
