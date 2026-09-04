using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;

namespace Maplibre.NativeFfi.Resource;

internal unsafe delegate mln_status ResourceRequestComplete(
    MlnResourceRequest handle,
    mln_resource_response* response
);

internal unsafe delegate mln_status ResourceRequestCancelled(
    MlnResourceRequest handle,
    bool* cancelled
);

internal unsafe delegate mln_status ResourceRequestSetCancelCallback(
    MlnResourceRequest handle,
    delegate* unmanaged[Cdecl]<void*, void> callback,
    void* userData
);

internal unsafe delegate void ResourceRequestRelease(MlnResourceRequest handle);

/// <summary>Resource provider request handle.</summary>
public sealed unsafe class ResourceRequestHandle : IDisposable
{
    private static readonly ResourceRequestComplete DefaultComplete = static (request, response) =>
        NativeMethods.mln_resource_request_complete(request, response);
    private static readonly ResourceRequestCancelled DefaultCancelled = static (
        request,
        cancelled
    ) => NativeMethods.mln_resource_request_cancelled(request, cancelled);
    private static readonly ResourceRequestSetCancelCallback DefaultSetCancelCallback = static (
        request,
        callback,
        userData
    ) => NativeMethods.mln_resource_request_set_cancel_callback(request, callback, userData);
    private static readonly ResourceRequestRelease DefaultRelease = static request =>
        NativeMethods.mln_resource_request_release(request);

    private readonly object gate = new();
    private readonly ResourceRequestComplete complete;
    private readonly ResourceRequestCancelled cancelled;
    private readonly ResourceRequestSetCancelCallback setCancelCallback;
    private readonly ResourceRequestRelease release;
    private MlnResourceRequest handle;
    private ResourceRequestCancelState? cancelState;
    private bool providerDecisionFinalized;
    private bool releaseAccountedFor;
    private bool releaseInProgress;
    private bool closed;
    private bool completed;

    internal ResourceRequestHandle(MlnResourceRequest handle)
        : this(handle, null, null, null, null) { }

    internal ResourceRequestHandle(
        MlnResourceRequest handle,
        ResourceRequestComplete? complete,
        ResourceRequestCancelled? cancelled,
        ResourceRequestRelease? release,
        ResourceRequestSetCancelCallback? setCancelCallback = null
    )
    {
        if (handle.IsNull)
        {
            throw new ArgumentException(
                "Resource request handle is the null handle.",
                nameof(handle)
            );
        }

        this.complete = complete ?? DefaultComplete;
        this.cancelled = cancelled ?? DefaultCancelled;
        this.setCancelCallback = setCancelCallback ?? DefaultSetCancelCallback;
        this.release = release ?? DefaultRelease;
        this.handle = handle;
    }

    /// <summary>Whether the request handle has been completed or released.</summary>
    public bool IsClosed
    {
        get
        {
            lock (gate)
            {
                return closed;
            }
        }
    }

    /// <summary>Completes the native request. Successful completion closes this wrapper.</summary>
    public void Complete(ResourceResponse response)
    {
        ArgumentNullException.ThrowIfNull(response);
        MlnResourceRequest pendingRelease = default;
        mln_status status;
        try
        {
            lock (gate)
            {
                if (completed)
                {
                    throw new InvalidStateException(
                        MaplibreStatus.InvalidState,
                        null,
                        "ResourceRequestHandle is already completed.",
                        null
                    );
                }

                ThrowIfClosed();
                using var nativeResponse = NativeResourceResponse.From(response);
                var value = nativeResponse.Value;
                status = complete(handle, &value);
                completed = true;
                closed = true;
                if (providerDecisionFinalized)
                {
                    pendingRelease = TakeHandleForReleaseLocked();
                }
                GC.SuppressFinalize(this);
            }
        }
        finally
        {
            ReleaseTaken(pendingRelease);
        }

        NativeStatus.Check(status);
    }

    /// <summary>Whether the native request has been cancelled.</summary>
    public bool IsCancelled()
    {
        lock (gate)
        {
            ThrowIfClosed();
            bool cancelled = false;
            NativeStatus.Check(this.cancelled(handle, &cancelled));
            return cancelled;
        }
    }

