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
    void* userData,
    bool* outCancelled
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
        userData,
        outCancelled
    ) =>
        NativeMethods.mln_resource_request_set_cancel_callback(
            request,
            callback,
            userData,
            outCancelled
        );
    private static readonly ResourceRequestRelease DefaultRelease = static request =>
        NativeMethods.mln_resource_request_release(request);

    private readonly object gate = new();
    private readonly ResourceRequestComplete complete;
    private readonly ResourceRequestCancelled cancelled;
    private readonly ResourceRequestSetCancelCallback setCancelCallback;
    private readonly ResourceRequestRelease release;
    private MlnResourceRequest handle;
    private Action? cancelCallback;
    private nint cancelToken;
    private bool providerDecisionFinalized;
    private bool releaseAccountedFor;
    private bool closed;
    private bool completed;

    internal ResourceRequestHandle(MlnResourceRequest handle)
        : this(handle, null, null, null) { }

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
            DropCancelCallbackLocked();
            if (providerDecisionFinalized)
            {
                pendingRelease = TakeHandleLocked();
            }
            GC.SuppressFinalize(this);
        }

        Release(pendingRelease);
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
    /// Registers a callback that runs once when MapLibre cancels this request.
    /// </summary>
    /// <remarks>
    /// The callback runs on the thread that discards the request: the runtime
    /// owner thread inside a map or runtime call, and a MapLibre thread
    /// otherwise. It runs only for a request the provider has not completed,
    /// and it may complete or close this handle. When the request is already
    /// cancelled, the callback runs on the calling thread before this method
    /// returns, as part of this call: a concurrent <see cref="Close" /> on
    /// another thread does not wait for it. A request accepts one
    /// registration, and a second registration throws
    /// <see cref="InvalidStateException" />.
    /// </remarks>
    public void SetCancelCallback(Action callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        nint token;
        bool alreadyCancelled = false;
        // The native setter never blocks or calls back into the host, so the
        // gate stays held across it: a concurrent Close then waits for this
        // registration the way it waits for any other in-flight use.
        lock (gate)
        {
            ThrowIfClosed();
            if (cancelCallback is not null)
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    "ResourceRequestHandle already has a cancel callback.",
                    null
                );
            }

            token = ResourceRequestCancelRegistry.Register(this);
            cancelCallback = callback;
            cancelToken = token;
            var status = setCancelCallback(
                handle,
                ResourceRequestCancelRegistry.NativeCallback,
                (void*)token,
                &alreadyCancelled
            );
            if (status != mln_status.MLN_STATUS_OK)
            {
                DropCancelCallbackLocked();
                NativeStatus.Check(status);
            }
        }

        if (alreadyCancelled)
        {
            // Native stored nothing, so this call runs the callback in its
            // place. A concurrent Close may already have emptied the slot,
            // which only stops a later dispatch; the registration still runs
            // its callback once, on this thread.
            TakeCancelCallback(token);
            RunCancelCallback(callback);
        }
    }

    /// <summary>Releases the native request handle without completing it.</summary>
    public void Close()
    {
        MlnResourceRequest pendingRelease = default;
        lock (gate)
        {
            if (closed)
            {
                return;
            }

            closed = true;
            DropCancelCallbackLocked();
            if (providerDecisionFinalized)
            {
                pendingRelease = TakeHandleLocked();
            }
            GC.SuppressFinalize(this);
        }

        Release(pendingRelease);
    }

    /// <inheritdoc />
    public void Dispose()
    {
        Close();
        GC.SuppressFinalize(this);
    }

    ~ResourceRequestHandle()
    {
        MlnResourceRequest pendingRelease = default;
        lock (gate)
        {
            DropCancelCallbackLocked();
            if (providerDecisionFinalized)
            {
                pendingRelease = TakeHandleLocked();
            }
        }

        Release(pendingRelease);
    }

    internal uint FinishProviderDecision(ResourceProviderDecision decision)
    {
        MlnResourceRequest pendingRelease = default;
        uint result;
        lock (gate)
        {
            if (completed || decision == ResourceProviderDecision.Handle)
            {
                providerDecisionFinalized = true;
                if (closed)
                {
                    pendingRelease = TakeHandleLocked();
                }

                result = (uint)ResourceProviderDecision.Handle;
            }
            else
            {
                MarkNativeWillReleaseLocked();
                result =
                    decision == ResourceProviderDecision.PassThrough
                        ? (uint)ResourceProviderDecision.PassThrough
                        : uint.MaxValue;
            }
        }

        Release(pendingRelease);
        return result;
    }

    internal uint FinishProviderException()
    {
        MlnResourceRequest pendingRelease = default;
        uint result;
        lock (gate)
        {
            if (completed)
            {
                providerDecisionFinalized = true;
                if (closed)
                {
                    pendingRelease = TakeHandleLocked();
                }

                result = (uint)ResourceProviderDecision.Handle;
            }
            else
            {
                MarkNativeWillReleaseLocked();
                result = uint.MaxValue;
            }
        }

        Release(pendingRelease);
        return result;
    }

    // Runs the host callback for a native cancellation that names this handle's
    // token. The callback leaves its slot under the lock and runs outside it, so
    // it runs at most once and may complete or close this handle.
    internal void DispatchCancel(nint token) => RunCancelCallback(TakeCancelCallback(token));

    internal bool HasCancelCallbackForTest
    {
        get
        {
            lock (gate)
            {
                return cancelCallback is not null;
            }
        }
    }

    private Action? TakeCancelCallback(nint token)
    {
        lock (gate)
        {
            return cancelToken == token ? DropCancelCallbackLocked() : null;
        }
    }

    private Action? DropCancelCallbackLocked()
    {
        var taken = cancelCallback;
        cancelCallback = null;
        ResourceRequestCancelRegistry.Remove(cancelToken);
        cancelToken = 0;
        return taken;
    }

    private static void RunCancelCallback(Action? callback)
    {
        try
        {
            callback?.Invoke();
        }
        catch
        {
            // A host failure must not unwind into the MapLibre cancel path.
        }
    }

    // Hands the native handle to the caller for release outside the lock.
    // Native release waits for a cancel callback running on another thread, and
    // that callback may call back into this handle.
    private MlnResourceRequest TakeHandleLocked()
    {
        if (releaseAccountedFor)
        {
            return default;
        }

        releaseAccountedFor = true;
        var current = handle;
        handle = default;
        closed = true;
        return current;
    }

    private void Release(MlnResourceRequest taken)
    {
        if (!taken.IsNull)
        {
            release(taken);
        }
    }

    private void MarkNativeWillReleaseLocked()
    {
        providerDecisionFinalized = true;
        releaseAccountedFor = true;
        handle = default;
        closed = true;
        DropCancelCallbackLocked();
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
