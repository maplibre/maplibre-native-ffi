using Maplibre.Native.Internal;
using Maplibre.Native.Json;

namespace Maplibre.Native.Geo;

/// <summary>Feature identifier value.</summary>
public abstract record FeatureIdentifier
{
    private FeatureIdentifier() { }

    public sealed record Null : FeatureIdentifier
    {
        public static Null Instance { get; } = new();

        private Null() { }
    }

    public sealed record UInt(ulong Value) : FeatureIdentifier;

    public sealed record Int(long Value) : FeatureIdentifier;

    public sealed record Double(double Value) : FeatureIdentifier;

    public sealed record String(string Value) : FeatureIdentifier;
}

/// <summary>GeoJSON feature value.</summary>
/// <remarks>
/// <see cref="Properties"/> compares member by member, preserving order and repeated member names.
/// </remarks>
public sealed record Feature(
    Geometry Geometry,
    IReadOnlyList<JsonMember> Properties,
    FeatureIdentifier Identifier
)
{
    public bool Equals(Feature? other) =>
        other is not null
        && Equals(Geometry, other.Geometry)
        && ValueEquality.SequenceEquals(Properties, other.Properties)
        && Equals(Identifier, other.Identifier);

    public override int GetHashCode() =>
        HashCode.Combine(Geometry, ValueEquality.SequenceHashCode(Properties), Identifier);
}

/// <summary>GeoJSON value.</summary>
public abstract record GeoJson
{
    private GeoJson() { }

    public sealed record GeometryValue(Geometry Geometry) : GeoJson;

    public sealed record FeatureValue(Feature Feature) : GeoJson;

    public sealed record FeatureCollection(IReadOnlyList<Feature> Features) : GeoJson
    {
        public bool Equals(FeatureCollection? other) =>
            other is not null && ValueEquality.SequenceEquals(Features, other.Features);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Features);
    }
}
