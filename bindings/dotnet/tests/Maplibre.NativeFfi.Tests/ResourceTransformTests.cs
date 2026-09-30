using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class ResourceTransformTests
{
    [Fact]
    public unsafe void GeneratedResponseExpiresAndRejectsOtherThreads()
    {
        ResourceTransformResponse? retained = null;
        string? copied = null;
        Exception? crossThread = null;
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeResourceTransform(
            new ResourceTransform
            {
                Callback = (_, url, response) =>
                {
                    copied = url;
                    retained = response;
                    var otherThread = new Thread(() =>
                        crossThread = Record.Exception(() => response.SetUrl("other"))
                    );
                    otherThread.Start();
                    otherThread.Join();
                    throw new FormatException("Host callback failed.");
                },
            },
            scope
        );
        var response = new mln_resource_transform_response
        {
            size = (uint)sizeof(mln_resource_transform_response),
        };
        var url = scope.CString("https://example.test/é");
        Assert.Equal(
            mln_status.MLN_STATUS_NATIVE_ERROR,
            native.callback(native.user_data, 1, url, &response)
        );
        Assert.Equal("https://example.test/é", copied);
        Assert.IsType<InvalidOperationException>(crossThread);
        Assert.NotNull(retained);
        Assert.Throws<InvalidOperationException>(() => retained.SetUrl("other"));
    }

    [Fact]
    public async Task InstalledTransformCopiesUnicodeResponseAndCanBeCleared()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(runtime, Map.MapOptions.Default);
        Exception? reentry = null;
        var transformed = new TaskCompletionSource<string>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await runtime.SetResourceTransformAsync(
            new ResourceTransform
            {
                Callback = (_, url, response) =>
                {
                    reentry = Record.Exception(() =>
                    {
                        runtime.CloseAsync();
                    });
                    response.SetUrl("transformed-test://é/style.json");
                    transformed.TrySetResult(url);
                },
            },
            TestContext.Current.CancellationToken
        );
        await map.SetStyleUrlAsync(
            "original-test://style.json",
            TestContext.Current.CancellationToken
        );
        Assert.Equal(
            "original-test://style.json",
            await transformed.Task.WaitAsync(
                TimeSpan.FromSeconds(10),
                TestContext.Current.CancellationToken
            )
        );
        await runtime.ClearResourceTransformAsync(TestContext.Current.CancellationToken);
        Assert.IsType<InvalidOperationException>(reentry);
        await runtime.BarrierAsync(TestContext.Current.CancellationToken);
    }
}
