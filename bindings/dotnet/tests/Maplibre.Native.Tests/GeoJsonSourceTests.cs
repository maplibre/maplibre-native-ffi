using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class GeoJsonSourceTests
{
    [BindingSpecTest("BND-065")]
    [Fact]
    public void GeoJsonMaterializesFeatureCollectionWithProperties()
    {
        var feature = new Feature(
            new Geometry.Point(new LatLng(1, 2)),
            [new JsonMember("name", new JsonValue.String("point"))],
            new FeatureIdentifier.String("id-1")
        );

        using var geoJson = NativeGeoJson.From(new GeoJson.FeatureCollection([feature]));

        Assert.Equal(
            (uint)mln_geojson_type.MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
            geoJson.Pointer->type
        );
        Assert.Equal(1u, geoJson.Pointer->data.feature_collection.feature_count);
        var nativeFeature = geoJson.Pointer->data.feature_collection.features[0];
        Assert.Equal((uint)mln_geometry_type.MLN_GEOMETRY_TYPE_POINT, nativeFeature.geometry->type);
        Assert.Equal(1u, nativeFeature.property_count);
        Assert.Equal(
            (uint)mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_STRING,
            nativeFeature.identifier_type
        );
    }

    [BindingSpecTest("BND-065", "BND-105")]
    [Fact]
    public void GeoJsonSourceDataAdaptsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");

        map.AddGeoJsonSourceData("geo-data", EmptyFeatureCollection(), null);
        map.SetGeoJsonSourceData(
            "geo-data",
            new GeoJson.GeometryValue(new Geometry.Point(new LatLng(1, 2)))
        );

        Assert.True(map.StyleSourceExists("geo-data"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-data"));
    }

    [BindingSpecTest("BND-060", "BND-105")]
    [Fact]
    public void ClusteredGeoJsonSourceOptionsParseThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");

        map.AddGeoJsonSourceData("clustered", NearbyPoints(), ClusterOptions());

        Assert.True(map.StyleSourceExists("clustered"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("clustered"));

        // The cluster aggregation graph is borrowed for the call and parsed by
        // MapLibre Native, so an unparseable expression fails the add.
        var options = ClusterOptions();
        options.ClusterProperties = new JsonValue.Object([
            new JsonMember("weight_sum", new JsonValue.String("not-an-expression")),
        ]);

        var error = Assert.Throws<InvalidArgumentException>(() =>
            map.AddGeoJsonSourceData("clustered-invalid", NearbyPoints(), options)
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.False(map.StyleSourceExists("clustered-invalid"));
    }

    private static GeoJsonSourceOptions ClusterOptions() =>
        new()
        {
            Cluster = true,
            ClusterRadius = 60,
            ClusterMinimumPoints = 2,
            ClusterMaximumZoom = 17,
            ClusterProperties = new JsonValue.Object([
                new JsonMember(
                    "weight_sum",
                    new JsonValue.Array([
                        new JsonValue.String("+"),
                        new JsonValue.Array([
                            new JsonValue.String("get"),
                            new JsonValue.String("weight"),
                        ]),
                    ])
                ),
            ]),
        };

    private static GeoJson NearbyPoints() =>
        new GeoJson.FeatureCollection([
            NearbyPoint(0.000, 1),
            NearbyPoint(0.001, 2),
            NearbyPoint(0.002, 3),
        ]);

    private static Feature NearbyPoint(double offset, ulong weight) =>
        new(
            new Geometry.Point(new LatLng(offset, offset)),
            [new JsonMember("weight", new JsonValue.UInt(weight))],
            FeatureIdentifier.Null.Instance
        );

    private static GeoJson EmptyFeatureCollection() => new GeoJson.FeatureCollection([]);
}
