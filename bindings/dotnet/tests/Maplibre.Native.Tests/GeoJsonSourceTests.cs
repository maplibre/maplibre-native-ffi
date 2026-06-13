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

    [Fact]
    public void GeoJsonSourceDataAdaptsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");

        map.AddGeoJsonSourceData("geo-data", EmptyFeatureCollection());
        map.SetGeoJsonSourceData(
            "geo-data",
            new GeoJson.GeometryValue(new Geometry.Point(new LatLng(1, 2)))
        );

        Assert.True(map.StyleSourceExists("geo-data"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-data"));
    }

    private static GeoJson EmptyFeatureCollection() => new GeoJson.FeatureCollection([]);
}
