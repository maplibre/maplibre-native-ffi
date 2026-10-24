// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for rendered feature queries.
/// </summary>
/// <remarks>
/// See <c>mln_rendered_feature_query_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public sealed record RenderedFeatureQueryOptions
{
    /// <summary>
    /// Optional style layer IDs. When absent, all rendered layers are queried.
    /// </summary>
    public string[]? LayerIds
    {
        get => LayerIdsStorage?.ToArray();
        init => LayerIdsStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<string>? LayerIdsStorage { get; init; }

    /// <summary>
    /// Optional UTF-8 MapLibre style-spec filter JSON. When absent, no filter.
    /// </summary>
    public byte[]? Filter
    {
        get => FilterStorage?.ToArray();
        init => FilterStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? FilterStorage { get; init; }
    public static RenderedFeatureQueryOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyRenderedFeatureQueryOptions(
                NativeMethods.mln_rendered_feature_query_options_default()
            );
        }
    }
}
