using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class RuntimeEventDrainTests
{
    private static readonly byte[] StyleJson =
        """{"version":8,"sources":{},"layers":[]}"""u8.ToArray();

    [BindingSpecTest("BND-090")]
    [Fact]
    public void OneDrainReportsEveryEventAStyleLoadProducedInQueueOrder()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleJson(StyleJson);
        var batch = DriveUntil(runtime, RuntimeEventType.MapStyleLoaded).Batch;

        var types = batch.Select(runtimeEvent => runtimeEvent.Type).ToArray();
        Assert.True(types.Length > 1, $"one drain reported {types.Length} events");
        Assert.Contains(RuntimeEventType.MapLoadingStarted, types);
        Assert.True(
            Array.IndexOf(types, RuntimeEventType.MapLoadingStarted)
                < Array.IndexOf(types, RuntimeEventType.MapStyleLoaded),
            "the loading-started event followed the style-loaded event"
        );
        Assert.All(batch, runtimeEvent => Assert.Same(map, runtimeEvent.MapSource));
    }

    [BindingSpecTest("BND-090")]
    [Fact]
    public void ABoundedDrainReportsRemainingEventsAndASecondDrainReachesZero()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson(StyleJson);

        var bounded = DriveUntilABoundedDrainLeavesEvents(runtime);
        Assert.Single(bounded.Events);
        Assert.True(bounded.RemainingCount > 0, "a bounded drain took the whole queue");

        // No pump runs between the two drains, so the second one takes exactly what the first
        // one left.
        var rest = runtime.DrainEvents();
        Assert.Equal((int)bounded.RemainingCount, rest.Events.Count);
        Assert.Equal(0ul, rest.RemainingCount);
    }

    [BindingSpecTest("BND-092")]
    [Fact]
    public void ADrainedBatchKeepsItsMessagesAfterTheNextDrainReusesTheArena()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        map.SetStyleUrl("first-unsupported-scheme://style.json");
        var first = DriveUntil(runtime, RuntimeEventType.MapLoadingFailed).Batch;
        var failure = first.Single(runtimeEvent =>
            runtimeEvent.Type == RuntimeEventType.MapLoadingFailed
        );
        Assert.Contains("first-unsupported-scheme", failure.Message, StringComparison.Ordinal);

        // The second drain refills the arena the first batch was copied from.
        map.SetStyleUrl("second-unsupported-scheme://style.json");
        var second = DriveUntil(runtime, RuntimeEventType.MapLoadingFailed).Batch;
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
    [BindingSpecTest("BND-091")]
    [Fact]
    public void TheDefaultMaskReportsAllAndDeliversEveryDrivenType()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        Assert.Equal(RuntimeEventMask.All, runtime.GetEventMask());
        Assert.Equal(RuntimeEventMask.All, map.GetEventMask());

        map.SetStyleJson(StyleJson);
        map.JumpTo(new CameraOptions { Zoom = 4 });
        var types = DriveUntil(runtime, RuntimeEventType.MapStyleLoaded)
            .Everything.Select(runtimeEvent => runtimeEvent.Type)
            .ToArray();

        Assert.Contains(RuntimeEventType.MapLoadingStarted, types);
        Assert.Contains(RuntimeEventType.MapStyleLoaded, types);
        Assert.Contains(RuntimeEventType.MapCameraDidChange, types);
    }

    [BindingSpecTest("BND-091")]
    [Fact]
    public void AClearedEventTypeNeverReachesABatch()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetEventMask(RuntimeEventMask.All & ~RuntimeEventMask.MapLoadingStarted);

        map.SetStyleJson(StyleJson);
        var types = DriveUntil(runtime, RuntimeEventType.MapStyleLoaded)
            .Everything.Select(runtimeEvent => runtimeEvent.Type)
            .ToArray();

        Assert.DoesNotContain(RuntimeEventType.MapLoadingStarted, types);
        Assert.Contains(RuntimeEventType.MapStyleLoaded, types);
    }

    [BindingSpecTest("BND-091")]
    [Fact]
    public void AMaskRoundTripsAndAReadModifyWriteKeepsTheOtherBits()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        runtime.SetEventMask(RuntimeEventMask.All);
        map.SetEventMask(RuntimeEventMask.All);
        Assert.Equal(RuntimeEventMask.All, runtime.GetEventMask());
        Assert.Equal(RuntimeEventMask.All, map.GetEventMask());

        map.SetEventMask(map.GetEventMask() & ~RuntimeEventMask.MapIdle);
        Assert.Equal(RuntimeEventMask.All & ~RuntimeEventMask.MapIdle, map.GetEventMask());

        runtime.SetEventMask(runtime.GetEventMask() & ~RuntimeEventMask.OfflineOperationCompleted);
        Assert.Equal(
            RuntimeEventMask.All & ~RuntimeEventMask.OfflineOperationCompleted,
            runtime.GetEventMask()
        );
    }

    [BindingSpecTest("BND-091")]
    [Fact]
    public void AMaskBitOutsideAllIsRejected()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        var undeclared = (RuntimeEventMask)(1UL << 63);

        Assert.Throws<InvalidArgumentException>(() => runtime.SetEventMask(undeclared));
        Assert.Throws<InvalidArgumentException>(() => map.SetEventMask(undeclared));
        Assert.Throws<InvalidArgumentException>(() =>
            MapHandle.Create(
                runtime,
                new MapOptions
                {
                    Width = 512,
                    Height = 512,
                    EventMask = undeclared,
                }
            )
        );
        // The options mask is validated before the owner thread's live runtime is, so this
        // rejection proves RuntimeOptions.EventMask reaches the native options struct.
        Assert.Throws<InvalidArgumentException>(() =>
            RuntimeHandle.Create(new RuntimeOptions { EventMask = undeclared })
        );
    }

    [BindingSpecTest("BND-092")]
    [Fact]
    public void TheDrainAndBothMaskSettersRejectAThreadThatDoesNotOwnTheRuntime()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        var thrown = new List<Exception?>();

        var thread = new Thread(() =>
        {
            thrown.Add(Record.Exception(() => runtime.DrainEvents()));
            thrown.Add(Record.Exception(() => runtime.SetEventMask(RuntimeEventMask.All)));
            thrown.Add(Record.Exception(() => map.SetEventMask(RuntimeEventMask.All)));
        });
        thread.Start();
        thread.Join();

        Assert.All(
            thrown,
            error =>
                Assert.Equal(
                    MaplibreStatus.WrongThread,
                    Assert.IsType<WrongThreadException>(error).Status
                )
        );
    }

    /// <param name="Batch">The one batch that carried the awaited type.</param>
    /// <param name="Everything">Every event drained up to and including that batch.</param>
    private sealed record DrivenEvents(
        IReadOnlyList<RuntimeEvent> Batch,
        IReadOnlyList<RuntimeEvent> Everything
    );

    // Pumps until one drain of a single event leaves the queue non-empty.
    private static RuntimeEventBatch DriveUntilABoundedDrainLeavesEvents(RuntimeHandle runtime)
    {
        for (var attempt = 0; attempt < 1000; attempt++)
        {
            runtime.Pump(TimeSpan.Zero);
            var batch = runtime.DrainEvents(1);
            if (batch.RemainingCount > 0)
            {
                return batch;
            }

            Thread.Sleep(1);
        }

        throw new TimeoutException("The style load never queued more than one event.");
    }

    // Pumps and drains until one batch carries the awaited type.
    private static DrivenEvents DriveUntil(RuntimeHandle runtime, RuntimeEventType eventType)
    {
        var everything = new List<RuntimeEvent>();
        for (var attempt = 0; attempt < 1000; attempt++)
        {
            runtime.Pump(TimeSpan.Zero);
            var batch = runtime.DrainEvents();
            everything.AddRange(batch.Events);
            if (batch.Events.Any(runtimeEvent => runtimeEvent.Type == eventType))
            {
                return new DrivenEvents(batch.Events, everything);
            }

            Thread.Sleep(1);
        }

        throw new TimeoutException($"Timed out waiting for {eventType}.");
    }
}
