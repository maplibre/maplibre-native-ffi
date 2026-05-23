using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native.Internal.Handle;

internal unsafe delegate mln_status StatusDestroy<T>(T* handle)
    where T : unmanaged;

internal sealed unsafe class NativeHandleState<T>
    where T : unmanaged
{
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
                $"{typeName} pointer is null.");
        }

        this.destroy = destroy;
        this.typeName = typeName;
        address = (nint)handle;
    }

    internal bool IsClosed => address == 0;

    internal T* Pointer
    {
        get
        {
            var handle = (T*)address;
            if (handle is null)
            {
                throw new InvalidArgumentException(
                    MaplibreStatus.InvalidArgument,
                    null,
                    $"{typeName} is closed.");
            }

            return handle;
        }
    }

    internal void Close()
    {
        var handle = (T*)address;
        if (handle is null)
        {
            return;
        }

        NativeStatus.Check(destroy(handle));
        address = 0;
    }

    internal bool TryClose()
    {
        var handle = (T*)address;
        if (handle is null)
        {
            return true;
        }

        if (destroy(handle) != mln_status.MLN_STATUS_OK)
        {
            return false;
        }

        address = 0;
        return true;
    }
}
