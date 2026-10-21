using System.Collections.Concurrent;
using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
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
        new() { Status = ResourceResponseStatus.Ok, Bytes = NativeFixture.EmptyStyle };

    [Fact]
    public async Task AProviderCanAnswerLaterThroughItsDecisionHandle()
    {
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await using var fixture = await NativeFixture.CreateAsync(provider: Holding(received));
        await fixture.Map.SetStyleUrlAsync(StyleUrl, TestWaits.Token);
        using var handle = await received.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        // Native rejects a malformed etag while the request stays open for another answer.
        Assert.Throws<MaplibreException>(() =>
            handle.Complete(
                new ResourceResponse { Status = ResourceResponseStatus.Ok, Etag = "invalid\0etag" }
            )
        );
        handle.Complete(StyleResponse());
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapStyleLoaded);

        handle.Close();
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
        var seen = new ConcurrentQueue<(ResourceKind, string?)>();
        var sourceRequested = new TaskCompletionSource(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        ResourceRoute[] routes =
        [
            // Another kind: claims no style.
            new ResourceRoute(ResourceKindMask.Tile, UrlMatchFlags.Glob, "unrouted-test://**"),
            new ResourceRoute { Flags = UrlMatchFlags.Glob, Url = "routed-test://**" },
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
                                Bytes = style,
                            }
                            : new ResourceResponse
                            {
                                Status = ResourceResponseStatus.Error,
                                ErrorReason = ResourceErrorReason.NotFound,
                            }
                    );
                    handle.Close();
                    if (request.Kind == ResourceKind.Source)
                        sourceRequested.TrySetResult();
                    return ResourceProviderDecision.Handle;
                },
                Routes = routes,
            }
        );

        // Changing the host array after registration leaves the native routes intact.
        Array.Fill(routes, new ResourceRoute { Flags = UrlMatchFlags.Glob, Url = "**" });

        await fixture.Map.SetStyleUrlAsync("unrouted-test://maps/style.json", TestWaits.Token);
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapLoadingFailed);
        Assert.Empty(seen);

        await fixture.Map.SetStyleUrlAsync(routedStyle, TestWaits.Token);
        await sourceRequested.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        Assert.Equal(
            [(ResourceKind.Style, routedStyle), (ResourceKind.Source, routedSource)],
            seen.Distinct()
        );
    }

    // A provider's exception is contained: native receives an unknown decision, and the exception goes
    // to the CallbackException handlers.
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
            reentry = Record.Exception(() => Maplibre.NetworkGetStatus());
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
            requested_url = scope.CString(StyleUrl),
            resolved_url = scope.CString("provider-test://resolved/é"),
            range = new mln_resource_range { start = 0, end = 7 },
            prior_data = (byte*)bytes.data,
            prior_data_size = bytes.size,
        };

        Maplibre.CallbackException += record;
        try
        {
            Assert.Equal(
                uint.MaxValue,
                native.callback(native.user_data, &request, SyntheticHandles.ResourceRequest(1))
            );
        }
        finally
        {
            Maplibre.CallbackException -= record;
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
    }

    [Fact]
    public async Task ACancelRegistrationIsReleasedWithItsRequest()
    {
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await using var fixture = await NativeFixture.CreateAsync(provider: Holding(received));
        await fixture.Map.SetStyleUrlAsync(StyleUrl, TestWaits.Token);
        var handle = await received.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);
        var calls = 0;

        var (cancelled, captured) = RegisterCapturing(
            handle,
            () => Interlocked.Increment(ref calls)
        );
        Assert.False(cancelled);
        handle.Complete(StyleResponse());
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapStyleLoaded);
        Assert.True(Gc.IsAlive(captured));

        handle.Close();
        Assert.False(Gc.IsAlive(captured));
        Assert.Equal(0, calls);
    }

    // The registration reports that the request is already cancelled, so native stores nothing
    // and the binding frees the callback at once.
    [Fact]
    public async Task ARegistrationOnACancelledRequestIsNotRooted()
    {
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await using var fixture = await NativeFixture.CreateAsync(provider: Holding(received));
        await fixture.Map.SetStyleUrlAsync(StyleUrl, TestWaits.Token);
        using var handle = await received.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);
        await fixture.Map.CloseAsync();
        Assert.True(handle.IsCancelled());
        var calls = 0;

        var (cancelled, captured) = RegisterCapturing(
            handle,
            () => Interlocked.Increment(ref calls)
        );

        Assert.True(cancelled);
        Assert.False(Gc.IsAlive(captured));
        handle.Close();
        Assert.Equal(0, calls);
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
                    return ResourceProviderDecision.PassThrough;
                },
            }
        );

        await fixture.Map.SetStyleUrlAsync(StyleUrl, TestWaits.Token);
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapLoadingFailed);

        Assert.NotNull(escaped);
        Assert.Equal(0, calls);
        Assert.Throws<MaplibreException>(() => escaped.IsCancelled());
    }

    /// <summary>A provider that keeps each request to answer later.</summary>
    private static ResourceProvider Holding(TaskCompletionSource<ResourceRequestHandle> received) =>
        new()
        {
            Callback = (_, handle) =>
            {
                received.TrySetResult(handle);
                return ResourceProviderDecision.Handle;
            },
        };

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (bool Cancelled, WeakReference Captured) RegisterCapturing(
        ResourceRequestHandle handle,
        Action onCancel
    )
    {
        var captured = new object();
        var cancelled = handle.SetCancelCallback(
            new ResourceRequestCancelHandler(() =>
            {
                GC.KeepAlive(captured);
                onCancel();
            })
        );
        return (cancelled, new WeakReference(captured));
    }
}
