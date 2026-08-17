using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeMapLifecycleTests
{
    [BindingSpecTest("BND-100")]
    [Fact]
    public void DefaultMapOptionsPreserveNativeCreationDefaults()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions());

        Assert.False(map.IsClosed);
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

        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { FastPforEnabled = true });

        Assert.False(map.IsClosed);
    }

    [BindingSpecTest("BND-040", "BND-100")]
    [Fact]
    public void RuntimeAndMapCloseDeterministically()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        Assert.False(runtime.IsClosed);
        Assert.False(map.IsClosed);

        TestHandles.Close(map);
        TestHandles.Close(runtime);

        Assert.True(map.IsClosed);
        Assert.True(runtime.IsClosed);
    }

    [BindingSpecTest("BND-042")]
    [Fact]
    public void RuntimeCloseFailsWhileMapIsLiveAndCanRetryAfterMapClose()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var error = Assert.Throws<InvalidStateException>(() => TestHandles.Close(runtime));

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Equal((int)MaplibreStatus.InvalidState, error.RawStatus);
        Assert.False(runtime.IsClosed);
        Assert.Contains(
            "live or pending children",
            error.Diagnostic,
            StringComparison.OrdinalIgnoreCase
        );

        TestHandles.Close(map);
        TestHandles.Close(runtime);

        Assert.True(runtime.IsClosed);
    }

    [BindingSpecTest("BND-100")]
    [Fact]
    public void CommittedCommandIsVisibleInSnapshotsAtOrPastItsGeneration()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        var options = DebugOptions.TileBorders | DebugOptions.ParseStatus;
        var completion = RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetDebugOptions(options),
            MaplibreStatus.Ok
        );

        // The published snapshot fence: a snapshot at or past the commit's generation
        // observes the committed value.
        var snapshot = map.GetSnapshot();
        Assert.True(snapshot.Generation >= completion.Generation);
        Assert.Equal(options, snapshot.DebugOptions);
        Assert.False(snapshot.FullyLoaded);
        map.DumpDebugLogs();
    }

    [BindingSpecTest("BND-100")]
    [Fact]
    public void RenderingStatsViewEnabledRoundTripsThroughSnapshot()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        Assert.False(map.GetSnapshot().RenderingStatsViewEnabled);
        RuntimeEventTestHelpers.AssertCommandFinishes(
            runtime,
            map.SetRenderingStatsViewEnabled(true),
            MaplibreStatus.Ok
        );
        Assert.True(map.GetSnapshot().RenderingStatsViewEnabled);
    }

    [Fact]
    public void MapSizeReportsCreationExtentAndPixelRatio()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
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

    [BindingSpecTest("BND-190", "BND-191", "BND-192")]
    [Fact]
    public async Task RuntimeAndMapWorkAcrossManagedThreads()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = await MapHandle.CreateAsync(
            runtime,
            new MapOptions { Width = 512, Height = 512 },
            TestContext.Current.CancellationToken
        );

        var commandId = await Task.Run(map.RequestRepaint);
        var snapshot = await Task.Run(map.GetSnapshot);
        await Task.Run(() => runtime.BarrierAsync(TestContext.Current.CancellationToken));

        Assert.NotEqual(0ul, commandId);
        Assert.Equal(new LogicalExtent(512, 512, 1), snapshot.LogicalExtent);
    }

    [BindingSpecTest("BND-023")]
    [Fact]
    public void MethodsRejectClosedMapBeforeNativeCall()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 512, Height = 512 });
        TestHandles.Close(map);
        TestHandles.Close(runtime);

        var error = Assert.Throws<InvalidStateException>(() =>
        {
            _ = map.RequestRepaint();
        });

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains("closed", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }
}
