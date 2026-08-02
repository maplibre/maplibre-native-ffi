using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal;
using Maplibre.NativeFfi.Json;

namespace Maplibre.NativeFfi.Query;

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
        private readonly IReadOnlyList<ScreenPoint> points = ValueEquality.Snapshot(Points);

        public IReadOnlyList<ScreenPoint> Points
        {
            get => points;
            init => points = ValueEquality.Snapshot(value);
        }

        public bool Equals(LineString? other) =>
            other is not null && ValueEquality.SequenceEquals(points, other.points);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(points);
    }
}

/// <remarks>
/// Compares and hashes by property value, comparing <see cref="LayerIds"/> element by element;
/// <c>with</c> returns an independent instance. Assigning <see cref="LayerIds"/> snapshots the
/// caller's list, so later caller mutation does not change this descriptor. Keep an instance
/// unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record RenderedFeatureQueryOptions
{
    private IReadOnlyList<string>? layerIds;

    public IReadOnlyList<string>? LayerIds
    {
        get => layerIds;
        set => layerIds = value is null ? null : Array.AsReadOnly([.. value]);
    }

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
/// element; <c>with</c> returns an independent instance. Assigning <see cref="SourceLayerIds"/>
/// snapshots the caller's list, so later caller mutation does not change this descriptor. Keep an
/// instance unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record SourceFeatureQueryOptions
{
    private IReadOnlyList<string>? sourceLayerIds;

    public IReadOnlyList<string>? SourceLayerIds
    {
        get => sourceLayerIds;
        set => sourceLayerIds = value is null ? null : Array.AsReadOnly([.. value]);
    }

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
        private readonly IReadOnlyList<Feature> features = ValueEquality.Snapshot(Features);

        public IReadOnlyList<Feature> Features
        {
            get => features;
            init => features = ValueEquality.Snapshot(value);
        }

        public bool Equals(FeatureCollection? other) =>
            other is not null && ValueEquality.SequenceEquals(features, other.features);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(features);
    }

    public sealed record Unknown(uint RawType) : FeatureExtensionResult;
}
