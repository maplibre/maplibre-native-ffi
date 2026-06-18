using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native.Internal.Handle;

internal unsafe delegate mln_status StatusDestroy<T>(T* handle)
    where T : unmanaged;

internal sealed unsafe class NativeHandleState<T>
    where T : unmanaged
{
    private readonly object gate = new();
    private readonly StatusDestroy<T> destroy;
    private readonly string typeName;
    private nint address;
    private bool releaseInProgress;

    internal NativeHandleState(T* handle, StatusDestroy<T> destroy, string typeName)
    {
        if (handle is null)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"{typeName} pointer is null.",
                null
            );
        }

        this.destroy = destroy;
        this.typeName = typeName;
        address = (nint)handle;
    }

    ~NativeHandleState()
    {
        var current = address;
        if (current != 0)
        {
            NativeLeakReporter.Report(
                new NativeLeakReport(
                    NativeLeakReportKind.LeakedHandle,
                    typeName,
                    current,
                    null,
                    $"Leaked {typeName} native handle 0x{current:x}; call Close() on the owner thread before releasing the wrapper."
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
                return address == 0;
            }
        }
    }

    internal T* Pointer
    {
        get
        {
            lock (gate)
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

                var handle = (T*)address;
                if (handle is null)
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
        }
    }

    internal void Close()
    {
        T* handle;
        lock (gate)
        {
            handle = BeginReleaseLocked();
            if (handle is null)
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
        T* handle;
        nint current;
        lock (gate)
        {
            handle = BeginReleaseLocked();
            if (handle is null)
            {
                return true;
            }

            current = address;
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
            NativeLeakReporter.Report(
                new NativeLeakReport(
                    NativeLeakReportKind.DisposeFailed,
                    typeName,
                    current,
                    status,
                    $"Dispose could not close {typeName} native handle 0x{current:x}; native destroy returned {status}. Call Close() on the owner thread to observe the error and retry."
                )
            );
            return false;
        }

        EndSuccessfulRelease();
        return true;
    }

    private T* BeginReleaseLocked()
    {
        while (releaseInProgress)
        {
            Monitor.Wait(gate);
        }

        var handle = (T*)address;
        if (handle is not null)
        {
            releaseInProgress = true;
        }

        return handle;
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
            address = 0;
            releaseInProgress = false;
            GC.SuppressFinalize(this);
            Monitor.PulseAll(gate);
        }
    }
}
