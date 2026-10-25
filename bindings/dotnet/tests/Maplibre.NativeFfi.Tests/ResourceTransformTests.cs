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
        var url = scope.Utf8("transform-test://é/style.json");
        Assert.Equal(
            mln_status.MLN_STATUS_NATIVE_ERROR,
            native.callback(
                native.user_data,
                mln_resource_kind.MLN_RESOURCE_KIND_STYLE,
                url,
                &response
            )
        );
        Assert.Equal("transform-test://é/style.json", copied);
        Assert.IsType<MaplibreException>(crossThread);
        Assert.NotNull(retained);
        Assert.Throws<MaplibreException>(() => retained.SetHeader("X-Test", "late"));
    }

    // A header transform returns to succeed and throws to fail. A thrown MaplibreException with a
    // failing status is deliberate: native receives that status and nothing is reported. Any other
    // exception is reported and answers the native-error fallback.
    [Fact]
    public unsafe void AHeaderTransformAnswersOkTheStatusItThrowsOrItsFallback()
    {
        Exception? thrown = null;
        var reports = new List<Exception>();
        // Other tests' callbacks may throw concurrently, so only this test's exceptions count.
        EventHandler<CallbackExceptionEventArgs> record = (_, report) =>
        {
            if (ReferenceEquals(report.Exception, thrown))
                lock (reports)
                    reports.Add(report.Exception);
        };
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeHttpHeaderTransform(
            new HttpHeaderTransform
            {
                Callback = (_, _, _) =>
                {
                    if (thrown is not null)
                        throw thrown;
                },
            },
            scope
        );
        var response = new mln_http_header_transform_response
        {
            size = (uint)sizeof(mln_http_header_transform_response),
        };
        var url = scope.Utf8("transform-test://style.json");
        var responsePointer = &response;
        mln_status Invoke(Exception? exception)
        {
            thrown = exception;
            return native.callback(
                native.user_data,
                mln_resource_kind.MLN_RESOURCE_KIND_STYLE,
                url,
                responsePointer
            );
        }

        Maplibre.CallbackException += record;
        try
        {
            Assert.Equal(mln_status.MLN_STATUS_OK, Invoke(null));
            Assert.Equal(
                mln_status.MLN_STATUS_INVALID_ARGUMENT,
                Invoke(new MaplibreException(Status.InvalidArgument, "Deliberate."))
            );
            Assert.Empty(reports);
            var succeeded = new MaplibreException(Status.Ok, "Not a failure.");
            Assert.Equal(mln_status.MLN_STATUS_NATIVE_ERROR, Invoke(succeeded));
            Assert.Same(succeeded, Assert.Single(reports));
        }
        finally
        {
            Maplibre.CallbackException -= record;
        }
    }

    [Fact]
    public async Task ATransformRequestIsAnsweredFromAnotherThreadAfterItsCallback()
    {
        const string styleUrl = "transform-test://style.json";
        var received = new TaskCompletionSource<ResourceTransformRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        // The provider passes the style through to the online source, which waits for the answer.
        var deny = NativeFixture.DenyingProvider.Callback;
        await using var fixture = await NativeFixture.CreateAsync(
            provider: new ResourceProvider
            {
                Callback = (request, handle) =>
                {
                    if (request.RequestedUrl == styleUrl)
                        handle.PassThrough();
                    else
                        deny(request, handle);
                },
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
            )
        );
        await fixture.Map.SetStyleUrlAsync(styleUrl);
        var request = await received.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        await Task.Run(() => request.Complete("unsupported://rewritten.json"), TestWaits.Token);
        // The answer consumed the request, so a second answer fails.
        Assert.True(request.IsClosed);
        var error = Assert.Throws<MaplibreException>(() =>
            request.Complete("unsupported://again.json")
        );
        Assert.Equal(Status.InvalidState, error.Status);
        // The online source cannot load the rewritten scheme.
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapLoadingFailed);
        await fixture.Runtime.ClearResourceTransformAsync();
    }
}
