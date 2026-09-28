using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeEventDrainTests
{
    [BindingSpecTest("")]
    [Fact]
    public void OneDrainReportsEveryEventAStyleLoadProducedInQueueOrder()
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
        var batch = DriveUntil(runtime, RuntimeEventType.MapStyleLoaded).LastBatch;

        var types = batch.Select(runtimeEvent => runtimeEvent.Type).ToArray();
        Assert.True(types.Length > 1, $"one drain reported {types.Length} events");
        Assert.Contains(RuntimeEventType.MapLoadingStarted, types);
        Assert.True(
            Array.IndexOf(types, RuntimeEventType.MapLoadingStarted)
                < Array.IndexOf(types, RuntimeEventType.MapStyleLoaded),
            "the loading-started event followed the style-loaded event"
        );
        Assert.All(batch, runtimeEvent => Assert.Equal(map.NativeId, runtimeEvent.Source));
    }

    [BindingSpecTest("")]
    [Fact]
    public void ADrainedBatchKeepsItsMessagesAfterTheNextDrainReusesTheArena()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        map.SetStyleUrlAsync(
            "first-unsupported-scheme://style.json",
            TestContext.Current.CancellationToken
        );
        var first = DriveUntil(runtime, RuntimeEventType.MapLoadingFailed).LastBatch;
        var failure = first.Single(runtimeEvent =>
            runtimeEvent.Type == RuntimeEventType.MapLoadingFailed
        );
        Assert.Contains("first-unsupported-scheme", failure.Message, StringComparison.Ordinal);

        // The second drain refills the arena the first batch was copied from.
        map.SetStyleUrlAsync(
            "second-unsupported-scheme://style.json",
            TestContext.Current.CancellationToken
        );
        var second = DriveUntil(runtime, RuntimeEventType.MapLoadingFailed).LastBatch;
        Assert.Contains(
            "second-unsupported-scheme",
            second
                .Single(runtimeEvent => runtimeEvent.Type == RuntimeEventType.MapLoadingFailed)
                .Message,
            StringComparison.Ordinal
        );
        Assert.Contains("first-unsupported-scheme", failure.Message, StringComparison.Ordinal);
    }

    // A host that writes no mask takes the property default, which selects every type.
    [BindingSpecTest("")]
    [Fact]
    public void TheDefaultMaskReportsAllAndDeliversEveryDrivenType()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        Assert.Equal(RuntimeEventMask.All, runtime.GetEventMask());
        Assert.Equal(RuntimeEventMask.All, map.SnapshotGet().EventMask);

        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        map.UpdateCameraAsync(
            CameraUpdate.Default with
            {
                Mode = CameraUpdateMode.Jump,
                Camera = new CameraOptions { Zoom = 4 },
            },
            TestContext.Current.CancellationToken
        );
        var types = DriveUntil(runtime, RuntimeEventType.MapStyleLoaded)
            .All.Select(runtimeEvent => runtimeEvent.Type)
            .ToArray();

        Assert.Contains(RuntimeEventType.MapLoadingStarted, types);
        Assert.Contains(RuntimeEventType.MapStyleLoaded, types);
        Assert.Contains(RuntimeEventType.MapCameraDidChange, types);
    }

    [BindingSpecTest("")]
    [Fact]
    public void AClearedEventTypeNeverReachesABatch()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        map.SetEventMaskAsync(
            RuntimeEventMask.All & ~RuntimeEventMask.MapLoadingStarted,
            TestContext.Current.CancellationToken
        );

        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var types = DriveUntil(runtime, RuntimeEventType.MapStyleLoaded)
            .All.Select(runtimeEvent => runtimeEvent.Type)
            .ToArray();

        Assert.DoesNotContain(RuntimeEventType.MapLoadingStarted, types);
        Assert.Contains(RuntimeEventType.MapStyleLoaded, types);
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task AMaskRoundTripsAndAReadModifyWriteKeepsTheOtherBits()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        _ = map.SetEventMaskAsync(
            map.SnapshotGet().EventMask & ~RuntimeEventMask.MapIdle,
            TestContext.Current.CancellationToken
        );
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        Assert.Equal(RuntimeEventMask.All & ~RuntimeEventMask.MapIdle, map.SnapshotGet().EventMask);

        runtime.SetEventMask(runtime.GetEventMask() & ~RuntimeEventMask.OfflineRegionStatusChanged);
        Assert.Equal(
            RuntimeEventMask.All & ~RuntimeEventMask.OfflineRegionStatusChanged,
            runtime.GetEventMask()
        );
    }

    [BindingSpecTest("")]
    [Fact]
    public void AMaskBitOutsideAllIsRejected()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        var undeclared = (RuntimeEventMask)(1UL << 63);

        Assert.Throws<InvalidArgumentException>(() => runtime.SetEventMask(undeclared));
        Assert.Throws<InvalidArgumentException>(() =>
            map.SetEventMaskAsync(undeclared, TestContext.Current.CancellationToken)
                .GetAwaiter()
                .GetResult()
        );
        Assert.Throws<InvalidArgumentException>(() =>
            TestHandles.CreateMap(
                runtime,
                MapOptions.Default with
                {
                    InitialExtent = MapOptions.Default.InitialExtent with
                    {
                        Width = 512,
                        Height = 512,
                    },

                    EventMask = undeclared,
                }
            )
        );
        // The options mask is validated during autonomous creation, so this rejection proves
        // RuntimeOptions.EventMask reaches the native options struct.
        Assert.Throws<InvalidArgumentException>(() =>
            RuntimeHandle.Create(RuntimeOptions.Default with { EventMask = undeclared })
        );
    }

    [BindingSpecTest("")]
    [Fact]
    public void EventOperationsAreAnyThread()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        var thrown = new List<Exception?>();

        var thread = new Thread(() =>
        {
            thrown.Add(Record.Exception(() => runtime.DrainEventCopies()));
            thrown.Add(Record.Exception(() => runtime.SetEventMask(RuntimeEventMask.All)));
            thrown.Add(
                Record.Exception(() =>
                {
                    _ = map.SetEventMaskAsync(
                        RuntimeEventMask.All,
                        TestContext.Current.CancellationToken
                    );
                })
            );
        });
        thread.Start();
        thread.Join();

        Assert.All(thrown, Assert.Null);
    }

    // Drains until one batch carries the awaited type.
    private static RuntimeEventTestHelpers.DrainedEvents DriveUntil(
        RuntimeHandle runtime,
        RuntimeEventType eventType
    ) =>
        RuntimeEventTestHelpers.DrainUntil(
            runtime,
            batch => batch.Any(runtimeEvent => runtimeEvent.Type == eventType)
        );
}
