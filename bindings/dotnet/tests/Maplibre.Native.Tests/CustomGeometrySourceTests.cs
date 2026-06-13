using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.Callback;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class CustomGeometrySourceTests
{
    [Fact]
    public void CustomGeometryCallbacksCopyTileIdsAndSwallowExceptions()
    {
        CanonicalTileId? fetched = null;
        CanonicalTileId? cancelled = null;
        using var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions
            {
                FetchTile = tileId => fetched = tileId,
                CancelTile = tileId => cancelled = tileId,
            }
        );
        var tile = new CanonicalTileId(1, 2, 3);

        state.FetchForTest(tile);
        state.CancelForTest(tile);

        Assert.Equal(tile, fetched);
        Assert.Equal(tile, cancelled);

        using var throwing = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions
            {
                FetchTile = _ => throw new InvalidOperationException("boom"),
            }
        );
        throwing.FetchForTest(tile);
    }

    [Fact]
    public void CustomGeometrySourceRequiresFetchTileCallback()
    {
        var error = Assert.Throws<ArgumentException>(() =>
            new CustomGeometrySourceState(new CustomGeometrySourceOptions())
        );
        Assert.Equal("options", error.ParamName);
    }

    [Fact]
    public void CustomGeometrySourceRejectsNegativeBuffer()
    {
        using var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions { FetchTile = _ => { }, Buffer = -1 }
        );

        var error = Assert.Throws<InvalidArgumentException>(() => state.Descriptor);

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
    }

    [Fact]
    public async Task CustomGeometryDisposeKeepsHandleAliveUntilActiveCallbackExits()
    {
        var entered = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var release = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var state = new CustomGeometrySourceState(
            new CustomGeometrySourceOptions
            {
                FetchTile = _ =>
                {
                    entered.SetResult();
                    release.Task.GetAwaiter().GetResult();
                },
            }
        );

        var worker = Task.Run(
            () => state.FetchForTest(new CanonicalTileId(1, 2, 3)),
            TestContext.Current.CancellationToken
        );
        await entered.Task.WaitAsync(
            TimeSpan.FromSeconds(5),
            TestContext.Current.CancellationToken
        );

        state.Dispose();

        Assert.True(state.IsHandleAllocatedForTest);
        release.SetResult();
        await worker.WaitAsync(TimeSpan.FromSeconds(5), TestContext.Current.CancellationToken);
        Assert.False(state.IsHandleAllocatedForTest);
    }

    [Fact]
    public void CustomGeometrySourceApisAdaptThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        var tile = new CanonicalTileId(0, 0, 0);

        map.AddCustomGeometrySource(
            "custom",
            new CustomGeometrySourceOptions
            {
                FetchTile = _ => { },
                CancelTile = _ => { },
                TileSize = 512,
                MinimumZoom = 0,
                MaximumZoom = 10,
                Tolerance = 0.375,
                Buffer = 128,
                Clip = true,
                Wrap = false,
            }
        );
        map.SetCustomGeometrySourceTileData("custom", tile, new GeoJson.FeatureCollection([]));
        map.InvalidateCustomGeometrySourceTile("custom", tile);
        map.InvalidateCustomGeometrySourceRegion(
            "custom",
            new LatLngBounds(new LatLng(-1, -1), new LatLng(1, 1))
        );

        Assert.Equal(SourceType.CustomVector, map.StyleSourceType("custom"));
        Assert.True(map.RemoveStyleSource("custom"));
    }

    [Fact]
    public void DetachedCustomGeometryCleanupKeepsActiveCustomVectorSources()
    {
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        map.AddCustomGeometrySource(
            "custom",
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );

        map.ReleaseDetachedCustomGeometrySources();

        Assert.Equal(1, map.CustomGeometrySourceCountForTest);
        Assert.Equal(SourceType.CustomVector, map.StyleSourceType("custom"));
    }

    [Fact]
    public void StaleStyleLoadedEventKeepsStillAttachedCustomGeometrySource()
    {
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        map.AddCustomGeometrySource(
            "custom",
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );

        for (var attempt = 0; attempt < 8; attempt++)
        {
            var runtimeEvent = runtime.PollEvent();
            if (runtimeEvent?.Type == RuntimeEventType.MapStyleLoaded)
            {
                break;
            }
            runtime.RunOnce();
        }

        Assert.Equal(1, map.CustomGeometrySourceCountForTest);
        Assert.Equal(SourceType.CustomVector, map.StyleSourceType("custom"));
    }

    [Fact]
    public void InlineStyleReplacementReleasesCustomGeometryCallbacks()
    {
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        map.AddCustomGeometrySource(
            "custom",
            new CustomGeometrySourceOptions { FetchTile = _ => { } }
        );

        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");

        Assert.Equal(0, map.CustomGeometrySourceCountForTest);
        Assert.False(map.StyleSourceExists("custom"));
    }
}
