namespace Maplibre.Native.Geo;

/// <summary>Immutable geometry tree used by Maplibre descriptors and copied results.</summary>
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

    public sealed record LineString(IReadOnlyList<LatLng> Coordinates) : Geometry;

    public sealed record Polygon(IReadOnlyList<IReadOnlyList<LatLng>> Rings) : Geometry;

    public sealed record MultiPoint(IReadOnlyList<LatLng> Coordinates) : Geometry;

    public sealed record MultiLineString(IReadOnlyList<IReadOnlyList<LatLng>> Lines) : Geometry;

    public sealed record MultiPolygon(IReadOnlyList<IReadOnlyList<IReadOnlyList<LatLng>>> Polygons)
        : Geometry;

    public sealed record Collection(IReadOnlyList<Geometry> Geometries) : Geometry;
}
