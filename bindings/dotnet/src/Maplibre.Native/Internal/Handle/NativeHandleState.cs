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

    internal NativeHandleState(T* handle, StatusDestroy<T> destroy, string typeName)
    {
        if (handle is null)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"{typeName} pointer is null."
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

    internal bool IsClosed => address == 0;

    internal T* Pointer
    {
        get
        {
            var handle = (T*)address;
            if (handle is null)
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    $"{typeName} is closed."
                );
            }

            return handle;
        }
    }

    internal void Close()
    {
        lock (gate)
        {
            var handle = (T*)address;
            if (handle is null)
            {
                return;
            }

            NativeStatus.Check(destroy(handle));
            address = 0;
            GC.SuppressFinalize(this);
        }
    }

    internal bool TryClose()
    {
        lock (gate)
        {
            var handle = (T*)address;
            if (handle is null)
            {
                return true;
            }

            var current = address;
            var status = destroy(handle);
            if (status != mln_status.MLN_STATUS_OK)
            {
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

            address = 0;
            GC.SuppressFinalize(this);
            return true;
        }
    }
}
