using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal;

namespace Maplibre.NativeFfi.Query;

public sealed class FeatureStateSelector
{
    public required string SourceId { get; set; }
    public string? SourceLayerId { get; set; }
    public string? FeatureId { get; set; }
    public string? StateKey { get; set; }
}

/// <summary>
/// One copied query hit. <see cref="Feature" /> is a UTF-8 GeoJSON Feature.
/// <see cref="State" /> is a UTF-8 JSON object when present.
/// </summary>
public sealed record QueriedFeature
{
    private readonly byte[] feature;
    private readonly byte[]? state;

    public QueriedFeature(byte[] Feature, string? SourceId, string? SourceLayerId, byte[]? State)
    {
        ArgumentNullException.ThrowIfNull(Feature);
        feature = (byte[])Feature.Clone();
        this.SourceId = SourceId;
        this.SourceLayerId = SourceLayerId;
        state = State?.ToArray();
    }

    public byte[] Feature => (byte[])feature.Clone();
    public string? SourceId { get; }
    public string? SourceLayerId { get; }
    public byte[]? State => state?.ToArray();

    public bool Equals(QueriedFeature? other) =>
        other is not null
        && feature.AsSpan().SequenceEqual(other.feature)
        && SourceId == other.SourceId
        && SourceLayerId == other.SourceLayerId
        && ValueEquality.SequenceEquals(state, other.state);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.AddBytes(feature);
        hash.Add(SourceId);
        hash.Add(SourceLayerId);
        hash.Add(ValueEquality.SequenceHashCode(state));
        return hash.ToHashCode();
    }
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
/// Compares and hashes by property value, comparing <see cref="LayerIds"/> element by element.
/// Assigning <see cref="LayerIds"/> snapshots the caller's list. Keep an instance unmodified
/// while it is a key in a hash-based collection.
/// </remarks>
public sealed record RenderedFeatureQueryOptions
{
    private IReadOnlyList<string>? layerIds;

    public IReadOnlyList<string>? LayerIds
    {
        get => layerIds;
        set => layerIds = value is null ? null : Array.AsReadOnly([.. value]);
    }

    private byte[]? filter;

    public byte[]? Filter
    {
        get => filter?.ToArray();
        set => filter = value?.ToArray();
    }

    public bool Equals(RenderedFeatureQueryOptions? other) =>
        other is not null
        && ValueEquality.SequenceEquals(LayerIds, other.LayerIds)
        && ValueEquality.SequenceEquals(filter, other.filter);

    public override int GetHashCode() =>
        HashCode.Combine(
            ValueEquality.SequenceHashCode(LayerIds),
            ValueEquality.SequenceHashCode(filter)
        );
}

/// <remarks>
/// Compares and hashes by property value, comparing <see cref="SourceLayerIds"/> element by
/// element. Assigning <see cref="SourceLayerIds"/> snapshots the caller's list. Keep an instance
/// unmodified while it is a key in a hash-based collection.
/// </remarks>
public sealed record SourceFeatureQueryOptions
{
    private IReadOnlyList<string>? sourceLayerIds;

    public IReadOnlyList<string>? SourceLayerIds
    {
        get => sourceLayerIds;
        set => sourceLayerIds = value is null ? null : Array.AsReadOnly([.. value]);
    }

    private byte[]? filter;

    public byte[]? Filter
    {
        get => filter?.ToArray();
        set => filter = value?.ToArray();
    }

    public bool Equals(SourceFeatureQueryOptions? other) =>
        other is not null
        && ValueEquality.SequenceEquals(SourceLayerIds, other.SourceLayerIds)
        && ValueEquality.SequenceEquals(filter, other.filter);

    public override int GetHashCode() =>
        HashCode.Combine(
            ValueEquality.SequenceHashCode(SourceLayerIds),
            ValueEquality.SequenceHashCode(filter)
        );
}
