using System.Diagnostics;
using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

#pragma warning disable xUnit1031 // The abandoning helpers must not be async, or a state machine roots what they drop.

public sealed class HandleLifecycleTests
{
    [Fact]
    public async Task SynchronousCloseIsIdempotentAndAsyncReleaseReachesNative()
    {
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
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

        var disposed = RuntimeHandle.Create(RuntimeOptions.Default);
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

        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
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
    public async Task BorrowedCopiesAllowReadersAndRefuseConcurrentRelease()
    {
        var state = CreateCopyProtectedState();
        using var entered = new SemaphoreSlim(0);
        using var leave = new SemaphoreSlim(0);
        var elapsed = Stopwatch.StartNew();
        var reader = Task.Run(
            () =>
            {
                using var read = state.Read(state);
                entered.Release();
                TestWaits.Wait(leave, elapsed, "the borrowed copy to finish");
            },
            TestWaits.Token
        );
        try
        {
            await TestWaits.WaitAsync(entered, elapsed, "the borrowed copy to start");
            Assert.Equal(
                Status.InvalidState,
                Assert.Throws<MaplibreException>(() => state.Close()).Status
            );
            ReadAgain(state);
        }
        finally
        {
            leave.Release();
            await reader;
        }
        state.Close();
    }

    private static unsafe NativeHandleState<MlnRuntime> CreateCopyProtectedState() =>
        new(
            SyntheticHandles.Runtime(1),
            (_, _) => mln_status.MLN_STATUS_OK,
            "test batch",
            protectsCopies: true
        );

    private static void ReadAgain(NativeHandleState<MlnRuntime> state)
    {
        using var read = state.Read(state);
        Assert.Equal(SyntheticHandles.Runtime(1), read.Handle);
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
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
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
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
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
        var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var map = runtime.CreateMap(NativeFixture.SmallMap);
        map.Ready.GetAwaiter().GetResult();
        return (map, new WeakReference(runtime));
    }
}

#pragma warning restore xUnit1031
