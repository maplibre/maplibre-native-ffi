using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Internal.Pointer;

internal unsafe delegate mln_status StatusDestroy<T>(T handle, mln_diagnostic* diagnostic);

internal interface INativeOwner { }

internal interface INativeOwner<T> : INativeOwner
    where T : unmanaged, IMlnHandle
{
    NativeHandleState<T> State { get; }
}

internal sealed unsafe class NativeHandleState<T>
    where T : unmanaged, IMlnHandle
{
    private readonly object gate = new();
    private readonly StatusDestroy<T> destroy;
    private readonly StatusDestroy<T>? disposeAbandoned;
    private readonly string typeName;
    private readonly T handle;
    private readonly object? retainedParent;
    private bool closed;

    internal NativeHandleState(
        T handle,
        StatusDestroy<T> destroy,
        string typeName,
        StatusDestroy<T>? disposeAbandoned = null,
        object? retainedParent = null
    )
    {
        if (handle.Value == 0)
            throw new MaplibreException(
                global::Maplibre.NativeFfi.Status.InvalidArgument,
                $"{typeName} handle is the null handle."
            );
        this.handle = handle;
        this.destroy = destroy;
        this.typeName = typeName;
        this.disposeAbandoned = disposeAbandoned;
        this.retainedParent = retainedParent;
    }

    ~NativeHandleState()
    {
        try
        {
            if (handle.Value == 0 || closed)
                return;
            string? failure = null;
            try
            {
                if (disposeAbandoned is null)
                    failure = "it has no native disposal";
                else if (
                    disposeAbandoned(handle, null) is var status
                    && status != mln_status.MLN_STATUS_OK
                )
                    failure = $"native disposal failed with {status}";
            }
            catch (Exception error)
            {
                failure = $"native disposal failed: {error.Message}";
            }
            NativeLeakReporter.Report(
                failure is null
                    ? $"Leaked {typeName} native handle 0x{handle.Value:x}; it was disposed when collected. Close or dispose it explicitly."
                    : $"Leaked {typeName} native handle 0x{handle.Value:x}; {failure}, so it was not destroyed. Close or dispose it explicitly."
            );
        }
        finally
        {
            GC.KeepAlive(retainedParent);
        }
    }

    // Wrapper allocation can fail after native has transferred the raw handle.
    internal static TOwner Adopt<TOwner>(T handle, Func<TOwner> create, StatusDestroy<T> dispose)
    {
        try
        {
            return create();
        }
        catch
        {
            dispose(handle, null);
            throw;
        }
    }

    internal T Handle => handle;

    internal bool IsClosed
    {
        get
        {
            lock (gate)
                return closed;
        }
    }

    // The one debug representation that every binding gives its handles.
    public override string ToString() =>
        $"{typeName}(id=0x{handle.Value:x}, closed={(IsClosed ? "true" : "false")})";

    internal bool Close(bool idempotent = true) => Release(destroy, idempotent);

    internal void Retire() => Release(disposeAbandoned ?? destroy);

    // Returns whether this call ended the handle, rather than finding it closed.
    internal bool Release(StatusDestroy<T> release, bool idempotent = true)
    {
        lock (gate)
        {
            if (idempotent && closed)
                return false;
        }
        try
        {
            mln_diagnostic diagnostic;
            NativeStatus.Check(release(handle, NativeDiagnostic.Prepare(&diagnostic)), &diagnostic);
            lock (gate)
                closed = true;
            GC.SuppressFinalize(this);
            return true;
        }
        finally
        {
            GC.KeepAlive(retainedParent);
        }
    }
}
