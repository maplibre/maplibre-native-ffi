using System.Runtime.CompilerServices;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The abandoning helpers must not be async, or a state machine roots what they drop.

public sealed class HandleLifecycleTests
{
    [Fact]
    public async Task SynchronousCloseIsIdempotentAndAsyncReleaseReachesNative()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        await map.Ready.WaitAsync(TestWaits.Token);

        await map.CloseAsync();
        Assert.Equal(
            Status.InvalidState,
            Assert
                .Throws<MaplibreException>(() =>
                {
                    _ = map.CloseAsync();
                })
                .Status
        );
        // A scope that exits after an explicit close does nothing.
        await map.DisposeAsync();
        await map.DisposeAsync();
        map.Dispose();
        map.Dispose();

        await runtime.CloseAsync();
        Assert.Equal(
            Status.InvalidState,
            Assert
                .Throws<MaplibreException>(() =>
                {
                    _ = runtime.CloseAsync();
                })
                .Status
        );
        runtime.Dispose();
        runtime.Dispose();

        var disposed = RuntimeHandle.Create(new RuntimeOptions());
        disposed.Dispose();
        await disposed.DisposeAsync();
    }

    [Fact]
    public async Task AHandleNamesItsIdAndWhetherItIsClosed()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        var projection = MapProjectionHandle.Create(fixture.Map.GetSnapshot().View);
        var projectionId = $"0x{projection.Id:x}";
        Assert.False(projection.IsClosed);
        Assert.Equal(
            $"MapProjectionHandle(id={projectionId}, closed=false)",
            projection.ToString()
        );
        projection.Dispose();
        Assert.True(projection.IsClosed);
        Assert.Equal($"MapProjectionHandle(id={projectionId}, closed=true)", projection.ToString());
        projection.Dispose();

        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var runtimeId = $"0x{runtime.Id:x}";
        Assert.False(runtime.IsClosed);
        Assert.Equal($"RuntimeHandle(id={runtimeId}, closed=false)", runtime.ToString());
        runtime.Dispose();
        Assert.True(runtime.IsClosed);
        Assert.Equal($"RuntimeHandle(id={runtimeId}, closed=true)", runtime.ToString());
        // A second dispose and a scope that exits after the dispose do nothing.
        runtime.Dispose();
        await runtime.DisposeAsync();
    }

    [Fact]
    public async Task AHandleIdNamesItsEventsAndOutlivesClose()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        await fixture.Map.SetStyleJsonAsync(NativeFixture.EmptyStyle);
        var loaded = await fixture.WaitForMapEventAsync(RuntimeEventType.MapStyleLoaded);
        Assert.Equal(fixture.Map.Id, loaded.Source);

        await fixture.Map.CloseAsync();
        // A host routes events that it drains after close by the id it kept.
        Assert.Equal(loaded.Source, fixture.Map.Id);
    }

    [Fact]
    public async Task ARefusedCloseLeavesTheHandleUsableAndARetrySucceeds()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = runtime.CreateMap(NativeFixture.SmallMap);
        await map.Ready.WaitAsync(TestWaits.Token);

        var error = Assert.Throws<MaplibreException>(() =>
        {
            _ = runtime.CloseAsync();
        });
        Assert.Equal(Status.InvalidState, error.Status);
        Assert.Contains("live or pending children", error.Diagnostic, StringComparison.Ordinal);
        await runtime.ClearResourceTransformAsync();

        await map.CloseAsync();
        await runtime.CloseAsync();
    }

    [Fact]
    public async Task AClosedHandlePassesItsStaleIdToNative()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        await map.Ready.WaitAsync(TestWaits.Token);
        await map.CloseAsync();
        await runtime.CloseAsync();

        var error = Assert.Throws<MaplibreException>(() =>
        {
            _ = map.RequestRepaintAsync();
        });

        Assert.Equal(Status.InvalidState, error.Status);
    }

    [Fact]
    public async Task AChildKeepsItsParentAlive()
    {
        var (map, runtime) = CreateMapOfUnreferencedRuntime();

        Gc.Collect();

        Assert.True(runtime.IsAlive);
        await map.SetStyleJsonAsync(NativeFixture.EmptyStyle);
        await map.CloseAsync();
        await ((RuntimeHandle)runtime.Target!).CloseAsync();
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (MapHandle Map, WeakReference Runtime) CreateMapOfUnreferencedRuntime()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        map.Ready.GetAwaiter().GetResult();
        return (map, new WeakReference(runtime));
    }
}

#pragma warning restore xUnit1031
