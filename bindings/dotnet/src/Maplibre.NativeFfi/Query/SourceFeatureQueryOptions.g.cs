// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Query;

public sealed record SourceFeatureQueryOptions
{
    private string[]? storageSourceLayerIds;
    public string[]? SourceLayerIds
    {
        get => storageSourceLayerIds?.ToArray();
        set => storageSourceLayerIds = value?.ToArray();
    }
    internal string[]? SourceLayerIdsStorage
    {
        get => storageSourceLayerIds;
        init => storageSourceLayerIds = value;
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

    public bool Equals(SourceFeatureQueryOptions? other) =>
        other is not null
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            SourceLayerIdsStorage,
            other.SourceLayerIdsStorage
        )
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            FilterStorage,
            other.FilterStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(
                SourceLayerIdsStorage
            )
        );
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(FilterStorage));
        return hash.ToHashCode();
    }

    public static SourceFeatureQueryOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_source_feature_query_options_default");
            return CopySourceFeatureQueryOptions(
                NativeMethods.mln_source_feature_query_options_default()
            );
        }
    }
}
