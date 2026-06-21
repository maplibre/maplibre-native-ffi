using Maplibre.Native.Error;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class RuntimeMapLifecycleTests
{
    [BindingSpecTest("BND-100")]
    [Fact]
    public void DefaultMapOptionsPreserveNativeCreationDefaults()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions());

        Assert.False(map.IsClosed);
    }

    [BindingSpecTest("BND-040", "BND-100")]
    [Fact]
    public void RuntimeAndMapCloseDeterministically()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        Assert.False(runtime.IsClosed);
        Assert.False(map.IsClosed);

        map.Close();
        runtime.Close();

        Assert.True(map.IsClosed);
        Assert.True(runtime.IsClosed);
    }

    [BindingSpecTest("BND-042")]
    [Fact]
    public void RuntimeCloseFailsWhileMapIsLiveAndCanRetryAfterMapClose()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var error = Assert.Throws<InvalidStateException>(runtime.Close);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Equal((int)MaplibreStatus.InvalidState, error.RawStatus);
        Assert.False(runtime.IsClosed);
        Assert.Contains("live maps", error.Diagnostic, StringComparison.OrdinalIgnoreCase);

        map.Close();
        runtime.Close();

        Assert.True(runtime.IsClosed);
    }

    [BindingSpecTest("BND-100")]
    [Fact]
    public void MapDebugSettingsRoundTripThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var options = DebugOptions.TileBorders | DebugOptions.ParseStatus;
        map.SetDebugOptions(options);
        map.SetRenderingStatsViewEnabled(true);

        Assert.Equal(options, map.GetDebugOptions());
        Assert.True(map.GetRenderingStatsViewEnabled());
        _ = map.IsFullyLoaded();
        map.DumpDebugLogs();
    }

    [BindingSpecTest("BND-190", "BND-191")]
    [Fact]
    public void OwnerThreadViolationMapsToWrongThreadException()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        Exception? thrown = null;

        var thread = new Thread(() =>
        {
            try
            {
                map.RequestRepaint();
            }
            catch (Exception error)
            {
                thrown = error;
            }
        });
        thread.Start();
        thread.Join();

        var wrongThread = Assert.IsType<WrongThreadException>(thrown);
        Assert.Equal(MaplibreStatus.WrongThread, wrongThread.Status);
        Assert.Equal((int)MaplibreStatus.WrongThread, wrongThread.RawStatus);
    }

    [BindingSpecTest("BND-192")]
    [Fact]
    public async Task OwnerThreadHelperConfinesRuntimeMapWorkToOneThread()
    {
        using var ownerThread = new OwnerThread();
        RuntimeHandle? runtime = null;
        MapHandle? map = null;

        var createThreadId = ownerThread.Invoke(() =>
        {
            runtime = RuntimeHandle.Create(new RuntimeOptions());
            map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
            return Environment.CurrentManagedThreadId;
        });

        var task = Task.Run(() =>
            ownerThread.Invoke(() =>
            {
                Assert.Equal(createThreadId, Environment.CurrentManagedThreadId);
                map!.RequestRepaint();
                runtime!.RunOnce();
                _ = runtime.PollEvent();
                return Environment.CurrentManagedThreadId;
            })
        );

        Assert.Equal(createThreadId, await task);
        ownerThread.Invoke(() =>
        {
            map!.Close();
            runtime!.Close();
        });
    }

    [BindingSpecTest("BND-192")]
    [Fact]
    public void OwnerThreadHelperPropagatesOrdinaryBindingErrors()
    {
        using var ownerThread = new OwnerThread();
        RuntimeHandle? runtime = null;
        MapHandle? map = null;
        ownerThread.Invoke(() =>
        {
            runtime = RuntimeHandle.Create(new RuntimeOptions());
            map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
            map.Close();
            runtime.Close();
        });

        var error = Assert.Throws<InvalidStateException>(() =>
            ownerThread.Invoke(() => map!.RequestRepaint())
        );

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(error.RawStatus);
    }

    [BindingSpecTest("BND-192")]
    [Fact]
    public void OwnerThreadHelperRejectsSubmissionsAfterClose()
    {
        var ownerThread = new OwnerThread();
        ownerThread.Close();

        var error = Assert.Throws<InvalidStateException>(() => ownerThread.Invoke(() => { }));

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(error.RawStatus);
    }

    [BindingSpecTest("BND-192")]
    [Fact]
    public void OwnerThreadHelperRejectsNestedSubmissionsAfterOwnerThreadClose()
    {
        using var ownerThread = new OwnerThread();

        ownerThread.Invoke(() =>
        {
            ownerThread.Close();

            var error = Assert.Throws<InvalidStateException>(() => ownerThread.Invoke(() => { }));

            Assert.Equal(MaplibreStatus.InvalidState, error.Status);
            Assert.Null(error.RawStatus);
        });
    }

    [BindingSpecTest("BND-023")]
    [Fact]
    public void MethodsRejectClosedMapBeforeNativeCall()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.Close();
        runtime.Close();

        var error = Assert.Throws<InvalidStateException>(map.RequestRepaint);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains("closed", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }
}
