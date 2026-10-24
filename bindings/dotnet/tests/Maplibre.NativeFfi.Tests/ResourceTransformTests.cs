using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class ResourceTransformTests
{
    [Fact]
    public unsafe void AScopedResponseExpiresWithItsCallbackAndRejectsOtherThreads()
    {
        HttpHeaderTransformResponse? retained = null;
        string? copied = null;
        Exception? crossThread = null;
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeHttpHeaderTransform(
            new HttpHeaderTransform
            {
                Callback = (_, url, response) =>
                {
                    copied = url;
                    retained = response;
                    var otherThread = new Thread(() =>
                        crossThread = Record.Exception(() => response.SetHeader("X-Test", "other"))
                    );
                    otherThread.Start();
                    otherThread.Join();
                    throw new FormatException("Host callback failed.");
                },
            },
            scope
        );
        var response = new mln_http_header_transform_response
        {
            size = (uint)sizeof(mln_http_header_transform_response),
        };
        var url = scope.CString("transform-test://é/style.json");
        Assert.Equal(
            mln_status.MLN_STATUS_NATIVE_ERROR,
            native.callback(native.user_data, 1, url, &response)
        );
        Assert.Equal("transform-test://é/style.json", copied);
        Assert.IsType<MaplibreException>(crossThread);
        Assert.NotNull(retained);
        Assert.Throws<MaplibreException>(() => retained.SetHeader("X-Test", "late"));
    }

    [Fact]
    public async Task ATransformRequestIsAnsweredFromAnotherThreadAfterItsCallback()
    {
        const string styleUrl = "transform-test://style.json";
        var received = new TaskCompletionSource<ResourceTransformRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        // The provider passes the style through to the online source, which waits for the answer.
        var deny = NativeFixture.DenyingProvider.Callback!;
        await using var fixture = await NativeFixture.CreateAsync(
            provider: new ResourceProvider
            {
                Callback = (request, handle) =>
                    request.RequestedUrl == styleUrl
                        ? ResourceProviderDecision.PassThrough
                        : deny(request, handle),
            }
        );
        await fixture.Runtime.SetResourceTransformAsync(
            new ResourceTransform(
                (_, url, request) =>
                {
                    if (url == styleUrl)
                        received.TrySetResult(request);
                    else
                        request.Dispose();
                }
            ),
            TestWaits.Token
        );
        await fixture.Map.SetStyleUrlAsync(styleUrl, TestWaits.Token);
        var request = await received.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        await Task.Run(() =>
        {
            request.Complete("unsupported://rewritten.json");
            request.Dispose();
        });
        // The online source cannot load the rewritten scheme.
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapLoadingFailed);
        await fixture.Runtime.ClearResourceTransformAsync(TestWaits.Token);
    }
}
