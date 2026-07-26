using Maplibre.Native.Geo;
using Maplibre.Native.Json;
using Maplibre.Native.Query;
using Xunit;

namespace Maplibre.Native.Tests;

/// <summary>
/// BND-069: public value trees snapshot caller-owned lists, so later caller mutation leaves the
/// stored value, its equality, and its hash code unchanged. Matches the Kotlin binding, whose
/// container constructors copy at every level.
/// </summary>
public sealed class ValueSnapshotTests
{
    [BindingSpecTest("BND-069")]
    [Fact]
    public void FlatGeometriesSnapshotCallerLists()
    {
        var coordinates = new List<LatLng> { new(0, 0) };
        var line = new Geometry.LineString(coordinates);
        var multiPoint = new Geometry.MultiPoint(coordinates);
        var expectedHash = line.GetHashCode();

        coordinates.Add(new LatLng(9, 9));

        Assert.Single(line.Coordinates);
        Assert.Single(multiPoint.Coordinates);
        Assert.Equal(expectedHash, line.GetHashCode());
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void NestedGeometriesSnapshotInnerLists()
    {
        var ring = new List<LatLng> { new(0, 0) };
        var rings = new List<IReadOnlyList<LatLng>> { ring };
        var polygon = new Geometry.Polygon(rings);
        var multiLine = new Geometry.MultiLineString(rings);

        // Mutating either level must not reach the stored geometry.
        ring.Add(new LatLng(9, 9));
        rings.Add(new List<LatLng> { new(5, 5) });

        Assert.Single(polygon.Rings);
        Assert.Single(polygon.Rings[0]);
        Assert.Single(multiLine.Lines);
        Assert.Single(multiLine.Lines[0]);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void MultiPolygonSnapshotsAllThreeLevels()
    {
        var ring = new List<LatLng> { new(0, 0) };
        var rings = new List<IReadOnlyList<LatLng>> { ring };
        var polygons = new List<IReadOnlyList<IReadOnlyList<LatLng>>> { rings };
        var multiPolygon = new Geometry.MultiPolygon(polygons);

        ring.Add(new LatLng(9, 9));
        rings.Add(new List<LatLng> { new(5, 5) });
        polygons.Add(rings);

        Assert.Single(multiPolygon.Polygons);
        Assert.Single(multiPolygon.Polygons[0]);
        Assert.Single(multiPolygon.Polygons[0][0]);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void GeometryCollectionsSnapshotCallerLists()
    {
        var geometries = new List<Geometry> { Geometry.Empty.Instance };
        var collection = new Geometry.Collection(geometries);

        geometries.Add(new Geometry.Point(new LatLng(1, 1)));

        Assert.Single(collection.Geometries);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void FeaturesAndFeatureCollectionsSnapshotCallerLists()
    {
        var properties = new List<JsonMember> { new("k", new JsonValue.Int(1)) };
        var feature = new Feature(
            Geometry.Empty.Instance,
            properties,
            FeatureIdentifier.Null.Instance
        );

        var features = new List<Feature> { feature };
        var geoJson = new GeoJson.FeatureCollection(features);
        var extension = new FeatureExtensionResult.FeatureCollection(features);

        properties.Add(new JsonMember("other", new JsonValue.Int(2)));
        features.Add(feature);

        Assert.Single(feature.Properties);
        Assert.Single(geoJson.Features);
        Assert.Single(extension.Features);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void JsonContainersSnapshotCallerLists()
    {
        var values = new List<JsonValue> { new JsonValue.Int(1) };
        var array = new JsonValue.Array(values);

        var members = new List<JsonMember> { new("k", new JsonValue.Int(1)) };
        var jsonObject = new JsonValue.Object(members);

        values.Add(new JsonValue.Int(2));
        members.Add(new JsonMember("other", new JsonValue.Int(2)));

        Assert.Single(array.Values);
        Assert.Single(jsonObject.Members);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void RenderedQueryLineStringSnapshotsCallerList()
    {
        var points = new List<ScreenPoint> { new(1, 2) };
        var line = new RenderedQueryGeometry.LineString(points);

        points.Add(new ScreenPoint(3, 4));

        Assert.Single(line.Points);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void WithAlsoSnapshotsTheAssignedList()
    {
        var replacement = new List<LatLng> { new(0, 0) };
        var line = new Geometry.LineString([new LatLng(7, 7)]) with { Coordinates = replacement };

        replacement.Add(new LatLng(9, 9));

        Assert.Single(line.Coordinates);
        Assert.Equal(new LatLng(0, 0), line.Coordinates[0]);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void ExposedListsCannotBeCastBackToMutableStorage()
    {
        var line = new Geometry.LineString([new LatLng(0, 0)]);

        Assert.IsNotType<LatLng[]>(line.Coordinates);
        Assert.IsNotType<List<LatLng>>(line.Coordinates);
    }
}
