using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class StyleJsonTests
{
    [Fact]
    public async Task UrlAndTileSourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        _ = map.AddGeojsonSourceUrlAsync(
            "geo-url",
            "https://example.test/data.geojson",
            null,
            TestContext.Current.CancellationToken
        );
        _ = map.SetGeojsonSourceUrlAsync(
            "geo-url",
            "https://example.test/other.geojson",
            TestContext.Current.CancellationToken
        );
        _ = map.AddVectorSourceTilesAsync(
            "vector-tiles",
            ["https://example.test/vector/{z}/{x}/{y}.pbf"],
            new StyleTileSourceOptions
            {
                MinZoom = 1,
                MaxZoom = 12,
                Attribution = "Vector attribution",
                Scheme = StyleTileScheme.Xyz,
                VectorEncoding = StyleVectorTileEncoding.Mvt,
            },
            TestContext.Current.CancellationToken
        );
        _ = map.AddRasterSourceTilesAsync(
            "raster-tiles",
            ["https://example.test/raster/{z}/{x}/{y}.png"],
            new StyleTileSourceOptions { TileSize = 256 },
            TestContext.Current.CancellationToken
        );
        _ = map.AddRasterDemSourceTilesAsync(
            "dem-tiles",
            ["https://example.test/dem/{z}/{x}/{y}.png"],
            new StyleTileSourceOptions { RasterEncoding = StyleRasterDemEncoding.Mapbox },
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            StyleSourceType.Geojson,
            (await map.GetStyleSourceInfoAsync("geo-url", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        Assert.Equal(
            StyleSourceType.Vector,
            (
                await map.GetStyleSourceInfoAsync(
                    "vector-tiles",
                    TestContext.Current.CancellationToken
                )
            )
                ?.Info
                .Type
        );
        Assert.Equal(
            StyleSourceType.Raster,
            (
                await map.GetStyleSourceInfoAsync(
                    "raster-tiles",
                    TestContext.Current.CancellationToken
                )
            )
                ?.Info
                .Type
        );
        Assert.Equal(
            StyleSourceType.RasterDem,
            (await map.GetStyleSourceInfoAsync("dem-tiles", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        Assert.Equal(
            "https://example.test/other.geojson",
            (
                await map.GetStyleSourceInfoAsync("geo-url", TestContext.Current.CancellationToken)
            )?.Url
        );
        Assert.Equal(
            "Vector attribution",
            (
                await map.GetStyleSourceInfoAsync(
                    "vector-tiles",
                    TestContext.Current.CancellationToken
                )
            )?.Attribution
        );
        Assert.Equal(
            256u,
            (
                await map.GetStyleSourceInfoAsync(
                    "raster-tiles",
                    TestContext.Current.CancellationToken
                )
            )
                ?.Info
                .TileSize
        );
        var demInfo = await map.GetStyleSourceInfoAsync(
            "dem-tiles",
            TestContext.Current.CancellationToken
        );
        Assert.Equal(StyleRasterDemEncoding.Mapbox, demInfo?.Info.RasterEncoding);
    }

    [Fact]
    public async Task StyleSourceVolatilityReadsBackAndRejectsMissingSource()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        _ = map.AddVectorSourceTilesAsync(
            "volatile-source",
            ["https://example.test/vector/{z}/{x}/{y}.pbf"],
            new StyleTileSourceOptions(),
            TestContext.Current.CancellationToken
        );

        Assert.False(
            (
                await map.GetStyleSourceInfoAsync(
                    "volatile-source",
                    TestContext.Current.CancellationToken
                )
            )!
                .Info
                .IsVolatile
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
                await map.GetStyleSourceInfoAsync(
                    "volatile-source",
                    TestContext.Current.CancellationToken
                )
            )!
                .Info
                .IsVolatile
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
                await map.GetStyleSourceInfoAsync(
                    "volatile-source",
                    TestContext.Current.CancellationToken
                )
            )!
                .Info
                .IsVolatile
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

    [Fact]
    public async Task SourceInspectionCopiesUrlAndInlineTileJsonAfterSourceRelease()
    {
        StyleSourceResult urlInfo;
        StyleSourceResult inlineInfo;
        var bounds = new LatLngBounds(new LatLng(-40, -120), new LatLng(40, 120));
        var tileUrls = new[]
        {
            "https://example.test/vector-a/{z}/{x}/{y}.pbf",
            "https://example.test/vector-b/{z}/{x}/{y}.pbf",
        };

        using (var runtime = RuntimeHandle.Create(RuntimeOptions.Default))
        using (
            var map = TestHandles.CreateMap(
                runtime,
                MapOptions.Default with
                {
                    InitialExtent = MapOptions.Default.InitialExtent with
                    {
                        Width = 512,
                        Height = 512,
                    },
                }
            )
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
                new StyleTileSourceOptions
                {
                    MinZoom = 0,
                    MaxZoom = 12,
                    Attribution = "Inline attribution",
                    Scheme = StyleTileScheme.Tms,
                    VectorEncoding = StyleVectorTileEncoding.Mlt,
                    Bounds = bounds,
                },
                TestContext.Current.CancellationToken
            );

            urlInfo = Assert.IsType<StyleSourceResult>(
                await map.GetStyleSourceInfoAsync(
                    "url-vector",
                    TestContext.Current.CancellationToken
                )
            );
            inlineInfo = Assert.IsType<StyleSourceResult>(
                await map.GetStyleSourceInfoAsync(
                    "inline-vector",
                    TestContext.Current.CancellationToken
                )
            );

            Assert.Equal("https://example.test/vector.json", urlInfo.Url);
            Assert.Null(urlInfo.Info.Tilejson);
            Assert.Null(urlInfo.Attribution);

            Assert.Null(inlineInfo.Url);
            Assert.Equal("Inline attribution", inlineInfo.Attribution);
            Assert.NotNull(inlineInfo.Info.Tilejson);
            Assert.Equal(tileUrls, inlineInfo.TileUrls);
            Assert.Equal(0, inlineInfo.Info.Tilejson.Value.MinZoom);
            Assert.Equal(12, inlineInfo.Info.Tilejson.Value.MaxZoom);
            Assert.Equal(StyleTileScheme.Tms, inlineInfo.Info.Tilejson.Value.Scheme);
            Assert.Equal(bounds, inlineInfo.Info.Bounds);
            Assert.Equal(512u, inlineInfo.Info.TileSize);
            Assert.Equal(StyleVectorTileEncoding.Mlt, inlineInfo.Info.VectorEncoding);
            Assert.Null(inlineInfo.Info.RasterEncoding);

            RuntimeEventTestHelpers.AssertCommitted(
                map.RemoveStyleSourceAsync("url-vector", TestContext.Current.CancellationToken)
            );
            RuntimeEventTestHelpers.AssertCommitted(
                map.RemoveStyleSourceAsync("inline-vector", TestContext.Current.CancellationToken)
            );
            Assert.Null(
                await map.GetStyleSourceInfoAsync(
                    "inline-vector",
                    TestContext.Current.CancellationToken
                )
            );
        }

        Assert.Equal("https://example.test/vector.json", urlInfo.Url);
        Assert.Equal(tileUrls, inlineInfo.TileUrls);
        Assert.Equal(bounds, inlineInfo.Info.Bounds);

        Assert.NotNull(inlineInfo.TileUrls);
        var tileInfo =
            inlineInfo.Info.Tilejson
            ?? throw new InvalidOperationException("Expected tile metadata.");
        using var rebuiltRuntime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var rebuiltMap = TestHandles.CreateMap(
            rebuiltRuntime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = rebuiltMap.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        _ = rebuiltMap.AddVectorSourceTilesAsync(
            "rebuilt",
            inlineInfo.TileUrls,
            new StyleTileSourceOptions
            {
                MinZoom = tileInfo.MinZoom,
                MaxZoom = tileInfo.MaxZoom,
                Scheme = tileInfo.Scheme,
                Bounds = inlineInfo.Info.Bounds,
                TileSize = inlineInfo.Info.TileSize,
                Attribution = inlineInfo.Attribution,
                VectorEncoding = inlineInfo.Info.VectorEncoding,
            },
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(
            await rebuiltMap.GetStyleSourceInfoAsync(
                "rebuilt",
                TestContext.Current.CancellationToken
            )
        );
    }

    // The narrow copies read the same values the aggregate reports, one field at a time.
    [Fact]
    public async Task NarrowSourceCopiesReadTheSameValuesAsTheAggregate()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 64, Height = 64 },
            }
        );
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
                new StyleTileSourceOptions { Attribution = "Inline attribution" },
                TestContext.Current.CancellationToken
            )
        );

        Assert.Equal(
            "https://example.test/vector.json",
            await map.CopyStyleSourceUrlAsync("url-vector", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.CopyStyleSourceUrlAsync(
                "inline-vector",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(
            "Inline attribution",
            await map.CopyStyleSourceAttributionAsync(
                "inline-vector",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Null(
            await map.CopyStyleSourceAttributionAsync(
                "url-vector",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(
            tileUrls,
            (
                await map.GetStyleSourceTileUrlsAsync(
                    "inline-vector",
                    TestContext.Current.CancellationToken
                )
            )?.TileUrls
        );
        var urlBackedTileUrls = await map.GetStyleSourceTileUrlsAsync(
            "url-vector",
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(urlBackedTileUrls);
        Assert.Empty(urlBackedTileUrls.Value.TileUrls);

        // A missing source is not an error for these queries.
        Assert.Null(
            await map.CopyStyleSourceUrlAsync("missing", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceTileUrlsAsync("missing", TestContext.Current.CancellationToken)
        );
    }

    [Fact]
    public async Task LoadedStyleDocumentAndUrlReadBackWhatWasLoaded()
    {
        var styleJson = TestStyles.Empty;
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        // Nothing parsed and nothing requested yet.
        Assert.Empty(await map.LoadedStyleJsonAsync(TestContext.Current.CancellationToken));
        Assert.Equal(string.Empty, await map.StyleUrlAsync(TestContext.Current.CancellationToken));

        // The document reads back byte-for-byte, so it can be reloaded unchanged.
        _ = map.SetStyleJsonAsync(styleJson, TestContext.Current.CancellationToken);
        Assert.Equal(
            styleJson,
            await map.LoadedStyleJsonAsync(TestContext.Current.CancellationToken)
        );
        // Inline JSON clears the URL.
        Assert.Equal(string.Empty, await map.StyleUrlAsync(TestContext.Current.CancellationToken));

        // The URL is request state, recorded before the load can succeed, while the
        // document still reports the style that last parsed.
        _ = map.SetStyleUrlAsync(
            "https://example.test/style.json",
            TestContext.Current.CancellationToken
        );
        Assert.Equal(
            "https://example.test/style.json",
            await map.StyleUrlAsync(TestContext.Current.CancellationToken)
        );
        Assert.Equal(
            styleJson,
            await map.LoadedStyleJsonAsync(TestContext.Current.CancellationToken)
        );
    }

    [Fact]
    public void SetStyleJsonReturnsCopiedStyleLoadedEventWithMapIdentity()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var runtimeEvent = RuntimeEventTestHelpers.WaitForMapEvent(
            runtime,
            map,
            RuntimeEventType.MapStyleLoaded
        );

        Assert.Equal(RuntimeEventType.MapStyleLoaded, runtimeEvent.Type);
        Assert.Equal((uint)RuntimeEventType.MapStyleLoaded, (uint)runtimeEvent.Type);
        Assert.Equal(RuntimeEventSourceType.Map, runtimeEvent.SourceType);
        Assert.Equal(map.Id, runtimeEvent.Source);
        Assert.NotEqual(0UL, runtimeEvent.Source);
        Assert.IsType<RuntimeEvent.PayloadValue.None>(runtimeEvent.Payload);
    }

    [Fact]
    public async Task LayerJsonPropertiesAndFiltersAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
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

    [Fact]
    public async Task StyleSourceAndLayerJsonAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
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
        var sourceInfo = await map.GetStyleSourceInfoAsync(
            "geo",
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(sourceInfo);
        Assert.Equal(StyleSourceType.Geojson, sourceInfo.Info.Type);
        Assert.False(sourceInfo.Attribution is not null);

        RuntimeEventTestHelpers.AssertCommitted(
            map.AddStyleLayerJsonAsync(
                """{"id":"background","type":"background"}"""u8.ToArray(),
                "",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(
            "background",
            (await map.GetStyleLayerInfoAsync("background", TestContext.Current.CancellationToken))
                ?.Info
                .Type
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
            await map.GetStyleLayerInfoAsync("background", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceInfoAsync("geo", TestContext.Current.CancellationToken)
        );
    }

    [Fact]
    public async Task StyleRemovalCommandsReportNotFoundAndInUseFailures()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
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
            await map.GetStyleSourceInfoAsync("geo", TestContext.Current.CancellationToken)
        );

        // After the layer goes away the removal commits and the found flag clears.
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleLayerAsync("fill", TestContext.Current.CancellationToken)
        );
        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleSourceAsync("geo", TestContext.Current.CancellationToken)
        );
        Assert.Null(
            await map.GetStyleSourceInfoAsync("geo", TestContext.Current.CancellationToken)
        );
    }

    private static byte[] GeoJsonSource() =>
        """{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}"""u8.ToArray();
}
