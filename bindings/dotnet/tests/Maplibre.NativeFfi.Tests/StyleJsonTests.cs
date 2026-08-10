using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class StyleJsonTests
{
    [BindingSpecTest("BND-105")]
    [Fact]
    public void UrlAndTileSourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson(EmptyStyle());

        map.AddGeoJsonSourceUrl("geo-url", "https://example.test/data.geojson", null);
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
        Assert.Equal("https://example.test/other.geojson", map.StyleSourceInfo("geo-url")?.Url);
        Assert.Equal("Vector attribution", map.StyleSourceInfo("vector-tiles")?.Attribution);
        Assert.Equal(256u, map.StyleSourceInfo("raster-tiles")?.TileSize);
        Assert.Equal(RasterDemEncoding.Mapbox, map.StyleSourceInfo("dem-tiles")?.RasterDemEncoding);
        Assert.Equal(
            (uint)RasterDemEncoding.Mapbox,
            map.StyleSourceInfo("dem-tiles")?.RawRasterDemEncoding
        );
    }

    [BindingSpecTest("BND-109")]
    [Fact]
    public void SourceInspectionCopiesUrlAndInlineTileJsonAfterSourceRelease()
    {
        SourceInfo urlInfo;
        SourceInfo inlineInfo;
        var bounds = new LatLngBounds(new LatLng(-40, -120), new LatLng(40, 120));
        var tileUrls = new[]
        {
            "https://example.test/vector-a/{z}/{x}/{y}.pbf",
            "https://example.test/vector-b/{z}/{x}/{y}.pbf",
        };

        using (var runtime = RuntimeHandle.Create(new RuntimeOptions()))
        using (var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 }))
        {
            map.SetStyleJson(EmptyStyle());
            map.AddVectorSourceUrl("url-vector", "https://example.test/vector.json", null);
            map.AddVectorSourceTiles(
                "inline-vector",
                tileUrls,
                new TileSourceOptions
                {
                    MinimumZoom = 0,
                    MaximumZoom = 12,
                    Attribution = "Inline attribution",
                    Scheme = TileScheme.Tms,
                    VectorEncoding = VectorTileEncoding.Mlt,
                    Bounds = bounds,
                }
            );

            urlInfo = Assert.IsType<SourceInfo>(map.StyleSourceInfo("url-vector"));
            inlineInfo = Assert.IsType<SourceInfo>(map.StyleSourceInfo("inline-vector"));

            Assert.Equal("https://example.test/vector.json", urlInfo.Url);
            Assert.Null(urlInfo.TileJson);
            Assert.Null(urlInfo.Attribution);

            Assert.Null(inlineInfo.Url);
            Assert.Equal("Inline attribution", inlineInfo.Attribution);
            var tileJson = Assert.IsType<TileJson>(inlineInfo.TileJson);
            Assert.Equal(tileUrls, tileJson.TileUrls);
            Assert.Equal(0, tileJson.MinimumZoom);
            Assert.Equal(12, tileJson.MaximumZoom);
            Assert.Equal(TileScheme.Tms, tileJson.Scheme);
            Assert.Equal((uint)TileScheme.Tms, tileJson.RawScheme);
            Assert.Equal(bounds, tileJson.Bounds);
            Assert.Equal(512u, inlineInfo.TileSize);
            Assert.Equal(VectorTileEncoding.Mlt, inlineInfo.VectorEncoding);
            Assert.Equal((uint)VectorTileEncoding.Mlt, inlineInfo.RawVectorEncoding);
            Assert.Null(inlineInfo.RasterDemEncoding);
            Assert.Null(inlineInfo.RawRasterDemEncoding);

            Assert.True(map.RemoveStyleSource("url-vector"));
            Assert.True(map.RemoveStyleSource("inline-vector"));
        }

        Assert.Equal("https://example.test/vector.json", urlInfo.Url);
        Assert.Equal(tileUrls, inlineInfo.TileJson?.TileUrls);
        Assert.Equal(bounds, inlineInfo.TileJson?.Bounds);

        using var rebuiltRuntime = RuntimeHandle.Create(new RuntimeOptions());
        using var rebuiltMap = MapHandle.Create(
            rebuiltRuntime,
            new MapOptions { Width = 512, Height = 512 }
        );
        rebuiltMap.SetStyleJson(EmptyStyle());
        rebuiltMap.AddVectorSourceTiles(
            "rebuilt",
            inlineInfo.TileJson!.TileUrls,
            new TileSourceOptions
            {
                MinimumZoom = inlineInfo.TileJson.MinimumZoom,
                MaximumZoom = inlineInfo.TileJson.MaximumZoom,
                Scheme = inlineInfo.TileJson.Scheme,
                Bounds = inlineInfo.TileJson.Bounds,
                TileSize = inlineInfo.TileSize,
                Attribution = inlineInfo.Attribution,
                VectorEncoding = inlineInfo.VectorEncoding,
            }
        );
        Assert.True(rebuiltMap.StyleSourceExists("rebuilt"));
    }

    [BindingSpecTest("BND-101")]
    [Fact]
    public void LoadedStyleDocumentAndUrlReadBackWhatWasLoaded()
    {
        var styleJson = EmptyStyle();
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        // Nothing parsed and nothing requested yet.
        Assert.Empty(map.GetLoadedStyleJson());
        Assert.Equal(string.Empty, map.GetStyleUrl());

        // The document reads back byte-for-byte, so it can be reloaded unchanged.
        map.SetStyleJson(styleJson);
        Assert.Equal(styleJson, map.GetLoadedStyleJson());
        // Inline JSON clears the URL.
        Assert.Equal(string.Empty, map.GetStyleUrl());

        // The URL is request state, recorded before the load can succeed, while the
        // document still reports the style that last parsed.
        map.SetStyleUrl("https://example.test/style.json");
        Assert.Equal("https://example.test/style.json", map.GetStyleUrl());
        Assert.Equal(styleJson, map.GetLoadedStyleJson());
    }

    [BindingSpecTest("BND-081", "BND-101")]
    [Fact]
    public void SetStyleJsonReturnsCopiedStyleLoadedEventWithMapIdentity()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleJson(EmptyStyle());
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
        map.SetStyleJson(EmptyStyle());
        map.AddStyleSourceJson("geo", GeoJsonSource());
        map.AddStyleLayerJson("""{"id":"fill","type":"fill","source":"geo"}"""u8.ToArray(), "");

        map.SetLayerProperty("fill", "fill-opacity", "0.5"u8.ToArray());
        map.SetLayerFilter("fill", """["==","kind","park"]"""u8.ToArray());

        Assert.Equal("0.5"u8.ToArray(), map.GetLayerProperty("fill", "fill-opacity"));
        Assert.NotEmpty(Assert.IsType<byte[]>(map.GetStyleLayerJson("fill")));
        Assert.Equal("""["==","kind","park"]"""u8.ToArray(), map.GetLayerFilter("fill"));

        map.SetLayerFilter("fill", null);
        Assert.Null(map.GetLayerFilter("fill"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void StyleSourceAndLayerJsonAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson(EmptyStyle());

        map.AddStyleSourceJson("geo", GeoJsonSource());
        Assert.True(map.StyleSourceExists("geo"));
        Assert.Equal(SourceType.GeoJson, map.StyleSourceType("geo"));
        Assert.Contains("geo", map.StyleSourceIds());
        var sourceInfo = map.StyleSourceInfo("geo");
        Assert.NotNull(sourceInfo);
        Assert.Equal("geo", sourceInfo.Id);
        Assert.Equal(SourceType.GeoJson, sourceInfo.Type);
        Assert.Null(sourceInfo.Attribution);

        map.AddStyleLayerJson("""{"id":"background","type":"background"}"""u8.ToArray(), "");
        Assert.True(map.StyleLayerExists("background"));
        Assert.Equal("background", map.StyleLayerType("background"));
        Assert.Contains("background", map.StyleLayerIds());

        Assert.True(map.RemoveStyleLayer("background"));
        Assert.True(map.RemoveStyleSource("geo"));
    }

    private static byte[] EmptyStyle() => """{"version":8,"sources":{},"layers":[]}"""u8.ToArray();

    private static byte[] GeoJsonSource() =>
        """{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}"""u8.ToArray();
}
