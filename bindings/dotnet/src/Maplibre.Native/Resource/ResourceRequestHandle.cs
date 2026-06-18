using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;

namespace Maplibre.Native.Resource;

internal unsafe delegate mln_status ResourceRequestComplete(
    mln_resource_request_handle* handle,
    mln_resource_response* response
);

internal unsafe delegate mln_status ResourceRequestCancelled(
    mln_resource_request_handle* handle,
    bool* cancelled
);

internal unsafe delegate void ResourceRequestRelease(mln_resource_request_handle* handle);

/// <summary>Resource provider request handle.</summary>
public sealed unsafe class ResourceRequestHandle : IDisposable
{
    private readonly object gate = new();
    private readonly ResourceRequestComplete complete;
    private readonly ResourceRequestCancelled cancelled;
    private readonly ResourceRequestRelease release;
    private mln_resource_request_handle* handle;
    private bool providerDecisionFinalized;
    private bool releaseAccountedFor;
    private bool closed;
    private bool completed;

    internal ResourceRequestHandle(mln_resource_request_handle* handle)
        : this(
            handle,
            static (request, response) =>
                NativeMethods.mln_resource_request_complete(request, response),
            static (request, cancelled) =>
                NativeMethods.mln_resource_request_cancelled(request, cancelled),
            static request => NativeMethods.mln_resource_request_release(request)
        ) { }

    internal ResourceRequestHandle(
        mln_resource_request_handle* handle,
        ResourceRequestComplete complete,
        ResourceRequestCancelled cancelled,
        ResourceRequestRelease release
    )
    {
        if (handle is null)
        {
            throw new ArgumentNullException(nameof(handle));
        }

        this.complete = complete;
        this.cancelled = cancelled;
        this.release = release;
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
            var status = complete(handle, &value);
            completed = true;
            closed = true;
            if (providerDecisionFinalized)
            {
                ReleaseIfOwnedLocked();
            }
            GC.SuppressFinalize(this);
            NativeStatus.Check(status);
        }
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

    /// <summary>Releases the native request handle without completing it.</summary>
    public void Close()
    {
        lock (gate)
        {
            if (closed)
            {
                return;
            }

            closed = true;
            if (providerDecisionFinalized)
            {
                ReleaseIfOwnedLocked();
            }
            GC.SuppressFinalize(this);
        }
    }

    /// <inheritdoc />
    public void Dispose()
    {
        Close();
        GC.SuppressFinalize(this);
    }

    ~ResourceRequestHandle()
    {
        lock (gate)
        {
            if (providerDecisionFinalized)
            {
                ReleaseIfOwnedLocked();
            }
        }
    }

    internal uint FinishProviderDecision(ResourceProviderDecision decision)
    {
        lock (gate)
        {
            if (completed || decision == ResourceProviderDecision.Handle)
            {
                providerDecisionFinalized = true;
                if (closed)
                {
                    ReleaseIfOwnedLocked();
                }

                return (uint)ResourceProviderDecision.Handle;
            }

            MarkNativeWillReleaseLocked();
            return decision == ResourceProviderDecision.PassThrough
                ? (uint)ResourceProviderDecision.PassThrough
                : uint.MaxValue;
        }
    }

    internal uint FinishProviderException()
    {
        lock (gate)
        {
            if (completed)
            {
                providerDecisionFinalized = true;
                if (closed)
                {
                    ReleaseIfOwnedLocked();
                }

                return (uint)ResourceProviderDecision.Handle;
            }

            MarkNativeWillReleaseLocked();
            return uint.MaxValue;
        }
    }

    private void ReleaseIfOwnedLocked()
    {
        if (releaseAccountedFor)
        {
            return;
        }

        releaseAccountedFor = true;
        var current = handle;
        handle = null;
        if (current is not null)
        {
            release(current);
        }
        closed = true;
    }

    private void MarkNativeWillReleaseLocked()
    {
        providerDecisionFinalized = true;
        releaseAccountedFor = true;
        handle = null;
        closed = true;
    }

    private void ThrowIfClosed()
    {
        if (closed || handle is null)
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
