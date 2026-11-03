using System.Collections.Concurrent;
using System.Collections.Immutable;
using System.Text;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class ResourceProviderTests
{
    private const string StyleUrl = "provider-test://é/style.json";

    private static ResourceResponse StyleResponse() =>
        new()
        {
            Status = ResourceResponseStatus.Ok,
            Bytes = [.. Encoding.UTF8.GetBytes(NativeFixture.EmptyStyle)],
            ErrorMessage = "",
            Etag = "",
        };

    [Fact]
    public async Task AProviderCanAnswerLaterThroughItsHandle()
    {
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await using var fixture = await NativeFixture.CreateAsync(provider: Holding(received));
        var loaded = fixture.Map.SetStyleUrlAsync(StyleUrl);
        var handle = await received.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        // Native rejects a malformed etag while the request stays open for another answer.
        Assert.Throws<MaplibreException>(() =>
            handle.Complete(
                new ResourceResponse
                {
                    Status = ResourceResponseStatus.Ok,
                    ErrorMessage = "",
                    Etag = "invalid\0etag",
                }
            )
        );
        Assert.False(handle.IsClosed);
        handle.Complete(StyleResponse());
        await loaded.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        // The answer consumed the handle, so a second answer fails.
        Assert.True(handle.IsClosed);
        Assert.Throws<MaplibreException>(() => handle.Complete(StyleResponse()));
    }

    // A request outside the routes passes through to native loading, which refuses the scheme.
    // The route without kinds claims both the style and the source that the style names.
    [Fact]
    public async Task RoutesDecideWhichRequestsReachTheProvider()
    {
        const string routedStyle = "routed-test://maps/style.json";
        const string routedSource = "routed-test://maps/source.json";
        var style =
            """{"version":8,"sources":{"s":{"type":"vector","url":"routed-test://maps/source.json"}},"layers":[]}"""u8.ToArray();
        var seen = new ConcurrentQueue<(ResourceKind, string)>();
        var sourceRequested = new TaskCompletionSource(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        ImmutableArray<ResourceRoute> routes =
        [
            // Another kind: claims no style.
            new ResourceRoute(ResourceKindFlag.Tile, UrlMatchFlag.Glob, "unrouted-test://**"),
            new ResourceRoute { Flags = UrlMatchFlag.Glob, Url = "routed-test://**" },
        ];
        await using var fixture = await NativeFixture.CreateAsync(
            provider: new ResourceProvider
            {
                Callback = (request, handle) =>
                {
                    seen.Enqueue((request.Kind, request.RequestedUrl));
                    handle.Complete(
                        request.Kind == ResourceKind.Style
                            ? new ResourceResponse
                            {
                                Status = ResourceResponseStatus.Ok,
                                Bytes = [.. style],
                                ErrorMessage = "",
                                Etag = "",
                            }
                            : new ResourceResponse
                            {
                                Status = ResourceResponseStatus.Error,
                                ErrorReason = ResourceErrorReason.NotFound,
                                ErrorMessage = "",
                                Etag = "",
                            }
                    );
                    if (request.Kind == ResourceKind.Source)
                        sourceRequested.TrySetResult();
                },
                Routes = routes,
            }
        );

        var failure = await Assert.ThrowsAsync<MaplibreException>(() =>
            fixture
                .Map.SetStyleUrlAsync("unrouted-test://maps/style.json")
                .WaitAsync(TestWaits.Deadline, TestWaits.Token)
        );
        Assert.Equal(Status.NativeError, failure.Status);
        Assert.Empty(seen);

        await fixture.Map.SetStyleUrlAsync(routedStyle);
        await sourceRequested.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        Assert.Equal(
            [(ResourceKind.Style, routedStyle), (ResourceKind.Source, routedSource)],
            seen.Distinct()
        );
    }

    // A provider's exception is contained: the binding releases the unanswered handle, which fails
    // the request, and the exception goes to the CallbackException handlers.
    [Fact]
    public unsafe void AProviderExceptionIsContainedAndFailsTheRequest()
    {
        ResourceRequestInfo? copied = null;
        ResourceRequestHandle? escaped = null;
        var thrown = new FormatException("Host callback failed.");
        var reports = new List<CallbackExceptionEventArgs>();
        Exception? reentry = null;
        // Other tests' callbacks may throw concurrently, so only this exception counts.
        EventHandler<CallbackExceptionEventArgs> record = (_, report) =>
        {
            if (!ReferenceEquals(report.Exception, thrown))
                return;
            lock (reports)
                reports.Add(report);
            // This callback permits status-returning native calls.
            reentry = Record.Exception(() => MaplibreNative.GetNetworkStatus());
        };
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeResourceProvider(
            new ResourceProvider
            {
                Callback = (request, handle) =>
                {
                    copied = request;
                    escaped = handle;
                    throw thrown;
                },
            },
            scope
        );
        var bytes = scope.Buffer([1, 2, 3]);
        var request = new mln_resource_request_info
        {
            size = (uint)sizeof(mln_resource_request_info),
            fields = mln_resource_request_field.MLN_RESOURCE_REQUEST_RANGE,
            requested_url = scope.Utf8(StyleUrl),
            resolved_url = scope.Utf8("provider-test://resolved/é"),
            range = new mln_resource_range { start = 0, end = 7 },
            prior_data = (byte*)bytes.data,
            prior_data_size = bytes.size,
        };

        MaplibreNative.CallbackException += record;
        try
        {
            native.callback(native.user_data, &request, SyntheticHandles.ResourceRequest(1));
        }
        finally
        {
            MaplibreNative.CallbackException -= record;
        }

        var report = Assert.Single(reports);
        Assert.Equal("mln_resource_provider_callback", report.Callback);
        Assert.Null(reentry);
        // The request is copied before the callback runs, so native reusing its memory afterwards
        // leaves the copy intact.
        ((byte*)bytes.data)[0] = 99;
        Assert.NotNull(copied);
        Assert.Equal(StyleUrl, copied.RequestedUrl);
        Assert.Equal(new ResourceRange(0, 7), copied.Range);
        Assert.Equal([1, 2, 3], copied.PriorData);
        Assert.NotNull(escaped);
        Assert.True(escaped.IsClosed);
    }

    [Fact]
    public async Task PassingThroughDisarmsAnEscapedRequest()
    {
        ResourceRequestHandle? escaped = null;
        var calls = 0;
        await using var fixture = await NativeFixture.CreateAsync(
            provider: new ResourceProvider
            {
                Callback = (_, handle) =>
                {
                    escaped = handle;
                    handle.SetCancelCallback(
                        new ResourceRequestCancelHandler(() => Interlocked.Increment(ref calls))
                    );
                    handle.PassThrough();
                },
            }
        );

        var failure = await Assert.ThrowsAsync<MaplibreException>(() =>
            fixture.Map.SetStyleUrlAsync(StyleUrl).WaitAsync(TestWaits.Deadline, TestWaits.Token)
        );
        Assert.Equal(Status.NativeError, failure.Status);

        Assert.NotNull(escaped);
        Assert.Equal(0, calls);
        Assert.True(escaped.IsClosed);
        Assert.Throws<MaplibreException>(() => escaped.IsCancelled());
    }

    /// <summary>A provider that keeps each request to answer later.</summary>
    private static ResourceProvider Holding(TaskCompletionSource<ResourceRequestHandle> received) =>
        new() { Callback = (_, handle) => received.TrySetResult(handle) };
}
