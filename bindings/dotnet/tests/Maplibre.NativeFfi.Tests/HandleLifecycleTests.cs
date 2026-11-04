using System.Runtime.CompilerServices;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The abandoning helpers must not be async, or a state machine roots what they drop.

public sealed class HandleLifecycleTests
{
    [Fact]
    public async Task ASecondReleaseFailsAndADisposeAfterReleaseDoesNothing()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        await map.Ready.WaitAsync(TestWaits.Token);

        await map.ReleaseAsync();
        Assert.Equal(
            Status.InvalidState,
            Assert
                .Throws<MaplibreException>(() =>
                {
                    _ = map.ReleaseAsync();
                })
                .Status
        );
        // A scope that exits after an explicit release does nothing.
        await ((IAsyncDisposable)map).DisposeAsync();
        await ((IAsyncDisposable)map).DisposeAsync();
        map.Dispose();
        map.Dispose();

        await runtime.ReleaseAsync();
        Assert.Equal(
            Status.InvalidState,
            Assert
                .Throws<MaplibreException>(() =>
                {
                    _ = runtime.ReleaseAsync();
                })
                .Status
        );
        runtime.Dispose();
        runtime.Dispose();
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
        projection.Release();
        Assert.True(projection.IsClosed);
        Assert.Equal($"MapProjectionHandle(id={projectionId}, closed=true)", projection.ToString());
        ((IDisposable)projection).Dispose();

        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var runtimeId = $"0x{runtime.Id:x}";
        Assert.False(runtime.IsClosed);
        Assert.Equal($"RuntimeHandle(id={runtimeId}, closed=false)", runtime.ToString());
        runtime.Dispose();
        Assert.True(runtime.IsClosed);
        Assert.Equal($"RuntimeHandle(id={runtimeId}, closed=true)", runtime.ToString());
        // A second dispose and a scope that exits after the dispose do nothing.
        runtime.Dispose();
        await ((IAsyncDisposable)runtime).DisposeAsync();
    }

    [Fact]
    public async Task AHandleIdNamesItsEventsAndOutlivesItsRelease()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        await fixture.Map.SetStyleJsonAsync(NativeFixture.EmptyStyle);
        var loaded = await fixture.WaitForMapEventAsync(RuntimeEventType.MapStyleLoaded);
        Assert.Equal(fixture.Map.Id, loaded.Source);

        await fixture.Map.ReleaseAsync();
        // A host routes events that it drains after the release by the id it kept.
        Assert.Equal(loaded.Source, fixture.Map.Id);
    }

    [Fact]
    public async Task ARefusedReleaseLeavesTheHandleUsableAndARetrySucceeds()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = runtime.CreateMap(NativeFixture.SmallMap);
        await map.Ready.WaitAsync(TestWaits.Token);

        var error = Assert.Throws<MaplibreException>(() =>
        {
            _ = runtime.ReleaseAsync();
        });
        Assert.Equal(Status.InvalidState, error.Status);
        Assert.Contains("live or pending children", error.Diagnostic, StringComparison.Ordinal);
        await runtime.ClearResourceTransformAsync();

        await map.ReleaseAsync();
        await runtime.ReleaseAsync();
    }

    [Fact]
    public async Task AReleasedHandlePassesItsStaleIdToNative()
    {
        var runtime = RuntimeHandle.Create(new RuntimeOptions());
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        await map.Ready.WaitAsync(TestWaits.Token);
        await map.ReleaseAsync();
        await runtime.ReleaseAsync();

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
        await map.ReleaseAsync();
        await ((RuntimeHandle)runtime.Target!).ReleaseAsync();
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
