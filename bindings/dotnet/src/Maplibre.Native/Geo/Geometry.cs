using Maplibre.Native.Internal;

namespace Maplibre.Native.Geo;

/// <summary>Immutable geometry tree used by Maplibre descriptors and copied results.</summary>
/// <remarks>
/// Coordinate lists compare element by element, so geometries built from distinct list instances
/// holding the same coordinates compare equal.
/// </remarks>
public abstract record Geometry
{
    public const int MaxCollectionDepth = 64;

    private Geometry() { }

    public sealed record Empty : Geometry
    {
        public static Empty Instance { get; } = new();

        private Empty() { }
    }

    public sealed record Point(LatLng Coordinate) : Geometry;

    public sealed record LineString(IReadOnlyList<LatLng> Coordinates) : Geometry
    {
        public bool Equals(LineString? other) =>
            other is not null && ValueEquality.SequenceEquals(Coordinates, other.Coordinates);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Coordinates);
    }

    public sealed record Polygon(IReadOnlyList<IReadOnlyList<LatLng>> Rings) : Geometry
    {
        public bool Equals(Polygon? other) =>
            other is not null && ValueEquality.NestedSequenceEquals(Rings, other.Rings);

        public override int GetHashCode() => ValueEquality.NestedSequenceHashCode(Rings);
    }

    public sealed record MultiPoint(IReadOnlyList<LatLng> Coordinates) : Geometry
    {
        public bool Equals(MultiPoint? other) =>
            other is not null && ValueEquality.SequenceEquals(Coordinates, other.Coordinates);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Coordinates);
    }

    public sealed record MultiLineString(IReadOnlyList<IReadOnlyList<LatLng>> Lines) : Geometry
    {
        public bool Equals(MultiLineString? other) =>
            other is not null && ValueEquality.NestedSequenceEquals(Lines, other.Lines);

        public override int GetHashCode() => ValueEquality.NestedSequenceHashCode(Lines);
    }

    public sealed record MultiPolygon(IReadOnlyList<IReadOnlyList<IReadOnlyList<LatLng>>> Polygons)
        : Geometry
    {
        public bool Equals(MultiPolygon? other) =>
            other is not null
            && ValueEquality.SequenceEquals(
                Polygons,
                other.Polygons,
                static (left, right) => ValueEquality.NestedSequenceEquals(left, right)
            );

        public override int GetHashCode() =>
            ValueEquality.SequenceHashCode(
                Polygons,
                static value => ValueEquality.NestedSequenceHashCode(value)
            );
    }

    public sealed record Collection(IReadOnlyList<Geometry> Geometries) : Geometry
    {
        public bool Equals(Collection? other) =>
            other is not null && ValueEquality.SequenceEquals(Geometries, other.Geometries);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Geometries);
    }
}
