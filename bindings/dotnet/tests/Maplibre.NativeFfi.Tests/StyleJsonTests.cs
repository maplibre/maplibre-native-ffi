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
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        _ = map.AddGeoJsonSourceUrlAsync(
            "geo-url",
            "https://example.test/data.geojson",
            null,
            TestContext.Current.CancellationToken
        );
        _ = map.SetGeoJsonSourceUrlAsync(
            "geo-url",
            "https://example.test/other.geojson",
            TestContext.Current.CancellationToken
        );
        _ = map.AddVectorSourceTilesAsync(
            "vector-tiles",
            ["https://example.test/vector/{z}/{x}/{y}.pbf"],
            new TileSourceOptions
            {
                MinimumZoom = 1,
                MaximumZoom = 12,
                Attribution = "Vector attribution",
                Scheme = TileScheme.Xyz,
                VectorEncoding = VectorTileEncoding.Mvt,
            },
            TestContext.Current.CancellationToken
        );
        _ = map.AddRasterSourceTilesAsync(
            "raster-tiles",
            ["https://example.test/raster/{z}/{x}/{y}.png"],
            new TileSourceOptions { TileSize = 256 },
            TestContext.Current.CancellationToken
        );
        _ = map.AddRasterDemSourceTilesAsync(
            "dem-tiles",
            ["https://example.test/dem/{z}/{x}/{y}.png"],
            new TileSourceOptions { RasterEncoding = RasterDemEncoding.Mapbox },
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            SourceType.GeoJson,
            (await map.StyleSourceInfoAsync("geo-url", TestContext.Current.CancellationToken))?.Type
        );
        Assert.Equal(
            SourceType.Vector,
            (
                await map.StyleSourceInfoAsync(
                    "vector-tiles",
                    TestContext.Current.CancellationToken
                )
            )?.Type
        );
        Assert.Equal(
            SourceType.Raster,
            (
                await map.StyleSourceInfoAsync(
                    "raster-tiles",
                    TestContext.Current.CancellationToken
                )
            )?.Type
        );
        Assert.Equal(
            SourceType.RasterDem,
            (
                await map.StyleSourceInfoAsync("dem-tiles", TestContext.Current.CancellationToken)
            )?.Type
        );
        Assert.Equal(
            "https://example.test/other.geojson",
            (await map.StyleSourceInfoAsync("geo-url", TestContext.Current.CancellationToken))?.Url
        );
        Assert.Equal(
            "Vector attribution",
            (
                await map.StyleSourceInfoAsync(
                    "vector-tiles",
                    TestContext.Current.CancellationToken
                )
            )?.Attribution
        );
        Assert.Equal(
            256u,
            (
                await map.StyleSourceInfoAsync(
                    "raster-tiles",
                    TestContext.Current.CancellationToken
                )
            )?.TileSize
        );
        var demInfo = await map.StyleSourceInfoAsync(
            "dem-tiles",
            TestContext.Current.CancellationToken
        );
        Assert.Equal(RasterDemEncoding.Mapbox, demInfo?.RasterDemEncoding);
        Assert.Equal((uint)RasterDemEncoding.Mapbox, demInfo?.RawRasterDemEncoding);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task StyleSourceVolatilityReadsBackAndRejectsMissingSource()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        _ = map.AddVectorSourceTilesAsync(
            "volatile-source",
            ["https://example.test/vector/{z}/{x}/{y}.pbf"],
            new TileSourceOptions(),
            TestContext.Current.CancellationToken
        );

        Assert.False(
            (
                await map.StyleSourceInfoAsync(
                    "volatile-source",
                    TestContext.Current.CancellationToken
                )
            )!.IsVolatile
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleSourceVolatileAsync(
                "volatile-source",
                true,
                TestContext.Current.CancellationToken
            )
        );
        Assert.True(
            (
                await map.StyleSourceInfoAsync(
                    "volatile-source",
                    TestContext.Current.CancellationToken
                )
            )!.IsVolatile
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleSourceVolatileAsync(
                "volatile-source",
                false,
                TestContext.Current.CancellationToken
            )
        );
        Assert.False(
            (
                await map.StyleSourceInfoAsync(
                    "volatile-source",
                    TestContext.Current.CancellationToken
                )
            )!.IsVolatile
        );

        RuntimeEventTestHelpers.AssertFailed(
            map.SetStyleSourceVolatileAsync(
                "missing-source",
                true,
                TestContext.Current.CancellationToken
            ),
            MaplibreStatus.NotFound
        );
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

        using (var runtime = RuntimeHandle.Create(new RuntimeOptions()))
        using (
            var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 512, Height = 512 })
        )
        {
            _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
            _ = map.AddVectorSourceUrlAsync(
                "url-vector",
                "https://example.test/vector.json",
                null,
                TestContext.Current.CancellationToken
            );
            _ = map.AddVectorSourceTilesAsync(
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
                },
                TestContext.Current.CancellationToken
            );

            urlInfo = Assert.IsType<SourceInfo>(
                await map.StyleSourceInfoAsync("url-vector", TestContext.Current.CancellationToken)
            );
            inlineInfo = Assert.IsType<SourceInfo>(
                await map.StyleSourceInfoAsync(
                    "inline-vector",
                    TestContext.Current.CancellationToken
                )
            );

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

            RuntimeEventTestHelpers.AssertCommitted(
                map.RemoveStyleSourceAsync("url-vector", TestContext.Current.CancellationToken)
            );
            RuntimeEventTestHelpers.AssertCommitted(
                map.RemoveStyleSourceAsync("inline-vector", TestContext.Current.CancellationToken)
            );
            Assert.Null(
                await map.StyleSourceInfoAsync(
                    "inline-vector",
                    TestContext.Current.CancellationToken
                )
            );
        }

        Assert.Equal("https://example.test/vector.json", urlInfo.Url);
        Assert.Equal(tileUrls, inlineInfo.TileJson?.TileUrls);
        Assert.Equal(bounds, inlineInfo.TileJson?.Bounds);

        using var rebuiltRuntime = RuntimeHandle.Create(new RuntimeOptions());
        using var rebuiltMap = TestHandles.CreateMap(
            rebuiltRuntime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = rebuiltMap.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        _ = rebuiltMap.AddVectorSourceTilesAsync(
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
            },
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(
            await rebuiltMap.StyleSourceInfoAsync("rebuilt", TestContext.Current.CancellationToken)
        );
    }

    // The narrow copies read the same values the aggregate reports, one field at a time.
    [BindingSpecTest("BND-101")]
    [Fact]
    public async Task NarrowSourceCopiesReadTheSameValuesAsTheAggregate()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });
        var tileUrls = new[]
        {
            "https://example.test/tiles/{z}/{x}/{y}.pbf",
            "https://example.test/mirror/{z}/{x}/{y}.pbf",
        };

        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        _ = map.AddVectorSourceUrlAsync(
            "url-vector",
            "https://example.test/vector.json",
            null,
            TestContext.Current.CancellationToken
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddVectorSourceTilesAsync(
                "inline-vector",
                tileUrls,
                new TileSourceOptions { Attribution = "Inline attribution" },
                TestContext.Current.CancellationToken
            )
        );

        Assert.Equal(
            "https://example.test/vector.json",
            await map.GetStyleSourceUrlAsync("url-vector", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceUrlAsync("inline-vector", TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            "Inline attribution",
            await map.GetStyleSourceAttributionAsync(
                "inline-vector",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Null(
            await map.GetStyleSourceAttributionAsync(
                "url-vector",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(
            tileUrls,
            await map.GetStyleSourceTileUrlsAsync(
                "inline-vector",
                TestContext.Current.CancellationToken
            )
        );
        var urlBackedTileUrls = await map.GetStyleSourceTileUrlsAsync(
            "url-vector",
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(urlBackedTileUrls);
        Assert.Empty(urlBackedTileUrls);

        // A missing source is not an error for these queries.
        Assert.Null(
            await map.GetStyleSourceUrlAsync("missing", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceTileUrlsAsync("missing", TestContext.Current.CancellationToken)
        );
    }

    [BindingSpecTest("BND-101")]
    [Fact]
    public async Task LoadedStyleDocumentAndUrlReadBackWhatWasLoaded()
    {
        var styleJson = TestStyles.Empty;
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        // Nothing parsed and nothing requested yet.
        Assert.Empty(await map.GetLoadedStyleJsonAsync(TestContext.Current.CancellationToken));
        Assert.Equal(
            string.Empty,
            await map.GetStyleUrlAsync(TestContext.Current.CancellationToken)
        );

        // The document reads back byte-for-byte, so it can be reloaded unchanged.
        _ = map.SetStyleJsonAsync(styleJson, TestContext.Current.CancellationToken);
        Assert.Equal(
            styleJson,
            await map.GetLoadedStyleJsonAsync(TestContext.Current.CancellationToken)
        );
        // Inline JSON clears the URL.
        Assert.Equal(
            string.Empty,
            await map.GetStyleUrlAsync(TestContext.Current.CancellationToken)
        );

        // The URL is request state, recorded before the load can succeed, while the
        // document still reports the style that last parsed.
        _ = map.SetStyleUrlAsync(
            "https://example.test/style.json",
            TestContext.Current.CancellationToken
        );
        Assert.Equal(
            "https://example.test/style.json",
            await map.GetStyleUrlAsync(TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            styleJson,
            await map.GetLoadedStyleJsonAsync(TestContext.Current.CancellationToken)
        );
    }

    [BindingSpecTest("BND-081", "BND-101")]
    [Fact]
    public void SetStyleJsonReturnsCopiedStyleLoadedEventWithMapIdentity()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
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
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddStyleSourceJsonAsync(
                "geo",
                GeoJsonSource(),
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddStyleLayerJsonAsync(
                """{"id":"fill","type":"fill","source":"geo"}"""u8.ToArray(),
                "",
                TestContext.Current.CancellationToken
            )
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerPropertyAsync(
                "fill",
                "fill-opacity",
                "0.5"u8.ToArray(),
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerFilterAsync(
                "fill",
                """["==","kind","park"]"""u8.ToArray(),
                TestContext.Current.CancellationToken
            )
        );

        Assert.Equal(
            "0.5"u8.ToArray(),
            await map.GetLayerPropertyAsync(
                "fill",
                "fill-opacity",
                TestContext.Current.CancellationToken
            )
        );
        Assert.NotEmpty(
            Assert.IsType<byte[]>(
                await map.GetStyleLayerJsonAsync("fill", TestContext.Current.CancellationToken)
            )
        );
        Assert.Equal(
            """["==","kind","park"]"""u8.ToArray(),
            await map.GetLayerFilterAsync("fill", TestContext.Current.CancellationToken)
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetLayerFilterAsync("fill", null, TestContext.Current.CancellationToken)
        );
        Assert.Null(await map.GetLayerFilterAsync("fill", TestContext.Current.CancellationToken));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task StyleSourceAndLayerJsonAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        RuntimeEventTestHelpers.AssertCommitted(
            map.AddStyleSourceJsonAsync(
                "geo",
                GeoJsonSource(),
                TestContext.Current.CancellationToken
            )
        );
        Assert.Contains(
            "geo",
            await map.StyleSourceIdsAsync(TestContext.Current.CancellationToken)
        );
        var sourceInfo = await map.StyleSourceInfoAsync(
            "geo",
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(sourceInfo);
        Assert.Equal("geo", sourceInfo.Id);
        Assert.Equal(SourceType.GeoJson, sourceInfo.Type);
        Assert.Null(sourceInfo.Attribution);

        RuntimeEventTestHelpers.AssertCommitted(
            map.AddStyleLayerJsonAsync(
                """{"id":"background","type":"background"}"""u8.ToArray(),
                "",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(
            "background",
            (
                await map.StyleLayerInfoAsync("background", TestContext.Current.CancellationToken)
            )?.Type
        );
        Assert.Contains(
            "background",
            await map.StyleLayerIdsAsync(TestContext.Current.CancellationToken)
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleLayerAsync("background", TestContext.Current.CancellationToken)
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("geo", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.StyleLayerInfoAsync("background", TestContext.Current.CancellationToken)
        );
        Assert.Null(await map.StyleSourceInfoAsync("geo", TestContext.Current.CancellationToken));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task StyleRemovalCommandsReportNotFoundAndInUseFailures()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        // Removing a missing layer, source, or image finishes FAILED with NOT_FOUND.
        RuntimeEventTestHelpers.AssertFailed(
            map.RemoveStyleLayerAsync("missing", TestContext.Current.CancellationToken),
            MaplibreStatus.NotFound
        );
        RuntimeEventTestHelpers.AssertFailed(
            map.RemoveStyleSourceAsync("missing", TestContext.Current.CancellationToken),
            MaplibreStatus.NotFound
        );
        RuntimeEventTestHelpers.AssertFailed(
            map.RemoveStyleImageAsync("missing", TestContext.Current.CancellationToken),
            MaplibreStatus.NotFound
        );

        // Removing a source a layer still uses finishes FAILED with INVALID_STATE.
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddStyleSourceJsonAsync(
                "geo",
                GeoJsonSource(),
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.AddStyleLayerJsonAsync(
                """{"id":"fill","type":"fill","source":"geo"}"""u8.ToArray(),
                "",
                TestContext.Current.CancellationToken
            )
        );
        RuntimeEventTestHelpers.AssertFailed(
            map.RemoveStyleSourceAsync("geo", TestContext.Current.CancellationToken),
            MaplibreStatus.InvalidState
        );
        Assert.NotNull(
            await map.StyleSourceInfoAsync("geo", TestContext.Current.CancellationToken)
        );

        // After the layer goes away the removal commits and the found flag clears.
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleLayerAsync("fill", TestContext.Current.CancellationToken)
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("geo", TestContext.Current.CancellationToken)
        );
        Assert.Null(await map.StyleSourceInfoAsync("geo", TestContext.Current.CancellationToken));
    }

    private static byte[] GeoJsonSource() =>
        """{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}"""u8.ToArray();
}
