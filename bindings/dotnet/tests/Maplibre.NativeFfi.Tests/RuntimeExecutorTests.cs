using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class RuntimeExecutorTests
{
    [BindingSpecTest("BND-088")]
    [Fact]
    public async Task NativeWorkProgressesAutonomously()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = await MapHandle.CreateAsync(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );

        _ = map.SetStyleUrlAsync("unsupported://style.json", TestContext.Current.CancellationToken);
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);

        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapLoadingFailed);
    }

    // A completion is delivered by the runtime itself, so a host that never drains events still
    // observes every barrier it awaits.
    [Fact]
    public async Task RepeatedCompletionWaitsNeedNoEventDrain()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        for (var index = 0; index < 256; index++)
        {
            await runtime.BarrierAsync(TestContext.Current.CancellationToken);
        }

        Assert.Empty(runtime.DrainEvents());
    }
}
