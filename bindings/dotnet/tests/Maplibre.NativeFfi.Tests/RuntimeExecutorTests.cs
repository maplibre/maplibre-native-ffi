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
            new MapOptions { Width = 512, Height = 512 },
            TestContext.Current.CancellationToken
        );

        map.SetStyleUrlAsync("unsupported://style.json");
        await runtime.BarrierAsync();

        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapLoadingFailed);
    }

    [BindingSpecTest("BND-089")]
    [Fact]
    public async Task CrossThreadCommandsProgressAutonomously()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = await MapHandle.CreateAsync(
            runtime,
            new MapOptions { Width = 512, Height = 512 },
            TestContext.Current.CancellationToken
        );

        var completion = await Task.Run(map.RequestRepaintAsync);
        await runtime.BarrierAsync();

        Assert.Equal(CommandDisposition.Committed, completion.Disposition);
    }

    [Fact]
    public async Task RepeatedCompletionWaitsNeedNoEventDrain()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        for (var index = 0; index < 256; index++)
        {
            await runtime.BarrierAsync();
        }
    }
}
