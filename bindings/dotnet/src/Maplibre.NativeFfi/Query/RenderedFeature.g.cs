// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One rendered-feature query hit borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_rendered_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct RenderedFeature
{
    public RenderedFeature(
        ImmutableArray<byte> Feature,
        string LayerId,
        string SourceId,
        string SourceLayerId,
        ImmutableArray<byte> State
    )
    {
        this.Feature = Feature;
        this.LayerId = LayerId;
        this.SourceId = SourceId;
        this.SourceLayerId = SourceLayerId;
        this.State = State;
    }

    public ImmutableArray<byte> Feature
    {
        get => FeatureStorage.AsImmutable();
        init => FeatureStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> FeatureStorage { get; init; }
    public string LayerId { get; init; }
    public string SourceId { get; init; }
    public string SourceLayerId { get; init; }
    public ImmutableArray<byte> State
    {
        get => StateStorage.AsImmutable();
        init => StateStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> StateStorage { get; init; }
}
