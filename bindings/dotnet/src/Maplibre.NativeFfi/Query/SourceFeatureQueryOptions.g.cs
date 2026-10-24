// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for source feature queries.
/// </summary>
/// <remarks>
/// See <c>mln_source_feature_query_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public sealed record SourceFeatureQueryOptions
{
    /// <summary>
    /// Optional source-layer IDs. Required by vector sources; ignored by
    /// GeoJSON.
    /// </summary>
    public string[]? SourceLayerIds
    {
        get => SourceLayerIdsStorage?.ToArray();
        init => SourceLayerIdsStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<string>? SourceLayerIdsStorage { get; init; }

    /// <summary>
    /// Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
    /// </summary>
    public byte[]? Filter
    {
        get => FilterStorage?.ToArray();
        init => FilterStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? FilterStorage { get; init; }
    public static SourceFeatureQueryOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopySourceFeatureQueryOptions(
                NativeMethods.mln_source_feature_query_options_default()
            );
        }
    }
}