    /// <summary>
    /// Registers <paramref name="callback" /> to run when MapLibre cancels this
    /// request, or clears the registration when it is <see langword="null" />.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The callback runs at most once, on the thread that cancels the request,
    /// which is the runtime owner thread inside a map or runtime call and a
    /// MapLibre thread otherwise. It runs only while the request is still open:
    /// a request this handle already completed never reports cancellation.
    /// Registering on a request MapLibre already cancelled runs the callback
    /// before this method returns.
    /// </para>
    /// <para>
    /// Each call replaces the previous registration. The callback may complete
    /// or close this request, and it must not call other MapLibre operations.
    /// The binding discards an exception that the callback throws.
    /// </para>
    /// </remarks>
    /// <param name="callback">The callback to run, or <see langword="null" /> to clear.</param>
    /// <exception cref="InvalidStateException">
    /// The handle is already completed or released.
    /// </exception>
    public void SetCancelCallback(Action? callback)
    {
        var replacement = callback is null ? null : new ResourceRequestCancelState(callback);
        ResourceRequestCancelState? previous;
        try
        {
            lock (gate)
            {
                ThrowIfClosed();
                var observed = Volatile.Read(ref cancelState);

                NativeStatus.Check(
                    setCancelCallback(
                        handle,
                        replacement is null ? null : ResourceRequestCancelState.NativeCallback,
                        replacement is null ? null : replacement.UserData
                    )
                );

                var current = Interlocked.CompareExchange(ref cancelState, replacement, observed);
                if (!ReferenceEquals(current, observed))
                {
                    replacement?.Retire();
                    return;
                }

                previous = observed;
                if (closed && replacement is not null)
                {
                    Interlocked.CompareExchange(ref cancelState, null, replacement);
                    replacement.Retire();
                }
            }
        }
        catch
        {
            replacement?.Retire();
            throw;
        }

        previous?.Retire();
    }

    /// <summary>Releases the native request handle without completing it.</summary>
    public void Close()
    {
        MlnResourceRequest pendingRelease = default;
        lock (gate)
        {
            WaitForReleaseLocked();
            if (closed)
            {
                return;
            }

            closed = true;
            if (providerDecisionFinalized)
            {
                pendingRelease = TakeHandleForReleaseLocked();
            }
            GC.SuppressFinalize(this);
        }

        ReleaseTaken(pendingRelease);
    }

    /// <inheritdoc />
    public void Dispose()
    {
        Close();
        GC.SuppressFinalize(this);
    }

    ~ResourceRequestHandle()
    {
        MlnResourceRequest pendingRelease;
        lock (gate)
        {
            pendingRelease = providerDecisionFinalized ? TakeHandleForReleaseLocked() : default;
        }

        ReleaseTaken(pendingRelease);
    }

    internal uint FinishProviderDecision(ResourceProviderDecision decision)
    {
        MlnResourceRequest pendingRelease = default;
        uint result;
        try
        {
            lock (gate)
            {
                if (completed || decision == ResourceProviderDecision.Handle)
                {
                    providerDecisionFinalized = true;
                    if (closed)
                    {
                        pendingRelease = TakeHandleForReleaseLocked();
                    }

                    return (uint)ResourceProviderDecision.Handle;
                }

                MarkNativeWillReleaseLocked();
                result =
                    decision == ResourceProviderDecision.PassThrough
                        ? (uint)ResourceProviderDecision.PassThrough
                        : uint.MaxValue;
            }
        }
        finally
        {
            ReleaseTaken(pendingRelease);
        }

        return result;
    }

    internal uint FinishProviderException()
    {
        MlnResourceRequest pendingRelease = default;
        try
        {
            lock (gate)
            {
                if (completed)
                {
                    providerDecisionFinalized = true;
                    if (closed)
                    {
                        pendingRelease = TakeHandleForReleaseLocked();
                    }

                    return (uint)ResourceProviderDecision.Handle;
                }

                MarkNativeWillReleaseLocked();
            }
        }
        finally
        {
            ReleaseTaken(pendingRelease);
        }

        return uint.MaxValue;
    }

    internal bool HasCancelCallbackForTest => Volatile.Read(ref cancelState) is not null;

    // Waits for a release another thread started, so a second close returns only
    // once the native handle is retired. The cancel callback skips the wait,
    // because native release returns without waiting when the callback calls it.
    private void WaitForReleaseLocked()
    {
        while (releaseInProgress && !ResourceRequestCancelState.IsDispatchingOnCurrentThread)
        {
            Monitor.Wait(gate);
        }
    }

    // Hands the native handle to the caller to release outside the lock, so a
    // release that waits for a cancel callback cannot block that callback from
    // reentering this handle.
    private MlnResourceRequest TakeHandleForReleaseLocked()
    {
        if (releaseAccountedFor)
        {
            return default;
        }

        releaseAccountedFor = true;
        var current = handle;
        handle = default;
        closed = true;
        if (!current.IsNull)
        {
            releaseInProgress = true;
        }

        return current;
    }

    private void ReleaseTaken(MlnResourceRequest taken)
    {
        if (taken.IsNull)
        {
            return;
        }

        try
        {
            release(taken);
        }
        finally
        {
            lock (gate)
            {
                releaseInProgress = false;
                Monitor.PulseAll(gate);
            }

            // Native release waits for a cancel callback running on another
            // thread, so the registration is unreachable once it returns.
            Interlocked.Exchange(ref cancelState, null)?.Retire();
        }
    }

    private void MarkNativeWillReleaseLocked()
    {
        providerDecisionFinalized = true;
        releaseAccountedFor = true;
        handle = default;
        closed = true;
        Interlocked.Exchange(ref cancelState, null)?.Retire();
    }

    private void ThrowIfClosed()
    {
        if (closed || handle.IsNull)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                "ResourceRequestHandle is already completed or released.",
                null
            );
        }
    }
}
