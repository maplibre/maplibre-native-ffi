using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Json;
using Maplibre.NativeFfi.Query;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// BND-070: public value trees compare element by element rather than by list identity. Each case
/// builds two trees from distinct list instances holding equal contents.
/// </summary>
public sealed class GeometryValueEqualityTests
{
    private static IReadOnlyList<LatLng> Ring(double offset) =>
        [new LatLng(offset, offset), new LatLng(offset + 1, offset + 1)];

    [BindingSpecTest("BND-070")]
    [Fact]
    public void FlatCoordinateGeometriesCompareByValue()
    {
        Assert.Equal(new Geometry.LineString(Ring(0)), new Geometry.LineString(Ring(0)));
        Assert.Equal(
            new Geometry.LineString(Ring(0)).GetHashCode(),
            new Geometry.LineString(Ring(0)).GetHashCode()
        );
        Assert.NotEqual(new Geometry.LineString(Ring(0)), new Geometry.LineString(Ring(5)));

        Assert.Equal(new Geometry.MultiPoint(Ring(0)), new Geometry.MultiPoint(Ring(0)));
        Assert.NotEqual(new Geometry.MultiPoint(Ring(0)), new Geometry.MultiPoint(Ring(5)));
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void NestedCoordinateGeometriesCompareByValue()
    {
        Assert.Equal(
            new Geometry.Polygon([Ring(0), Ring(2)]),
            new Geometry.Polygon([Ring(0), Ring(2)])
        );
        Assert.NotEqual(
            new Geometry.Polygon([Ring(0), Ring(2)]),
            new Geometry.Polygon([Ring(0), Ring(9)])
        );

        Assert.Equal(
            new Geometry.MultiLineString([Ring(0)]),
            new Geometry.MultiLineString([Ring(0)])
        );

        Assert.Equal(
            new Geometry.MultiPolygon([
                [Ring(0), Ring(2)],
            ]),
            new Geometry.MultiPolygon([
                [Ring(0), Ring(2)],
            ])
        );
        Assert.NotEqual(
            new Geometry.MultiPolygon([
                [Ring(0), Ring(2)],
            ]),
            new Geometry.MultiPolygon([
                [Ring(0), Ring(9)],
            ])
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void GeometryCollectionsCompareRecursively()
    {
        Assert.Equal(
            new Geometry.Collection([new Geometry.LineString(Ring(0)), Geometry.Empty.Instance]),
            new Geometry.Collection([new Geometry.LineString(Ring(0)), Geometry.Empty.Instance])
        );
        Assert.NotEqual(
            new Geometry.Collection([new Geometry.LineString(Ring(0))]),
            new Geometry.Collection([new Geometry.LineString(Ring(5))])
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void FeaturesCompareGeometryPropertiesAndIdentifier()
    {
        static Feature Build(long id, long propertyValue) =>
            new(
                new Geometry.LineString(Ring(0)),
                [new JsonMember("k", new JsonValue.Int(propertyValue))],
                new FeatureIdentifier.Int(id)
            );

        Assert.Equal(Build(1, 2), Build(1, 2));
        Assert.Equal(Build(1, 2).GetHashCode(), Build(1, 2).GetHashCode());
        Assert.NotEqual(Build(1, 2), Build(1, 3));
        Assert.NotEqual(Build(1, 2), Build(9, 2));

        Assert.Equal(
            new GeoJson.FeatureCollection([Build(1, 2)]),
            new GeoJson.FeatureCollection([Build(1, 2)])
        );
        Assert.NotEqual(
            new GeoJson.FeatureCollection([Build(1, 2)]),
            new GeoJson.FeatureCollection([Build(1, 3)])
        );

        Assert.Equal(
            new FeatureExtensionResult.FeatureCollection([Build(1, 2)]),
            new FeatureExtensionResult.FeatureCollection([Build(1, 2)])
        );
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void RenderedQueryLineStringComparesByValue()
    {
        IReadOnlyList<ScreenPoint> Points() => [new ScreenPoint(1, 2), new ScreenPoint(3, 4)];

        Assert.Equal(
            new RenderedQueryGeometry.LineString(Points()),
            new RenderedQueryGeometry.LineString(Points())
        );
        Assert.NotEqual(
            new RenderedQueryGeometry.LineString(Points()),
            new RenderedQueryGeometry.LineString([new ScreenPoint(1, 2)])
        );
    }
}
