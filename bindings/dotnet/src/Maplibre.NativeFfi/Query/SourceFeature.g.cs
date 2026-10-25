// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One source-feature query hit borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_source_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct SourceFeature
{
    public SourceFeature(ImmutableArray<byte> Feature, string SourceLayerId)
    {
        this.Feature = Feature;
        this.SourceLayerId = SourceLayerId;
    }

    public ImmutableArray<byte> Feature
    {
        get => FeatureStorage.AsImmutable();
        init => FeatureStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> FeatureStorage { get; init; }
    public string SourceLayerId { get; init; }
}
