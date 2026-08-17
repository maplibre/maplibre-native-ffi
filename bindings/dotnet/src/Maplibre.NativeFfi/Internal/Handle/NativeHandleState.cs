using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Internal.Pointer;

internal delegate mln_status StatusDestroy<T>(T handle)
    where T : unmanaged, IMlnHandle;

/// <summary>
/// Close-once ownership for one native handle.
/// </summary>
/// <remarks>
/// The C API issues generational handles and rejects a released one, so this
/// tracks ownership rather than identity. The null handle means closed.
/// </remarks>
internal sealed class NativeHandleState<T>
    where T : unmanaged, IMlnHandle
{
    private readonly object gate = new();
    private readonly StatusDestroy<T> destroy;
    private readonly string typeName;
    private readonly ulong issued;
    private T handle;
    private bool closed;
    private bool releaseInProgress;

    internal NativeHandleState(T handle, StatusDestroy<T> destroy, string typeName)
    {
        if (handle.Value == 0)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"{typeName} handle is the null handle.",
                null
            );
        }

        this.destroy = destroy;
        this.typeName = typeName;
        this.handle = handle;
        issued = handle.Value;
    }

    ~NativeHandleState()
    {
        if (!closed)
        {
            var current = issued;
            NativeLeakReporter.Report(
                new NativeLeakReport(
                    NativeLeakReportKind.LeakedHandle,
                    typeName,
                    current,
                    null,
                    $"Leaked {typeName} native handle 0x{current:x}; call Close() before releasing the wrapper."
                )
            );
        }
    }

    internal bool IsClosed
    {
        get
        {
            lock (gate)
            {
                return closed;
            }
        }
    }

    internal T Handle
    {
        get
        {
            lock (gate)
            {
                return HandleLocked();
            }
        }
    }

    private T HandleLocked()
    {
        if (releaseInProgress)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                $"{typeName} is closing.",
                null
            );
        }

        if (closed)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                $"{typeName} is closed.",
                null
            );
        }

        return handle;
    }

    /// <summary>
    /// Runs <paramref name="use" /> after checking that this wrapper still owns the handle.
    /// </summary>
    /// <remarks>
    /// The C API leases the native object for each entry point, so concurrent release does not need
    /// a second binding-side active-use lease.
    /// </remarks>
    internal TResult WithLive<TResult>(Func<T, TResult> use)
    {
        return use(Handle);
    }

    internal void WithLive(Action<T> use)
    {
        WithLive<object?>(handle =>
        {
            use(handle);
            return null;
        });
    }

    internal void Close()
    {
        T handle;
        lock (gate)
        {
            if (!BeginReleaseLocked(out handle))
            {
                return;
            }
        }

        mln_status status;
        try
        {
            status = destroy(handle);
        }
        catch
        {
            EndFailedRelease();
            throw;
        }

        if (status != mln_status.MLN_STATUS_OK)
        {
            EndFailedRelease();
            NativeStatus.Check(status);
        }

        EndSuccessfulRelease();
    }

    internal bool TryClose()
    {
        T handle;
        lock (gate)
        {
            if (!BeginReleaseLocked(out handle))
            {
                return true;
            }
        }

        var current = handle.Value;

        mln_status status;
        try
        {
            status = destroy(handle);
        }
        catch
        {
            EndFailedRelease();
            throw;
        }

        if (status != mln_status.MLN_STATUS_OK)
        {
            EndFailedRelease();
            NativeLeakReporter.Report(
                new NativeLeakReport(
                    NativeLeakReportKind.DisposeFailed,
                    typeName,
                    current,
                    status,
                    $"Dispose could not close {typeName} native handle 0x{current:x}; native destroy returned {status}. Call Close() to observe the error and retry."
                )
            );
            return false;
        }

        EndSuccessfulRelease();
        return true;
    }

    private bool BeginReleaseLocked(out T live)
    {
        while (releaseInProgress)
        {
            Monitor.Wait(gate);
        }

        live = handle;
        if (closed)
        {
            return false;
        }

        releaseInProgress = true;

        return true;
    }

    private void EndFailedRelease()
    {
        lock (gate)
        {
            releaseInProgress = false;
            Monitor.PulseAll(gate);
        }
    }

    private void EndSuccessfulRelease()
    {
        lock (gate)
        {
            closed = true;
            releaseInProgress = false;
            GC.SuppressFinalize(this);
            Monitor.PulseAll(gate);
        }
    }
}
