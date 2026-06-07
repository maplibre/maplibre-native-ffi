using Maplibre.Native.Error;
using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class RuntimeMapLifecycleTests
{
    [Fact]
    public void DefaultMapOptionsPreserveNativeCreationDefaults()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime);

        Assert.False(map.IsClosed);
    }

    [Fact]
    public void RuntimeAndMapCloseDeterministically()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        Assert.False(runtime.IsClosed);
        Assert.False(map.IsClosed);

        map.Close();
        runtime.Close();

        Assert.True(map.IsClosed);
        Assert.True(runtime.IsClosed);
    }

    [Fact]
    public void MapDebugSettingsRoundTripThroughNativeMap()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var options = DebugOptions.TileBorders | DebugOptions.ParseStatus;
        map.SetDebugOptions(options);
        map.SetRenderingStatsViewEnabled(true);

        Assert.Equal(options, map.GetDebugOptions());
        Assert.True(map.GetRenderingStatsViewEnabled());
        _ = map.IsFullyLoaded();
        map.DumpDebugLogs();
    }

    [Fact]
    public void OwnerThreadViolationMapsToWrongThreadException()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
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

    [Fact]
    public void MethodsRejectClosedMapBeforeNativeCall()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
        var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.Close();
        runtime.Close();

        var error = Assert.Throws<InvalidStateException>(map.RequestRepaint);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains("closed", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }
}
