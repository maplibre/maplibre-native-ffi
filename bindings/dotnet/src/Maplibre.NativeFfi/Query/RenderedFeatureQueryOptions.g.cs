// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for a rendered feature query.
/// </summary>
/// <remarks>
/// See <c>mln_rendered_feature_query_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public sealed record RenderedFeatureQueryOptions
{
    /// <summary>
    /// The layers to query. Without them, the query covers every layer.
    /// </summary>
    public ImmutableArray<string>? LayerIds
    {
        get => LayerIdsStorage?.AsImmutable();
        init => LayerIdsStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<string>? LayerIdsStorage { get; init; }

    /// <summary>
    /// A [filter](https://maplibre.org/maplibre-style-spec/expressions/) that
    /// each feature must match. Empty matches every feature.
    /// </summary>
    public string Filter { get; init; } = "";
}
