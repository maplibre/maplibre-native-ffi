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

    // The admission policy forbids runtime calls inside the callback and allows answering the
    // request, which claims the handle even though the callback then passes the request on.
    [Fact]
    public async Task AProviderMayAnswerButNotReenterTheRuntime()
    {
        NativeFixture? owner = null;
        ResourceRequestHandle? retained = null;
        string? requested = null;
        Exception? reentry = null;
        Exception? waitReentry = null;
        await using var fixture = await NativeFixture.CreateAsync(
            provider: new ResourceProvider(
                (request, handle) =>
                {
                    requested = request.RequestedUrl;
                    retained = handle;
                    reentry = Record.Exception(() =>
                    {
                        _ = owner!.Runtime.BarrierAsync(TestWaits.Token);
                    });
                    waitReentry = Record.Exception(handle.WaitUntilRetired);
                    handle.Complete(StyleResponse());
                    return ResourceProviderDecision.PassThrough;
                }
            )
        );
        owner = fixture;

        await fixture.Map.SetStyleUrlAsync(StyleUrl, TestWaits.Token);
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapStyleLoaded);

        Assert.Equal(StyleUrl, requested);
        Assert.IsType<InvalidOperationException>(reentry);
        Assert.IsType<InvalidOperationException>(waitReentry);
        Assert.NotNull(retained);
        using (retained)
            Assert.False(retained.IsClosed);
        Assert.True(retained.IsClosed);
    }

    [Fact]
    public async Task AProviderCanAnswerLaterThroughItsDecisionHandle()
    {
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await using var fixture = await NativeFixture.CreateAsync(provider: Holding(received));
        await fixture.Map.SetStyleUrlAsync(StyleUrl, TestWaits.Token);
        using var handle = await received.Task.WaitAsync(TestWaits.Deadline, TestWaits.Token);

        // A response the binding cannot convert leaves the request open for another answer.
        Assert.Throws<ArgumentException>(() =>
            handle.Complete(
                new ResourceResponse { Status = ResourceResponseStatus.Ok, Etag = "invalid\0etag" }
            )
        );
        Assert.False(handle.IsClosed);
        handle.Complete(StyleResponse());
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapStyleLoaded);

        Assert.False(handle.IsClosed);
        handle.Close();
        handle.WaitUntilRetired();
    }

    // A provider's exception is contained: native receives a pass-through, and the exception goes
    // to the CallbackException handlers.
    [Fact]
    public unsafe void AProviderExceptionIsContainedAndPassesTheRequestThrough()
    {
        ResourceRequest? copied = null;
        ResourceRequestHandle? escaped = null;
        var thrown = new FormatException("Host callback failed.");
        var reports = new List<CallbackExceptionEventArgs>();
        // Other tests' callbacks may throw concurrently, so only this exception counts.
        EventHandler<CallbackExceptionEventArgs> record = (_, report) =>
        {
            if (ReferenceEquals(report.Exception, thrown))
                lock (reports)
                    reports.Add(report);
        };
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeResourceProvider(
            new ResourceProvider(
                (request, handle) =>
                {
                    copied = request;
                    escaped = handle;
                    throw thrown;
                }
            ),
            scope
        );
        var bytes = scope.Buffer([1, 2, 3]);
        var request = new mln_resource_request
        {
            size = (uint)sizeof(mln_resource_request),
            requested_url = scope.CString(StyleUrl),
            resolved_url = scope.CString("provider-test://resolved/é"),
            has_range = 1,
            range_start = 0,
            range_end = 7,
            prior_data = (byte*)bytes.data,
            prior_data_size = bytes.size,
        };

        Maplibre.CallbackException += record;
        try
        {
            Assert.Equal(
                (uint)ResourceProviderDecision.PassThrough,
                native.callback(native.user_data, &request, SyntheticHandles.ResourceRequest(1))
            );
        }
        finally
        {
            Maplibre.CallbackException -= record;
        }

        var report = Assert.Single(reports);
        Assert.Equal("mln_resource_provider_callback", report.Callback);
        // The request is copied before the callback runs, so native reusing its memory afterwards
        // leaves the copy intact.
        ((byte*)bytes.data)[0] = 99;
        Assert.NotNull(copied);
        Assert.Equal(StyleUrl, copied.RequestedUrl);
        Assert.Equal(new ResourceRequest.RangeValue(0, 7), copied.Range);
        Assert.Equal([1, 2, 3], copied.PriorData);
        Assert.NotNull(escaped);
        Assert.True(escaped.IsClosed);
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
        Assert.True(handle.Cancelled());
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
            provider: new ResourceProvider(
                (_, handle) =>
                {
                    escaped = handle;
                    handle.SetCancelCallback(() => Interlocked.Increment(ref calls));
                    return ResourceProviderDecision.PassThrough;
                }
            )
        );

        await fixture.Map.SetStyleUrlAsync(StyleUrl, TestWaits.Token);
        await fixture.WaitForMapEventAsync(RuntimeEventType.MapLoadingFailed);

        Assert.NotNull(escaped);
        Assert.True(escaped.IsClosed);
        Assert.Equal(0, calls);
        Assert.Throws<InvalidStateException>(() => escaped.Cancelled());
    }

    [Fact]
    public unsafe void DecisionStatePreservesAcceptedAndInFlightCompletionButRollsBackRejection()
    {
        var released = 0;
        NativeHandleState<MlnResourceRequest> Create() =>
            new(
                SyntheticHandles.ResourceRequest(1),
                (_, _) =>
                {
                    released++;
                    return mln_status.MLN_STATUS_OK;
                },
                "Request",
                pendingDecision: true
            );
        var rejected = Create();
        using (rejected.BeginClaim()) { }
        Assert.False(rejected.FinishDecision(false));
        Assert.True(rejected.IsClosed);
        Assert.Equal(0, released);
        var accepted = Create();
        using (var claim = accepted.BeginClaim())
            claim.Accept();
        Assert.True(accepted.FinishDecision(false));
        accepted.Close();
        Assert.Equal(1, released);
        var inFlight = Create();
        using (inFlight.BeginClaim())
            Assert.True(inFlight.FinishDecision(false));
        inFlight.Close();
        Assert.Equal(2, released);
        var explicitlyClosed = Create();
        explicitlyClosed.Close();
        explicitlyClosed.Close();
        Assert.True(explicitlyClosed.FinishDecision(false));
        Assert.True(explicitlyClosed.IsClosed);
        Assert.Equal(3, released);
    }

    /// <summary>A provider that keeps each request to answer later.</summary>
    private static ResourceProvider Holding(TaskCompletionSource<ResourceRequestHandle> received) =>
        new(
            (_, handle) =>
            {
                received.TrySetResult(handle);
                return ResourceProviderDecision.Handle;
            }
        );

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (bool Cancelled, WeakReference Captured) RegisterCapturing(
        ResourceRequestHandle handle,
        Action onCancel
    )
    {
        var captured = new object();
        var cancelled = handle.SetCancelCallback(() =>
        {
            GC.KeepAlive(captured);
            onCancel();
        });
        return (cancelled, new WeakReference(captured));
    }
}
