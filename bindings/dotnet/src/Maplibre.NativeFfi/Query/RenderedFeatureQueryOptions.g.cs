// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Query;

public sealed record RenderedFeatureQueryOptions
{
    private string[]? storageLayerIds;
    public string[]? LayerIds
    {
        get => storageLayerIds?.ToArray();
        set => storageLayerIds = value?.ToArray();
    }
    internal string[]? LayerIdsStorage
    {
        get => storageLayerIds;
        init => storageLayerIds = value;
    }
    private byte[]? storageFilter;
    public byte[]? Filter
    {
        get => storageFilter?.ToArray();
        set => storageFilter = value?.ToArray();
    }
    internal byte[]? FilterStorage
    {
        get => storageFilter;
        init => storageFilter = value;
    }

    public bool Equals(RenderedFeatureQueryOptions? other) =>
        other is not null
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            LayerIdsStorage,
            other.LayerIdsStorage
        )
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            FilterStorage,
            other.FilterStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(LayerIdsStorage)
        );
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(FilterStorage));
        return hash.ToHashCode();
    }

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
