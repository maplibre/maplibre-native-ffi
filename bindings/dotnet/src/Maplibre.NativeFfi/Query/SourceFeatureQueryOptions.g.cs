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
    public ImmutableArray<string>? SourceLayerIds
    {
        get => SourceLayerIdsStorage?.AsImmutable();
        init => SourceLayerIdsStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<string>? SourceLayerIdsStorage { get; init; }

    /// <summary>
    /// UTF-8 MapLibre style-spec filter JSON, or empty for no filter.
    /// </summary>
    public string Filter { get; init; } = "";
}
