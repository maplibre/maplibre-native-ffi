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
    private bool completedOrReleased;

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
                return completedOrReleased;
            }
        }
    }

    /// <summary>Completes the native request and releases this wrapper's native reference.</summary>
    public void Complete(ResourceResponse response)
    {
        ArgumentNullException.ThrowIfNull(response);
        lock (gate)
        {
            ThrowIfClosed();
            using var nativeResponse = NativeResourceResponse.From(response);
            var value = nativeResponse.Value;
            NativeStatus.Check(NativeMethods.mln_resource_request_complete(handle, &value));
            ReleaseLocked();
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
            if (completedOrReleased)
            {
                return;
            }

            ReleaseLocked();
        }
    }

    /// <inheritdoc />
    public void Dispose()
    {
        Close();
    }

    private void ReleaseLocked()
    {
        NativeMethods.mln_resource_request_release(handle);
        handle = null;
        completedOrReleased = true;
    }

    private void ThrowIfClosed()
    {
        if (completedOrReleased)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                "ResourceRequestHandle is already completed or released.");
        }
    }
}
