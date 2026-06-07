using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;

namespace Maplibre.Native.Resource;

/// <summary>Resource provider request handle.</summary>
public sealed unsafe class ResourceRequestHandle : IDisposable
{
    private readonly object gate = new();
    private mln_resource_request_handle* handle;
    private bool providerDecisionFinalized;
    private bool releaseAccountedFor;
    private bool closed;
    private bool completed;

    internal ResourceRequestHandle(mln_resource_request_handle* handle)
    {
        if (handle is null)
        {
            throw new ArgumentNullException(nameof(handle));
        }

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
                    "ResourceRequestHandle is already completed."
                );
            }

            ThrowIfClosed();
            using var nativeResponse = NativeResourceResponse.From(response);
            var value = nativeResponse.Value;
            NativeStatus.Check(NativeMethods.mln_resource_request_complete(handle, &value));
            completed = true;
            closed = true;
            if (providerDecisionFinalized)
            {
                ReleaseIfOwnedLocked();
            }
        }
    }

    /// <summary>Whether the native request has been cancelled.</summary>
    public bool IsCancelled()
    {
        lock (gate)
        {
            ThrowIfClosed();
            bool cancelled = false;
            NativeStatus.Check(NativeMethods.mln_resource_request_cancelled(handle, &cancelled));
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
            NativeMethods.mln_resource_request_release(current);
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
                "ResourceRequestHandle is already completed or released."
            );
        }
    }
}
