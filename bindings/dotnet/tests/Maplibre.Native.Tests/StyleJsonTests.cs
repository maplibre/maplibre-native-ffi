using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class StyleJsonTests
{
    [BindingSpecTest("BND-064")]
    [Fact]
    public void NativeJsonValueMaterializesNestedObjectsAndArrays()
    {
        using var native = NativeJsonValue.From(
            new JsonValue.Object([
                new JsonMember("type", new JsonValue.String("geojson")),
                new JsonMember(
                    "data",
                    new JsonValue.Object([
                        new JsonMember("type", new JsonValue.String("FeatureCollection")),
                        new JsonMember("features", new JsonValue.Array([])),
                    ])
                ),
            ])
        );

        Assert.Equal((uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_OBJECT, native.Pointer->type);
        Assert.Equal(2u, native.Pointer->data.object_value.member_count);
        var first = native.Pointer->data.object_value.members[0];
        Assert.Equal("type", Marshal.PtrToStringUTF8((nint)first.key.data, (int)first.key.size));
        Assert.Equal((uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_STRING, first.value->type);
    }

    [BindingSpecTest("BND-025")]
    [Fact]
    public void NativeJsonValueRejectsNonFiniteNumbersBeforeNativeCall()
    {
        var error = Assert.Throws<InvalidArgumentException>(() =>
            NativeJsonValue.From(new JsonValue.Double(double.NaN))
        );
        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
    }

    // Support invariant for BND-064: malformed native JSON containers fail
    // deterministically instead of fabricating public JSON values.
    [Fact]
    public void JsonReaderRejectsNonZeroCountsWithNullBackingPointers()
    {
        var arrayValue = new mln_json_value
        {
            type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_ARRAY,
            data =
            {
                array_value = new mln_json_array { value_count = 1, values = null },
            },
        };
        var arrayError = Assert.Throws<InvalidOperationException>(() =>
            ReadJsonValueForTest(arrayValue)
        );
        Assert.Contains("mln_json_array", arrayError.Message, StringComparison.Ordinal);

        var objectValue = new mln_json_value
        {
            type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_OBJECT,
            data =
            {
                object_value = new mln_json_object { member_count = 1, members = null },
            },
        };
        var objectError = Assert.Throws<InvalidOperationException>(() =>
            ReadJsonValueForTest(objectValue)
        );
        Assert.Contains("mln_json_object", objectError.Message, StringComparison.Ordinal);
    }

    // Support invariant for BND-064: unknown native JSON tags fail
    // deterministically instead of fabricating public JSON values.
    [Fact]
    public void JsonReaderRejectsUnknownNativeValueTypes()
    {
        var value = new mln_json_value { type = uint.MaxValue };

        var error = Assert.Throws<InvalidOperationException>(() => ReadJsonValueForTest(value));

        Assert.Contains("Unknown native JSON value type", error.Message, StringComparison.Ordinal);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void UrlAndTileSourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");

        map.AddGeoJsonSourceUrl("geo-url", "https://example.test/data.geojson");
        map.SetGeoJsonSourceUrl("geo-url", "https://example.test/other.geojson");
        map.AddVectorSourceTiles(
            "vector-tiles",
            ["https://example.test/vector/{z}/{x}/{y}.pbf"],
            new TileSourceOptions
            {
                MinimumZoom = 1,
                MaximumZoom = 12,
                Attribution = "Vector attribution",
                Scheme = TileScheme.Xyz,
                VectorEncoding = VectorTileEncoding.Mvt,
            }
        );
        map.AddRasterSourceTiles(
            "raster-tiles",
            ["https://example.test/raster/{z}/{x}/{y}.png"],
            new TileSourceOptions { TileSize = 256 }
        );
        map.AddRasterDemSourceTiles(
            "dem-tiles",
            ["https://example.test/dem/{z}/{x}/{y}.png"],
            new TileSourceOptions { RasterEncoding = RasterDemEncoding.Mapbox }
        );

        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo-url"));
        Assert.Equal(SourceType.Vector, map.StyleSourceType("vector-tiles"));
        Assert.Equal(SourceType.Raster, map.StyleSourceType("raster-tiles"));
        Assert.Equal(SourceType.RasterDem, map.StyleSourceType("dem-tiles"));
        Assert.Equal("Vector attribution", map.StyleSourceInfo("vector-tiles")?.Attribution);
    }

    [BindingSpecTest("BND-081", "BND-101")]
    [Fact]
    public void SetStyleJsonReturnsCopiedStyleLoadedEventWithMapIdentity()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        var runtimeEvent = RuntimeEventTestHelpers.WaitForMapEvent(
            runtime,
            map,
            RuntimeEventType.MapStyleLoaded
        );

        Assert.Equal(RuntimeEventType.MapStyleLoaded, runtimeEvent.Type);
        Assert.Equal((uint)RuntimeEventType.MapStyleLoaded, runtimeEvent.RawType);
        Assert.Equal(RuntimeEventSourceType.Map, runtimeEvent.SourceType);
        Assert.Same(map, runtimeEvent.MapSource);
        Assert.Null(runtimeEvent.RuntimeSource);
        Assert.Same(RuntimeEventPayload.None.Instance, runtimeEvent.Payload);
        Assert.Same(map, runtimeEvent.MapSource);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void LayerJsonPropertiesAndFiltersAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        map.AddStyleSourceJson("geo", GeoJsonSource());
        map.AddStyleLayerJson(
            new JsonValue.Object([
                new JsonMember("id", new JsonValue.String("fill")),
                new JsonMember("type", new JsonValue.String("fill")),
                new JsonMember("source", new JsonValue.String("geo")),
            ]),
            ""
        );

        map.SetLayerProperty("fill", "fill-opacity", new JsonValue.Double(0.5));
        map.SetLayerFilter(
            "fill",
            new JsonValue.Array([
                new JsonValue.String("=="),
                new JsonValue.String("kind"),
                new JsonValue.String("park"),
            ])
        );

        Assert.Equal(new JsonValue.Double(0.5), map.GetLayerProperty("fill", "fill-opacity"));
        Assert.IsType<JsonValue.Object>(map.GetStyleLayerJson("fill"));
        Assert.IsType<JsonValue.Array>(map.GetLayerFilter("fill"));

        map.SetLayerFilter("fill", null);
        Assert.Null(map.GetLayerFilter("fill"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void StyleSourceAndLayerJsonAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");

        map.AddStyleSourceJson("geo", GeoJsonSource());
        Assert.True(map.StyleSourceExists("geo"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo"));
        Assert.Contains("geo", map.StyleSourceIds());
        var sourceInfo = map.StyleSourceInfo("geo");
        Assert.NotNull(sourceInfo);
        Assert.Equal("geo", sourceInfo.Id);
        Assert.Equal(SourceType.GeoJson, sourceInfo.Type);
        Assert.Null(sourceInfo.Attribution);

        map.AddStyleLayerJson(
            new JsonValue.Object([
                new JsonMember("id", new JsonValue.String("background")),
                new JsonMember("type", new JsonValue.String("background")),
            ]),
            ""
        );
        Assert.True(map.StyleLayerExists("background"));
        Assert.Equal("background", map.StyleLayerType("background"));
        Assert.Contains("background", map.StyleLayerIds());

        Assert.True(map.RemoveStyleLayer("background"));
        Assert.True(map.RemoveStyleSource("geo"));
    }

    private static JsonValue GeoJsonSource() =>
        new JsonValue.Object([
            new JsonMember("type", new JsonValue.String("geojson")),
            new JsonMember(
                "data",
                new JsonValue.Object([
                    new JsonMember("type", new JsonValue.String("FeatureCollection")),
                    new JsonMember("features", new JsonValue.Array([])),
                ])
            ),
        ]);

    private static void ReadJsonValueForTest(mln_json_value value)
    {
        ValueStructs.ReadJsonValue(&value);
    }
}
