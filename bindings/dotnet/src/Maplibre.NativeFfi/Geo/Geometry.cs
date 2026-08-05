using Maplibre.NativeFfi.Internal;

namespace Maplibre.NativeFfi.Geo;

/// <summary>Immutable geometry tree used by Maplibre descriptors and copied results.</summary>
/// <remarks>
/// Coordinate lists compare element by element. Construction and <c>with</c> snapshot the caller's
/// lists at every level.
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
        private readonly IReadOnlyList<LatLng> coordinates = ValueEquality.Snapshot(Coordinates);

        public IReadOnlyList<LatLng> Coordinates
        {
            get => coordinates;
            init => coordinates = ValueEquality.Snapshot(value);
        }

        public bool Equals(LineString? other) =>
            other is not null && ValueEquality.SequenceEquals(coordinates, other.coordinates);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(coordinates);
    }

    public sealed record Polygon(IReadOnlyList<IReadOnlyList<LatLng>> Rings) : Geometry
    {
        private readonly IReadOnlyList<IReadOnlyList<LatLng>> rings = ValueEquality.NestedSnapshot(
            Rings
        );

        public IReadOnlyList<IReadOnlyList<LatLng>> Rings
        {
            get => rings;
            init => rings = ValueEquality.NestedSnapshot(value);
        }

        public bool Equals(Polygon? other) =>
            other is not null && ValueEquality.NestedSequenceEquals(rings, other.rings);

        public override int GetHashCode() => ValueEquality.NestedSequenceHashCode(rings);
    }

    public sealed record MultiPoint(IReadOnlyList<LatLng> Coordinates) : Geometry
    {
        private readonly IReadOnlyList<LatLng> coordinates = ValueEquality.Snapshot(Coordinates);

        public IReadOnlyList<LatLng> Coordinates
        {
            get => coordinates;
            init => coordinates = ValueEquality.Snapshot(value);
        }

        public bool Equals(MultiPoint? other) =>
            other is not null && ValueEquality.SequenceEquals(coordinates, other.coordinates);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(coordinates);
    }

    public sealed record MultiLineString(IReadOnlyList<IReadOnlyList<LatLng>> Lines) : Geometry
    {
        private readonly IReadOnlyList<IReadOnlyList<LatLng>> lines = ValueEquality.NestedSnapshot(
            Lines
        );

        public IReadOnlyList<IReadOnlyList<LatLng>> Lines
        {
            get => lines;
            init => lines = ValueEquality.NestedSnapshot(value);
        }

        public bool Equals(MultiLineString? other) =>
            other is not null && ValueEquality.NestedSequenceEquals(lines, other.lines);

        public override int GetHashCode() => ValueEquality.NestedSequenceHashCode(lines);
    }

    public sealed record MultiPolygon(IReadOnlyList<IReadOnlyList<IReadOnlyList<LatLng>>> Polygons)
        : Geometry
    {
        private readonly IReadOnlyList<IReadOnlyList<IReadOnlyList<LatLng>>> polygons =
            ValueEquality.DeepNestedSnapshot(Polygons);

        public IReadOnlyList<IReadOnlyList<IReadOnlyList<LatLng>>> Polygons
        {
            get => polygons;
            init => polygons = ValueEquality.DeepNestedSnapshot(value);
        }

        public bool Equals(MultiPolygon? other) =>
            other is not null
            && ValueEquality.SequenceEquals(
                polygons,
                other.polygons,
                static (left, right) => ValueEquality.NestedSequenceEquals(left, right)
            );

        public override int GetHashCode() =>
            ValueEquality.SequenceHashCode(
                polygons,
                static value => ValueEquality.NestedSequenceHashCode(value)
            );
    }

    public sealed record Collection(IReadOnlyList<Geometry> Geometries) : Geometry
    {
        private readonly IReadOnlyList<Geometry> geometries = ValueEquality.Snapshot(Geometries);

        public IReadOnlyList<Geometry> Geometries
        {
            get => geometries;
            init => geometries = ValueEquality.Snapshot(value);
        }

        public bool Equals(Collection? other) =>
            other is not null && ValueEquality.SequenceEquals(geometries, other.geometries);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(geometries);
    }
}
