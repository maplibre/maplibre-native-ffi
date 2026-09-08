using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeMapLifecycleTests
{
    // A descriptor that writes no field takes the native creation defaults, which the map then
    // publishes.
    [BindingSpecTest("BND-100")]
    [Fact]
    public void DefaultMapOptionsPreserveNativeCreationDefaults()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions());

        var snapshot = map.GetSnapshot();
        Assert.Equal(new LogicalExtent(256, 256, 1), snapshot.LogicalExtent);
        Assert.Equal(DebugOptions.None, snapshot.DebugOptions);
        Assert.False(snapshot.RenderingStatsViewEnabled);
        Assert.False(snapshot.GestureInProgress);
    }

    [BindingSpecTest("BND-100")]
    [Fact]
    public void MapOptionsMaterializeFastPforDecoding()
    {
        Assert.Equal(0, new MapOptions().ToNative().fast_pfor_enabled);
        Assert.Equal(
            1,
            new MapOptions { FastPforEnabled = true }
                .ToNative()
                .fast_pfor_enabled
        );

        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions
            {
                Width = 128,
                Height = 64,
                FastPforEnabled = true,
            }
        );

        Assert.Equal(new LogicalExtent(128, 64, 1), map.GetSnapshot().LogicalExtent);
    }

    [BindingSpecTest("BND-040", "BND-100")]
    [Fact]
    public void RuntimeAndMapCloseDeterministically()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        Assert.False(runtime.IsClosed);
        Assert.False(map.IsClosed);

        map.Close();
        runtime.Close();

        Assert.True(map.IsClosed);
        Assert.True(runtime.IsClosed);
    }

    [BindingSpecTest("BND-040")]
    [Fact]
    public async Task RuntimeCloseAsyncCompletesAfterNativeTeardown()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = await MapHandle.CreateAsync(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
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

    [BindingSpecTest("BND-040")]
    [Fact]
    public async Task RuntimeDisposeAsyncWaitsForNativeTeardown()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        await using (runtime)
        {
            await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        }

        Assert.True(runtime.IsClosed);
    }

    [BindingSpecTest("BND-042")]
    [Fact]
    public void RuntimeCloseFailsWhileMapIsLiveAndCanRetryAfterMapClose()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
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

    [BindingSpecTest("BND-100")]
    [Fact]
    public void CommittedCommandIsVisibleInSnapshotsAtOrPastItsGeneration()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var options = DebugOptions.TileBorders | DebugOptions.ParseStatus;
        var completion = RuntimeEventTestHelpers.AssertCommitted(
            map.SetDebugOptionsAsync(options, TestContext.Current.CancellationToken)
        );

        // The published snapshot fence: a snapshot at or past the commit's generation
        // observes the committed value.
        var snapshot = map.GetSnapshot();
        Assert.True(snapshot.Generation >= completion.Generation);
        Assert.Equal(options, snapshot.DebugOptions);
        Assert.False(snapshot.FullyLoaded);
    }

    [BindingSpecTest("BND-100")]
    [Fact]
    public void DumpingDebugLogsCommits()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        RuntimeEventTestHelpers.AssertCommitted(
            map.DumpDebugLogsAsync(TestContext.Current.CancellationToken)
        );
    }

    // A still image stays pending until a render session produces it, so closing the map with
    // no session attached retires the request and reports the cancelled status.
    [BindingSpecTest("BND-041")]
    [Fact]
    public async Task AnOutstandingRequestIsCancelledWhenTheMapCloses()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = TestHandles.CreateMap(
            runtime,
            new MapOptions
            {
                Width = 64,
                Height = 64,
                MapMode = MapMode.Static,
            }
        );

        var stillImage = map.RequestStillImageAsync(TestContext.Current.CancellationToken);
        Assert.False(stillImage.IsCompleted);

        await map.CloseAsync();

        var error = await Assert.ThrowsAsync<MaplibreException>(() => stillImage);
        Assert.Equal(MaplibreStatus.Cancelled, error.Status);
    }

    [BindingSpecTest("BND-100")]
    [Fact]
    public void RenderingStatsViewEnabledRoundTripsThroughSnapshot()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        Assert.False(map.GetSnapshot().RenderingStatsViewEnabled);
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetRenderingStatsViewEnabledAsync(true, TestContext.Current.CancellationToken)
        );
        Assert.True(map.GetSnapshot().RenderingStatsViewEnabled);
    }

    [Fact]
    public void MapSizeReportsCreationExtentAndPixelRatio()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions
            {
                Width = 512,
                Height = 256,
                ScaleFactor = 2,
            }
        );

        Assert.Equal(new LogicalExtent(512, 256, 2), map.GetSnapshot().LogicalExtent);
    }

    [BindingSpecTest("BND-190", "BND-191")]
    [Fact]
    public async Task RuntimeAndMapWorkAcrossManagedThreads()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = await MapHandle.CreateAsync(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var completion = await Task.Run(() => map.RequestRepaintAsync());
        var snapshot = await Task.Run(map.GetSnapshot);
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        Assert.Equal(CommandDisposition.Committed, completion.Disposition);
        Assert.Equal(new LogicalExtent(512, 512, 1), snapshot.LogicalExtent);
    }

    [BindingSpecTest("BND-023")]
    [Fact]
    public void MethodsRejectClosedMapBeforeNativeCall()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 512, Height = 512 });
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
