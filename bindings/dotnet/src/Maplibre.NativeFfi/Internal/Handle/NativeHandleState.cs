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
    private readonly bool protectsCopies;
    private bool closed;
    private int readers;
    private bool releasing;

    internal NativeHandleState(
        T handle,
        StatusDestroy<T> destroy,
        string typeName,
        StatusDestroy<T>? disposeAbandoned = null,
        object? retainedParent = null,
        bool protectsCopies = false
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
        this.protectsCopies = protectsCopies;
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

    // Only a batch getter copies pointers after its native lease has ended.
    internal ReadScope Read(object owner)
    {
        if (protectsCopies)
        {
            lock (gate)
            {
                if (releasing)
                    throw InUse();
                readers++;
            }
        }
        return new(this, owner);
    }

    internal readonly ref struct ReadScope(NativeHandleState<T> state, object owner)
    {
        internal T Handle => state.handle;

        public void Dispose()
        {
            if (state.protectsCopies)
            {
                lock (state.gate)
                    state.readers--;
            }
            GC.KeepAlive(owner);
        }
    }

    internal void HandOff()
    {
        lock (gate)
            closed = true;
        GC.SuppressFinalize(this);
    }

    internal bool Close(bool idempotent = true) => Release(destroy, idempotent);

    internal void Retire() => Release(disposeAbandoned ?? destroy);

    private static MaplibreException InUse() =>
        new(global::Maplibre.NativeFfi.Status.InvalidState, "Batch is in use");

    // Returns whether this call ended the handle, rather than finding it closed.
    internal bool Release(StatusDestroy<T> release, bool idempotent = true)
    {
        lock (gate)
        {
            if (protectsCopies && (readers != 0 || releasing))
                throw InUse();
            if (idempotent && closed)
                return false;
            releasing = protectsCopies;
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
            lock (gate)
                releasing = false;
            GC.KeepAlive(retainedParent);
        }
    }
}
