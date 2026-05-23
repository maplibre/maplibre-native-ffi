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
    [Fact]
    public void NativeJsonValueMaterializesNestedObjectsAndArrays()
    {
        using var native = NativeJsonValue.From(new JsonValue.Object([
            new JsonMember("type", new JsonValue.String("geojson")),
            new JsonMember("data", new JsonValue.Object([
                new JsonMember("type", new JsonValue.String("FeatureCollection")),
                new JsonMember("features", new JsonValue.Array([])),
            ])),
        ]));

        Assert.Equal((uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_OBJECT, native.Pointer->type);
        Assert.Equal(2u, native.Pointer->data.object_value.member_count);
        var first = native.Pointer->data.object_value.members[0];
        Assert.Equal("type", Marshal.PtrToStringUTF8((nint)first.key.data, (int)first.key.size));
        Assert.Equal((uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_STRING, first.value->type);
    }

    [Fact]
    public void NativeJsonValueRejectsNonFiniteNumbersBeforeNativeCall()
    {
        var error = Assert.Throws<InvalidArgumentException>(() => NativeJsonValue.From(new JsonValue.Double(double.NaN)));
        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
    }

    [Fact]
    public void LayerJsonPropertiesAndFiltersAdaptThroughNativeMap()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        map.AddStyleSourceJson("geo", GeoJsonSource());
        map.AddStyleLayerJson(new JsonValue.Object([
            new JsonMember("id", new JsonValue.String("fill")),
            new JsonMember("type", new JsonValue.String("fill")),
            new JsonMember("source", new JsonValue.String("geo")),
        ]));

        map.SetLayerProperty("fill", "fill-opacity", new JsonValue.Double(0.5));
        map.SetLayerFilter("fill", new JsonValue.Array([
            new JsonValue.String("=="),
            new JsonValue.String("kind"),
            new JsonValue.String("park"),
        ]));

        Assert.Equal(new JsonValue.Double(0.5), map.GetLayerProperty("fill", "fill-opacity"));
        Assert.IsType<JsonValue.Object>(map.GetStyleLayerJson("fill"));
        Assert.IsType<JsonValue.Array>(map.GetLayerFilter("fill"));

        map.SetLayerFilter("fill", null);
        Assert.Null(map.GetLayerFilter("fill"));
    }

    [Fact]
    public void StyleSourceAndLayerJsonAdaptThroughNativeMap()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
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

        map.AddStyleLayerJson(new JsonValue.Object([
            new JsonMember("id", new JsonValue.String("background")),
            new JsonMember("type", new JsonValue.String("background")),
        ]));
        Assert.True(map.StyleLayerExists("background"));
        Assert.Equal("background", map.StyleLayerType("background"));
        Assert.Contains("background", map.StyleLayerIds());

        Assert.True(map.RemoveStyleLayer("background"));
        Assert.True(map.RemoveStyleSource("geo"));
    }

    private static JsonValue GeoJsonSource() => new JsonValue.Object([
        new JsonMember("type", new JsonValue.String("geojson")),
        new JsonMember("data", new JsonValue.Object([
            new JsonMember("type", new JsonValue.String("FeatureCollection")),
            new JsonMember("features", new JsonValue.Array([])),
        ])),
    ]);
}
