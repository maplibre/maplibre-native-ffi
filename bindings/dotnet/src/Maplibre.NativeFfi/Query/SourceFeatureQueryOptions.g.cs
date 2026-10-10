// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record SourceFeatureQueryOptions
{
    public string[]? SourceLayerIds
    {
        get => SourceLayerIdsStorage?.ToArray();
        set => SourceLayerIdsStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<string>? SourceLayerIdsStorage { get; set; }
    public byte[]? Filter
    {
        get => FilterStorage?.ToArray();
        set => FilterStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? FilterStorage { get; set; }
    public static SourceFeatureQueryOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_source_feature_query_options_default");
            return GeneratedValues.CopySourceFeatureQueryOptions(
                NativeMethods.mln_source_feature_query_options_default()
            );
        }
    }
}
