using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
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
internal sealed unsafe class NativeHandleState<T>
    where T : unmanaged, IMlnHandle
{
    private readonly object gate = new();
    private readonly StatusDestroy<T> destroy;
    private readonly StatusDestroy<T>? disposeAbandoned;
    private readonly string typeName;
    private readonly ulong issued;
    private readonly T handle;
    private readonly object? retainedParent;
    private bool closed;
    private bool releaseInProgress;
    private int readers;
    private bool pendingDecision;
    private bool pendingRelease;
    private bool claimed;
    private int activeClaims;
    private List<NativeOwnerCallbackRoot>? callbacks;
    private NativeCallbackOwner? callbackOwner;
    internal NativeCallbackOwner CallbackOwner
    {
        get
        {
            lock (gate)
            {
                if (callbackOwner is null)
                {
                    callbackOwner = new();
                    if (closed)
                        callbackOwner.Retire();
                }
                return callbackOwner;
            }
        }
    }

    internal NativeHandleState(
        T handle,
        StatusDestroy<T> destroy,
        string typeName,
        StatusDestroy<T>? disposeAbandoned = null,
        bool pendingDecision = false,
        object? retainedParent = null
    )
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

        this.pendingDecision = pendingDecision;
        this.retainedParent = retainedParent;
        this.destroy = destroy;
        this.disposeAbandoned = disposeAbandoned;
        this.typeName = typeName;
        this.handle = handle;
        issued = handle.Value;
    }

    ~NativeHandleState()
    {
        try
        {
            FinalizeOwner();
        }
        finally
        {
            GC.KeepAlive(retainedParent);
        }
    }

    private void FinalizeOwner()
    {
        if (issued == 0 || pendingDecision)
            return;
        if (!closed)
        {
            callbackOwner?.Retire();
            try
            {
                if (
                    disposeAbandoned is not null
                    && disposeAbandoned(handle) == mln_status.MLN_STATUS_OK
                )
                {
                    closed = true;
                    ClearCallbacks();
                    return;
                }
            }
            catch
            {
                // A finalizer reports failed retirement without unwinding.
            }
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

    internal T IssuedHandle => handle;

    internal RetainedScope Retain() => new(this);

    internal readonly ref struct RetainedScope(NativeHandleState<T> owner)
    {
        public void Dispose() => GC.KeepAlive(owner);
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

    internal ReadScope Borrow()
    {
        lock (gate)
        {
            var live = HandleLocked();
            checked
            {
                readers++;
            }
            return new ReadScope(this, live);
        }
    }

    internal ref struct ReadScope
    {
        private NativeHandleState<T>? owner;
        internal T Handle { get; }

        internal ReadScope(NativeHandleState<T> owner, T handle)
        {
            this.owner = owner;
            Handle = handle;
        }

        public void Dispose()
        {
            var retained = owner;
            if (retained is null)
                return;
            owner = null;
            lock (retained.gate)
            {
                retained.readers--;
            }
        }
    }

    internal void Close() => Release(destroy);

    internal void Retire() => Release(disposeAbandoned ?? destroy);

    internal void Release(StatusDestroy<T> release)
    {
        T handle;
        lock (gate)
        {
            if (!BeginReleaseLocked(out handle))
            {
                return;
            }
        }

        lock (gate)
        {
            if (pendingDecision)
            {
                pendingRelease = true;
                closed = true;
                releaseInProgress = false;
                return;
            }
        }
        mln_status status;
        try
        {
            status = release(handle);
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

    internal ClaimScope BeginClaim()
    {
        lock (gate)
        {
            _ = HandleLocked();
            activeClaims++;
            return new ClaimScope(this);
        }
    }

    internal ref struct ClaimScope(NativeHandleState<T> owner)
    {
        private bool accepted;

        internal void Accept() => accepted = true;

        public void Dispose()
        {
            lock (owner.gate)
            {
                owner.claimed |= accepted;
                owner.activeClaims--;
            }
        }
    }

    internal bool FinishDecision(bool accepted)
    {
        bool release;
        lock (gate)
        {
            if (!pendingDecision)
                throw new InvalidOperationException("Decision was already returned.");
            pendingDecision = false;
            accepted |= claimed || activeClaims != 0 || pendingRelease;
            release = accepted && pendingRelease;
            if (!accepted)
            {
                closed = true;
                GC.SuppressFinalize(this);
            }
            else if (release)
                closed = false;
        }
        if (!accepted)
            ClearCallbacks();
        if (release)
            Close();
        return accepted;
    }

    internal Registration PrepareCallback(object callback, object? owner = null)
    {
        var root = new NativeOwnerCallbackRoot(callback, owner ?? this);
        try
        {
            var registration = new Registration(this, root);
            lock (gate)
            {
                _ = HandleLocked();
                (callbacks ??= []).Add(root);
            }
            return registration;
        }
        catch
        {
            root.Dispose();
            throw;
        }
    }

    internal sealed class Registration(NativeHandleState<T> owner, NativeOwnerCallbackRoot root)
        : IDisposable
    {
        private bool accepted;
        internal void* Pointer => root.Pointer;

        internal void Accept()
        {
            accepted = true;
        }

        public void Dispose()
        {
            if (accepted)
                return;
            lock (owner.gate)
            {
                owner.callbacks?.Remove(root);
            }
            root.Dispose();
        }
    }

    private void ClearCallbacks()
    {
        var registrations = callbacks;
        callbacks = null;
        if (registrations is not null)
            foreach (var registration in registrations)
                registration.Dispose();
    }

    private bool BeginReleaseLocked(out T live)
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

        live = handle;
        if (closed)
        {
            return false;
        }

        if (readers != 0)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                $"{typeName} is in use by a native value copy.",
                null
            );
        }

        releaseInProgress = true;

        return true;
    }

    private void EndFailedRelease()
    {
        lock (gate)
        {
            releaseInProgress = false;
        }
    }

    private void EndSuccessfulRelease()
    {
        ClearCallbacks();
        NativeCallbackOwner? owner;
        lock (gate)
        {
            closed = true;
            releaseInProgress = false;
            owner = callbackOwner;
            GC.SuppressFinalize(this);
        }
        owner?.Retire();
    }
}
