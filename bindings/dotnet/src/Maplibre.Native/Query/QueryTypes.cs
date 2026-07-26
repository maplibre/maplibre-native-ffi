using Maplibre.Native.Geo;
using Maplibre.Native.Internal;
using Maplibre.Native.Json;

namespace Maplibre.Native.Query;

public sealed class FeatureStateSelector
{
    public required string SourceId { get; set; }
    public string? SourceLayerId { get; set; }
    public string? FeatureId { get; set; }
    public string? StateKey { get; set; }
}

public abstract record RenderedQueryGeometry
{
    private RenderedQueryGeometry() { }

    public sealed record Point(ScreenPoint Value) : RenderedQueryGeometry;

    public sealed record Box(ScreenBox Value) : RenderedQueryGeometry;

    public sealed record LineString(IReadOnlyList<ScreenPoint> Points) : RenderedQueryGeometry
    {
        public bool Equals(LineString? other) =>
            other is not null && ValueEquality.SequenceEquals(Points, other.Points);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Points);
    }
}

/// <remarks>
/// Compares and hashes by property value, comparing <see cref="LayerIds"/> element by element;
/// <c>with</c> returns an independent instance. Keep an instance unmodified while it is a key in a
/// hash-based collection.
/// </remarks>
public sealed record RenderedFeatureQueryOptions
{
    public IReadOnlyList<string>? LayerIds { get; set; }
    public JsonValue? Filter { get; set; }

    public bool Equals(RenderedFeatureQueryOptions? other) =>
        other is not null
        && ValueEquality.SequenceEquals(LayerIds, other.LayerIds)
        && Equals(Filter, other.Filter);

    public override int GetHashCode() =>
        HashCode.Combine(ValueEquality.SequenceHashCode(LayerIds), Filter);
}

/// <remarks>
/// Compares and hashes by property value, comparing <see cref="SourceLayerIds"/> element by
/// element; <c>with</c> returns an independent instance. Keep an instance unmodified while it is a
/// key in a hash-based collection.
/// </remarks>
public sealed record SourceFeatureQueryOptions
{
    public IReadOnlyList<string>? SourceLayerIds { get; set; }
    public JsonValue? Filter { get; set; }

    public bool Equals(SourceFeatureQueryOptions? other) =>
        other is not null
        && ValueEquality.SequenceEquals(SourceLayerIds, other.SourceLayerIds)
        && Equals(Filter, other.Filter);

    public override int GetHashCode() =>
        HashCode.Combine(ValueEquality.SequenceHashCode(SourceLayerIds), Filter);
}

public sealed record QueriedFeature(
    Feature Feature,
    string? SourceId,
    string? SourceLayerId,
    JsonValue? State
);

public abstract record FeatureExtensionResult
{
    private FeatureExtensionResult() { }

    public sealed record Value(JsonValue Json) : FeatureExtensionResult;

    public sealed record FeatureCollection(IReadOnlyList<Feature> Features) : FeatureExtensionResult
    {
        public bool Equals(FeatureCollection? other) =>
            other is not null && ValueEquality.SequenceEquals(Features, other.Features);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Features);
    }

    public sealed record Unknown(uint RawType) : FeatureExtensionResult;
}
