using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Internal.Pointer;

internal unsafe delegate mln_status StatusDestroy<T>(T handle, mln_diagnostic* diagnostic)
    where T : unmanaged, IMlnHandle;

/// <summary>A binding object that owns one native handle.</summary>
internal interface INativeOwner
{
    /// <summary>The roots of callbacks registered through this owner.</summary>
    NativeCallbackOwner CallbackOwner { get; }
}

/// <summary>A binding object that owns one native handle of type <typeparamref name="T"/>.</summary>
internal interface INativeOwner<T> : INativeOwner
    where T : unmanaged, IMlnHandle
{
    NativeHandleState<T> State { get; }

    NativeCallbackOwner INativeOwner.CallbackOwner => State.CallbackOwner;
}

/// <summary>A handle state that a call scope borrows for the duration of one call.</summary>
internal interface INativeReader
{
    void EndRead();
}

/// <summary>
/// Close-once ownership for one native handle.
/// </summary>
/// <remarks>
/// The C API issues generational handles and rejects a released one, so this
/// tracks ownership rather than identity. The null handle means closed.
/// </remarks>
internal sealed unsafe class NativeHandleState<T> : INativeReader
    where T : unmanaged, IMlnHandle
{
    private readonly object gate = new();
    private readonly StatusDestroy<T> destroy;
    private readonly StatusDestroy<T>? disposeAbandoned;
    private readonly string typeName;
    private readonly T handle;
    private readonly object? retainedParent;
    private bool closed;
    private bool releaseInProgress;
    private int readers;
    private bool pendingDecision;
    private bool pendingRelease;
    private bool claimed;
    private int activeClaims;
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
        // A constructor that rejected the null handle leaves nothing to finalize.
        if (handle.Value == 0 || pendingDecision)
            return;
        if (!closed)
        {
            callbackOwner?.Retire();
            try
            {
                if (
                    disposeAbandoned is not null
                    && disposeAbandoned(handle, null) == mln_status.MLN_STATUS_OK
                )
                {
                    closed = true;
                    return;
                }
            }
            catch
            {
                // A finalizer reports failed retirement without unwinding.
            }
            NativeLeakReporter.Report(
                $"Leaked {typeName} native handle 0x{handle.Value:x}; call Close() before releasing the wrapper."
            );
        }
    }

    internal T IssuedHandle => handle;

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
                $"{typeName} is closing",
                null
            );
        }

        if (closed)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                $"{typeName} is closed",
                null
            );
        }

        return handle;
    }

    internal ReadScope Borrow() => new(this, BeginRead());

    /// <summary>Enters <paramref name="operation"/> and borrows the handle for its native call.</summary>
    internal ReadScope Read(object owner, string operation)
    {
        NativeCallbackGuard.EnsureAllowed(owner, operation);
        return Borrow();
    }

    /// <summary>
    /// Constructs the owner of a handle that native code just transferred, and
    /// disposes the handle if construction fails.
    /// </summary>
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

    internal T BeginRead()
    {
        lock (gate)
        {
            var live = HandleLocked();
            checked
            {
                readers++;
            }
            return live;
        }
    }

    public void EndRead()
    {
        lock (gate)
        {
            readers--;
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
            retained.EndRead();
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
            if (pendingDecision)
            {
                pendingRelease = true;
                closed = true;
                releaseInProgress = false;
                return;
            }
        }
        mln_diagnostic diagnostic;
        mln_status status;
        try
        {
            status = release(handle, NativeDiagnostic.Prepare(&diagnostic));
        }
        catch
        {
            EndFailedRelease();
            throw;
        }

        if (status != mln_status.MLN_STATUS_OK)
        {
            EndFailedRelease();
            NativeStatus.Check(status, &diagnostic);
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
        if (release)
            Close();
        return accepted;
    }

    private bool BeginReleaseLocked(out T live)
    {
        if (releaseInProgress)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                $"{typeName} is closing",
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
                $"{typeName} is in use",
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
