using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class ResourceProviderTests
{
    private const string StyleUrl = "provider-test://é/style.json";

    private static ResourceResponse StyleResponse() =>
        new() { Status = ResourceResponseStatus.Ok, Bytes = TestStyles.Empty };

    [BindingSpecTest("", "")]
    [Fact]
    public async Task InlineCompletionClaimsHandleEvenWhenCallbackReturnsPassThrough()
    {
        ResourceRequestHandle? retained = null;
        string? requested = null;
        Exception? reentry = null;
        Exception? waitReentry = null;
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (request, handle) =>
                {
                    requested = request.RequestedUrl;
                    retained = handle;
                    reentry = Record.Exception(() =>
                    {
                        runtime.CloseAsync();
                    });
                    waitReentry = Record.Exception(() => handle.WaitUntilRetired());
                    handle.Complete(StyleResponse());
                    return ResourceProviderDecision.PassThrough;
                }
            )
        );
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapStyleLoaded);
        Assert.Equal(StyleUrl, requested);
        Assert.IsType<InvalidOperationException>(reentry);
        Assert.IsType<InvalidOperationException>(waitReentry);
        Assert.NotNull(retained);
        using (retained)
            Assert.False(retained.IsClosed);
        Assert.True(retained.IsClosed);
    }

    [BindingSpecTest("", "")]
    [Fact]
    public async Task FailedResponseConversionRemainsRetryableAndSuccessfulCompletionKeepsOwner()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (_, handle) =>
                {
                    received.TrySetResult(handle);
                    return ResourceProviderDecision.Handle;
                }
            )
        );
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        using var handle = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        Assert.Throws<ArgumentException>(() =>
            handle.Complete(
                new ResourceResponse { Status = ResourceResponseStatus.Ok, Etag = "invalid\0etag" }
            )
        );
        Assert.False(handle.IsClosed);
        handle.Complete(StyleResponse());
        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapStyleLoaded);
        Assert.False(handle.IsClosed);
        handle.Close();
        handle.WaitUntilRetired();
    }

    [BindingSpecTest("", "")]
    [Fact]
    public async Task CancellationCallbackCanCloseRequestWithoutBlockingNativeRetirement()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (_, handle) =>
                {
                    received.TrySetResult(handle);
                    return ResourceProviderDecision.Handle;
                }
            )
        );
        var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        using var handle = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        var cancelled = new TaskCompletionSource(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        var calls = 0;
        Assert.False(
            handle.SetCancelCallback(() =>
            {
                Interlocked.Increment(ref calls);
                handle.Close();
                cancelled.TrySetResult();
            })
        );
        await map.CloseAsync();
        await cancelled.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        Assert.Equal(1, calls);
        Assert.True(handle.IsClosed);
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task CancelRegistrationIsReleasedOnceItsCallbackReturns()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var received = await HandledRequest(runtime);
        var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        using var handle = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        var cancelled = new TaskCompletionSource(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        var (accepted, captured) = RegisterCapturing(handle, () => cancelled.TrySetResult());
        Assert.False(accepted);
        Assert.True(Alive(captured));
        await map.CloseAsync();
        await cancelled.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        // Native code releases the registration after the callback returns on its
        // own thread, before the provider releases the request.
        await WaitUntilCollected(captured);
        Assert.False(handle.IsClosed);
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task CancelRegistrationIsReleasedWithTheRequestWhenNeverCancelled()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var received = await HandledRequest(runtime);
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        var handle = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        var calls = 0;
        var (accepted, captured) = RegisterCapturing(
            handle,
            () => Interlocked.Increment(ref calls)
        );
        Assert.False(accepted);
        handle.Complete(StyleResponse());
        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapStyleLoaded);
        Assert.True(Alive(captured));
        handle.Close();
        Assert.False(Alive(captured));
        Assert.Equal(0, calls);
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task AlreadyCancelledRegistrationKeepsNothing()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var received = await HandledRequest(runtime);
        var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        using var handle = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        await map.CloseAsync();
        Assert.True(handle.Cancelled());
        var calls = 0;
        var (accepted, captured) = RegisterCapturing(
            handle,
            () => Interlocked.Increment(ref calls)
        );
        Assert.True(accepted);
        // The request stores nothing, so the binding frees the callback before
        // the request is released.
        Assert.False(Alive(captured));
        handle.Close();
        Assert.Equal(0, calls);
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task CancelCallbackCapturingItsRequestDoesNotRootAnAbandonedRequest()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var abandoned = new TaskCompletionSource<(WeakReference Owner, WeakReference Captured)>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (_, handle) =>
                {
                    abandoned.TrySetResult(RegisterCapturingOwner(handle));
                    return ResourceProviderDecision.Handle;
                }
            )
        );
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        var (owner, captured) = await abandoned.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        await WaitUntilCollected(owner);
        await WaitUntilCollected(captured);
    }

    private static async Task<TaskCompletionSource<ResourceRequestHandle>> HandledRequest(
        RuntimeHandle runtime
    )
    {
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (_, handle) =>
                {
                    received.TrySetResult(handle);
                    return ResourceProviderDecision.Handle;
                }
            )
        );
        return received;
    }

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

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (WeakReference Owner, WeakReference Captured) RegisterCapturingOwner(
        ResourceRequestHandle handle
    )
    {
        var captured = new object();
        Assert.False(
            handle.SetCancelCallback(() =>
            {
                GC.KeepAlive(captured);
                GC.KeepAlive(handle);
            })
        );
        return (new WeakReference(handle), new WeakReference(captured));
    }

    private static bool Alive(WeakReference value)
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        return value.IsAlive;
    }

    private static async Task WaitUntilCollected(WeakReference value)
    {
        for (var attempt = 0; attempt < 200 && Alive(value); attempt++)
            await Task.Delay(10, TestContext.Current.CancellationToken);
        Assert.False(value.IsAlive);
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task NativeCompletionRejectionPreservesOwnerAfterCancellation()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        var received = new TaskCompletionSource<ResourceRequestHandle>(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (_, handle) =>
                {
                    received.TrySetResult(handle);
                    return ResourceProviderDecision.Handle;
                }
            )
        );
        var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        using var handle = await received.Task.WaitAsync(
            TimeSpan.FromSeconds(10),
            TestContext.Current.CancellationToken
        );
        await map.CloseAsync();
        Assert.True(handle.Cancelled());
        Assert.Throws<InvalidStateException>(() => handle.Complete(StyleResponse()));
        Assert.False(handle.IsClosed);
    }

    [BindingSpecTest("", "")]
    [Fact]
    public async Task PassThroughAfterCancellationRegistrationDisarmsEscapedWrapper()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        ResourceRequestHandle? escaped = null;
        var calls = 0;
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (_, handle) =>
                {
                    escaped = handle;
                    handle.SetCancelCallback(() => Interlocked.Increment(ref calls));
                    return ResourceProviderDecision.PassThrough;
                }
            )
        );
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        RuntimeEventTestHelpers.WaitForMapEvent(runtime, map, RuntimeEventType.MapLoadingFailed);
        Assert.NotNull(escaped);
        Assert.True(escaped.IsClosed);
        Assert.Equal(0, calls);
        Assert.Throws<InvalidStateException>(() => escaped.Cancelled());
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task ProviderErrorCopiesItsMessageBeforeTemporaryResponseIsReleased()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        await runtime.SetResourceProviderAsync(
            new ResourceProvider(
                (_, handle) =>
                {
                    handle.Complete(
                        new ResourceResponse
                        {
                            Status = ResourceResponseStatus.Error,
                            ErrorReason = ResourceErrorReason.NotFound,
                            ErrorMessage = "style missing é",
                        }
                    );
                    handle.Close();
                    return ResourceProviderDecision.Handle;
                }
            )
        );
        using var map = TestHandles.CreateMap(runtime, MapOptions.Default);
        await map.SetStyleUrlAsync(StyleUrl);
        var failure = RuntimeEventTestHelpers.WaitForMapEvent(
            runtime,
            map,
            RuntimeEventType.MapLoadingFailed
        );
        Assert.Contains("style missing é", failure.Message, StringComparison.Ordinal);
    }

    [BindingSpecTest("", "")]
    [Fact]
    public unsafe void RequestCopiesTransientFieldsAndContainsProviderExceptions()
    {
        ResourceRequest? copied = null;
        ResourceRequestHandle? escaped = null;
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeResourceProvider(
            new ResourceProvider(
                (request, handle) =>
                {
                    copied = request;
                    escaped = handle;
                    throw new FormatException("Host callback failed.");
                }
            ),
            scope
        );
        var bytes = scope.Buffer([1, 2, 3]);
        var request = new mln_resource_request
        {
            size = (uint)sizeof(mln_resource_request),
            requested_url = scope.CString(StyleUrl),
            resolved_url = scope.CString("https://example.test/é"),
            has_range = 1,
            range_start = 0,
            range_end = 7,
            prior_data = (byte*)bytes.data,
            prior_data_size = bytes.size,
        };
        Assert.Equal(
            (uint)ResourceProviderDecision.PassThrough,
            native.callback(native.user_data, &request, SyntheticHandles.ResourceRequest(1))
        );
        ((byte*)bytes.data)[0] = 99;
        Assert.NotNull(copied);
        Assert.Equal(StyleUrl, copied.RequestedUrl);
        Assert.Equal(new ResourceRequest.RangeValue(0, 7), copied.Range);
        Assert.Equal([1, 2, 3], copied.PriorData);
        Assert.NotNull(escaped);
        Assert.True(escaped.IsClosed);
    }

    [BindingSpecTest("")]
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
}
