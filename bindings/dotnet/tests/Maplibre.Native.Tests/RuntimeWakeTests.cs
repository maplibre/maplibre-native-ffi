using Maplibre.Native.Map;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class RuntimeWakeTests
{
    private static readonly TimeSpan ParkTimeout = TimeSpan.FromSeconds(10);

    // Leaves the runtime idle with no latched signal, so a following park can only
    // be released by the signal the test raises.
    private static void DrainLatchedWakes(RuntimeHandle runtime)
    {
        for (var attempt = 0; attempt < 100; attempt++)
        {
            if (!runtime.Wait(TimeSpan.Zero))
            {
                return;
            }

            runtime.RunOnce();
            while (runtime.PollEvent() is not null) { }
        }

        Assert.Fail("The runtime kept latching wakes while idle.");
    }

    [BindingSpecTest("BND-088")]
    [Fact]
    public void ParkedOwnerThreadWakesForNativeWorkAndForAWakeSource()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        DrainLatchedWakes(runtime);

        // The style is malformed, so native reports the failure from its own
        // threads. What matters here is that the failure reaches a parked owner
        // thread at all.
        map.SetStyleUrl("unsupported://style.json");
        var loadingFailed = false;
        for (var attempt = 0; attempt < 20 && !loadingFailed; attempt++)
        {
            Assert.True(runtime.Wait(ParkTimeout), "A park timed out while loading was pending.");
            runtime.RunOnce();
            while (runtime.PollEvent() is { } polled)
            {
                if (polled.Type == RuntimeEventType.MapLoadingFailed)
                {
                    loadingFailed = true;
                }
            }
        }

        Assert.True(loadingFailed);

        // A source signalled from another thread is what a host's submission path
        // holds, and the park it releases has no other work to end it.
        var source = runtime.AcquireWakeSource();
        DrainLatchedWakes(runtime);
        var signaller = new Thread(() =>
        {
            Thread.Sleep(TimeSpan.FromMilliseconds(20));
            source.Signal();
        });
        signaller.Start();

        Assert.True(
            runtime.Wait(ParkTimeout),
            "The parked owner thread timed out instead of taking the signal."
        );
        signaller.Join();

        // A wake source stays usable once its runtime is gone, so host teardown
        // ordering is free.
        map.Close();
        runtime.Close();
        source.Signal();
        source.Close();
        Assert.True(source.IsClosed);
    }

    [BindingSpecTest("BND-089")]
    [Fact]
    public void WaitConsumesOneLatchedSignalAtATime()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var source = runtime.AcquireWakeSource();
        DrainLatchedWakes(runtime);

        source.Signal();
        Assert.True(runtime.Wait(TimeSpan.Zero));
        // The latch is consumed, so an idle runtime reports the timeout instead.
        Assert.False(runtime.Wait(TimeSpan.Zero));
    }
}
