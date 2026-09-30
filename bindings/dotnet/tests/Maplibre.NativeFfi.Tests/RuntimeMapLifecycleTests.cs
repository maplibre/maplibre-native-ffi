using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeMapLifecycleTests
{
    [Fact]
    public async Task AbandonedCreationResultReleasesItsNativeMap()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var weak = CreateUnclaimedMap(runtime);
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        Assert.False(weak.IsAlive);
        await runtime.CloseAsync();
    }

    [System.Runtime.CompilerServices.MethodImpl(
        System.Runtime.CompilerServices.MethodImplOptions.NoInlining
    )]
    private static WeakReference CreateUnclaimedMap(RuntimeHandle runtime)
    {
        var map = runtime.MapCreateAsync(MapOptions.Default).GetAwaiter().GetResult();
        return new WeakReference(map);
    }

    [Fact]
    public void UnreachableRuntimeAndMapReleaseNativeOwnership()
    {
        var reports = new List<NativeLeakReport>();
        using var capture = NativeLeakReporter.CaptureForTest(reports.Add);
        var weak = CreateUnclaimedRuntimeAndMap();
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        Assert.False(weak.Runtime.IsAlive);
        Assert.False(weak.Map.IsAlive);
        Assert.Empty(reports);
    }

    [System.Runtime.CompilerServices.MethodImpl(
        System.Runtime.CompilerServices.MethodImplOptions.NoInlining
    )]
    private static (WeakReference Runtime, WeakReference Map) CreateUnclaimedRuntimeAndMap()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = runtime.MapCreateAsync(MapOptions.Default).GetAwaiter().GetResult();
        runtime.BarrierAsync().GetAwaiter().GetResult();
        return (new WeakReference(runtime), new WeakReference(map));
    }

    // A descriptor that writes no field takes the native creation defaults, which the map then
    // publishes.
    [Fact]
    public void DefaultMapOptionsPreserveNativeCreationDefaults()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);

        var snapshot = map.SnapshotGet();
        Assert.Equal(new LogicalExtent(256, 256, 1), snapshot.LogicalExtent);
        Assert.Equal((MapDebugOption)0, snapshot.DebugOptions);
        Assert.False(snapshot.RenderingStatsViewEnabled);
        Assert.False(snapshot.GestureInProgress);
    }

    [Fact]
    public void MapOptionsMaterializeFastPforDecoding()
    {
        Assert.Equal(0, GeneratedValues.NativeMapOptions(MapOptions.Default).fast_pfor_enabled);
        Assert.Equal(
            1,
            GeneratedValues
                .NativeMapOptions(MapOptions.Default with { FastPforEnabled = true })
                .fast_pfor_enabled
        );

        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 128, Height = 64 },

                FastPforEnabled = true,
            }
        );

        Assert.Equal(new LogicalExtent(128, 64, 1), map.SnapshotGet().LogicalExtent);
    }

    [Fact]
    public void RuntimeAndMapCloseDeterministically()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        Assert.False(runtime.IsClosed);
        Assert.False(map.IsClosed);

        map.Close();
        runtime.Close();

        Assert.True(map.IsClosed);
        Assert.True(runtime.IsClosed);
    }

    [Fact]
    public async Task RuntimeCloseAsyncCompletesAfterNativeTeardown()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = await runtime.MapCreateAsync(
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        _ = map.SetStyleUrlAsync("unsupported://style.json", TestContext.Current.CancellationToken);
        map.Close();

        var teardown = runtime.CloseAsync();

        Assert.True(runtime.IsClosed);
        await teardown;

        var second = runtime.CloseAsync();
        Assert.True(second.IsCompletedSuccessfully);
        await second;
    }

    [Fact]
    public async Task RuntimeDisposeAsyncWaitsForNativeTeardown()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        await using (runtime)
        {
            await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        }

        Assert.True(runtime.IsClosed);
    }

    [Fact]
    public void RuntimeCloseFailsWhileMapIsLiveAndCanRetryAfterMapClose()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        var error = Assert.Throws<InvalidStateException>(() => runtime.Close());

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Equal((int)MaplibreStatus.InvalidState, error.RawStatus);
        Assert.False(runtime.IsClosed);
        Assert.Contains(
            "live or pending children",
            error.Diagnostic,
            StringComparison.OrdinalIgnoreCase
        );

        map.Close();
        runtime.Close();

        Assert.True(runtime.IsClosed);
    }

    [Fact]
    public void CommittedCommandIsVisibleInSnapshotsAtOrPastItsGeneration()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        var options = MapDebugOption.TileBorders | MapDebugOption.ParseStatus;
        var completion = RuntimeEventTestHelpers.AssertCommitted(
            map.SetDebugOptionsAsync(options, TestContext.Current.CancellationToken)
        );

        // The published snapshot fence: a snapshot at or past the commit's generation
        // observes the committed value.
        var snapshot = map.SnapshotGet();
        Assert.True(snapshot.Generation >= completion.Generation);
        Assert.Equal(options, snapshot.DebugOptions);
        Assert.False(snapshot.FullyLoaded);
    }

    [Fact]
    public void DumpingDebugLogsCommits()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.DumpDebugLogsAsync(TestContext.Current.CancellationToken)
        );
    }

    // A still image stays pending until a render session produces it, so closing the map with
    // no session attached retires the request and reports the cancelled status.
    [Fact]
    public async Task AnOutstandingRequestIsCancelledWhenTheMapCloses()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 64, Height = 64 },

                MapMode = MapMode.Static,
            }
        );

        var stillImage = map.RequestStillImageAsync(TestContext.Current.CancellationToken);
        Assert.False(stillImage.IsCompleted);

        await map.CloseAsync();

        var error = await Assert.ThrowsAsync<MaplibreException>(() => stillImage);
        Assert.Equal(MaplibreStatus.Cancelled, error.Status);
    }

    [Fact]
    public void RenderingStatsViewEnabledRoundTripsThroughSnapshot()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        Assert.False(map.SnapshotGet().RenderingStatsViewEnabled);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetRenderingStatsViewEnabledAsync(true, TestContext.Current.CancellationToken)
        );
        Assert.True(map.SnapshotGet().RenderingStatsViewEnabled);
    }

    [Fact]
    public void MapSizeReportsCreationExtentAndPixelRatio()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with
                {
                    Width = 512,
                    Height = 256,
                    ScaleFactor = 2,
                },
            }
        );

        Assert.Equal(new LogicalExtent(512, 256, 2), map.SnapshotGet().LogicalExtent);
    }

    [Fact]
    public async Task RuntimeAndMapWorkAcrossManagedThreads()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = await runtime.MapCreateAsync(
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        var completion = await Task.Run(() => map.RequestRepaintAsync());
        var snapshot = await Task.Run(map.SnapshotGet);
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        Assert.Equal(CommandDisposition.Committed, completion.Disposition);
        Assert.Equal(new LogicalExtent(512, 512, 1), snapshot.LogicalExtent);
    }

    [Fact]
    public void MethodsRejectClosedMapBeforeNativeCall()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        map.Close();
        runtime.Close();

        var error = Assert.Throws<InvalidStateException>(() =>
        {
            _ = map.RequestRepaintAsync(TestContext.Current.CancellationToken);
        });

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains("closed", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }
}
