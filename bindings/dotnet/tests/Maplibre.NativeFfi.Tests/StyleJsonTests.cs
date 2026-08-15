using Maplibre.NativeFfi.Error;
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
    public async Task UrlAndTileSourceApisAdaptThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
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

        Assert.Equal(SourceType.GeoJson, (await map.StyleSourceInfoAsync("geo-url"))?.Type);
        Assert.Equal(SourceType.Vector, (await map.StyleSourceInfoAsync("vector-tiles"))?.Type);
        Assert.Equal(SourceType.Raster, (await map.StyleSourceInfoAsync("raster-tiles"))?.Type);
        Assert.Equal(SourceType.RasterDem, (await map.StyleSourceInfoAsync("dem-tiles"))?.Type);
        Assert.Equal(
            "https://example.test/other.geojson",
            (await map.StyleSourceInfoAsync("geo-url"))?.Url
        );
        Assert.Equal(
            "Vector attribution",
            (await map.StyleSourceInfoAsync("vector-tiles"))?.Attribution
        );
        Assert.Equal(256u, (await map.StyleSourceInfoAsync("raster-tiles"))?.TileSize);
        var demInfo = await map.StyleSourceInfoAsync("dem-tiles");
        Assert.Equal(RasterDemEncoding.Mapbox, demInfo?.RasterDemEncoding);
        Assert.Equal((uint)RasterDemEncoding.Mapbox, demInfo?.RawRasterDemEncoding);
    }

    [BindingSpecTest("BND-109")]
    [Fact]
    public async Task SourceInspectionCopiesUrlAndInlineTileJsonAfterSourceRelease()
    {
        SourceInfo urlInfo;
        SourceInfo inlineInfo;
        var bounds = new LatLngBounds(new LatLng(-40, -120), new LatLng(40, 120));
        var tileUrls = new[]
        {
            "https://example.test/vector-a/{z}/{x}/{y}.pbf",
            "https://example.test/vector-b/{z}/{x}/{y}.pbf",
        };

        using (var runtime = TestHandles.CreateRuntime(new RuntimeOptions()))
        using (
            var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 512, Height = 512 })
        )
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

            urlInfo = Assert.IsType<SourceInfo>(await map.StyleSourceInfoAsync("url-vector"));
            inlineInfo = Assert.IsType<SourceInfo>(await map.StyleSourceInfoAsync("inline-vector"));

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

            Assert.NotEqual(0ul, map.RemoveStyleSource("url-vector"));
            RuntimeEventTestHelpers.AssertCommandFinishes(
                runtime,
                map.RemoveStyleSource("inline-vector"),
                MaplibreStatus.Ok
            );
            Assert.Null(await map.StyleSourceInfoAsync("inline-vector"));
        }

        Assert.Equal("https://example.test/vector.json", urlInfo.Url);
        Assert.Equal(tileUrls, inlineInfo.TileJson?.TileUrls);
        Assert.Equal(bounds, inlineInfo.TileJson?.Bounds);

        using var rebuiltRuntime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var rebuiltMap = TestHandles.CreateMap(
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
        Assert.NotNull(await rebuiltMap.StyleSourceInfoAsync("rebuilt"));
    }

    [BindingSpecTest("BND-101")]
    [Fact]
    public async Task LoadedStyleDocumentAndUrlReadBackWhatWasLoaded()
    {
        var styleJson = EmptyStyle();
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        // Nothing parsed and nothing requested yet.
        Assert.Empty(await map.GetLoadedStyleJsonAsync());
        Assert.Equal(string.Empty, await map.GetStyleUrlAsync());

        // The document reads back byte-for-byte, so it can be reloaded unchanged.
        map.SetStyleJson(styleJson);
        Assert.Equal(styleJson, await map.GetLoadedStyleJsonAsync());
        // Inline JSON clears the URL.
        Assert.Equal(string.Empty, await map.GetStyleUrlAsync());

        // The URL is request state, recorded before the load can succeed, while the
        // document still reports the style that last parsed.
        map.SetStyleUrl("https://example.test/style.json");
        Assert.Equal("https://example.test/style.json", await map.GetStyleUrlAsync());
        Assert.Equal(styleJson, await map.GetLoadedStyleJsonAsync());
    }

    [BindingSpecTest("BND-081", "BND-101")]
    [Fact]
    public void SetStyleJsonReturnsCopiedStyleLoadedEventWithMapIdentity()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

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
        Assert.NotEqual(0UL, runtimeEvent.RawSource);
        Assert.Same(RuntimeEventPayload.None.Instance, runtimeEvent.Payload);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task LayerJsonPropertiesAndFiltersAdaptThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson(EmptyStyle());
        Assert.NotEqual(0ul, map.AddStyleSourceJson("geo", GeoJsonSource()));
        Assert.NotEqual(
            0ul,
            map.AddStyleLayerJson("""{"id":"fill","type":"fill","source":"geo"}"""u8.ToArray(), "")
        );

        Assert.NotEqual(0ul, map.SetLayerProperty("fill", "fill-opacity", "0.5"u8.ToArray()));
        Assert.NotEqual(0ul, map.SetLayerFilter("fill", """["==","kind","park"]"""u8.ToArray()));

        Assert.Equal("0.5"u8.ToArray(), await map.GetLayerPropertyAsync("fill", "fill-opacity"));
        Assert.NotEmpty(Assert.IsType<byte[]>(await map.GetStyleLayerJsonAsync("fill")));
        Assert.Equal("""["==","kind","park"]"""u8.ToArray(), await map.GetLayerFilterAsync("fill"));

        Assert.NotEqual(0ul, map.SetLayerFilter("fill", null));
        Assert.Null(await map.GetLayerFilterAsync("fill"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task StyleSourceAndLayerJsonAdaptThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson(EmptyStyle());

        Assert.NotEqual(0ul, map.AddStyleSourceJson("geo", GeoJsonSource()));
        Assert.Contains("geo", await map.StyleSourceIdsAsync());
        var sourceInfo = await map.StyleSourceInfoAsync("geo");
        Assert.NotNull(sourceInfo);
        Assert.Equal("geo", sourceInfo.Id);
        Assert.Equal(SourceType.GeoJson, sourceInfo.Type);
        Assert.Null(sourceInfo.Attribution);

        Assert.NotEqual(
            0ul,
            map.AddStyleLayerJson("""{"id":"background","type":"background"}"""u8.ToArray(), "")
        );
        Assert.Equal("background", (await map.StyleLayerInfoAsync("background"))?.Type);
        Assert.Contains("background", await map.StyleLayerIdsAsync());

        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleLayer("background"),
            MaplibreStatus.Ok
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleSource("geo"),
            MaplibreStatus.Ok
        );
        Assert.Null(await map.StyleLayerInfoAsync("background"));
        Assert.Null(await map.StyleSourceInfoAsync("geo"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task StyleRemovalCommandsReportNotFoundAndInUseFailures()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson(EmptyStyle());

        // Removing a missing layer, source, or image finishes FAILED with NOT_FOUND.
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleLayer("missing"),
            MaplibreStatus.NotFound
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleSource("missing"),
            MaplibreStatus.NotFound
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleImage("missing"),
            MaplibreStatus.NotFound
        );

        // Removing a source a layer still uses finishes FAILED with INVALID_STATE.
        Assert.NotEqual(0ul, map.AddStyleSourceJson("geo", GeoJsonSource()));
        Assert.NotEqual(
            0ul,
            map.AddStyleLayerJson("""{"id":"fill","type":"fill","source":"geo"}"""u8.ToArray(), "")
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleSource("geo"),
            MaplibreStatus.InvalidState
        );
        Assert.NotNull(await map.StyleSourceInfoAsync("geo"));

        // After the layer goes away the removal commits and the found flag clears.
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleLayer("fill"),
            MaplibreStatus.Ok
        );
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.RemoveStyleSource("geo"),
            MaplibreStatus.Ok
        );
        Assert.Null(await map.StyleSourceInfoAsync("geo"));
    }

    private static byte[] EmptyStyle() => """{"version":8,"sources":{},"layers":[]}"""u8.ToArray();

    private static byte[] GeoJsonSource() =>
        """{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}"""u8.ToArray();
}
