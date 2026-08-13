using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Runtime;

internal enum OperationResultKind
{
    None,
    CreateRegion,
    GetRegion,
    ListRegions,
    MergeRegions,
    UpdateRegionMetadata,
    RegionStatus,
}

/// <summary>Common asynchronous operation handle.</summary>
public sealed unsafe class OperationHandle : IDisposable
{
    private readonly object gate = new();
    private readonly MlnOperation handle;
    private readonly RuntimeHandle runtime;
    private readonly OperationResultKind resultKind;
    private int activeUses;
    private bool resultUseActive;
    private bool closing;
    private bool closed;
    private bool resultConsumed;

    internal OperationHandle(
        RuntimeHandle runtime,
        MlnOperation handle,
        OperationResultKind resultKind
    )
    {
        this.runtime = runtime ?? throw new ArgumentNullException(nameof(runtime));
        if (handle.IsNull)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                "Operation handle must not be zero.",
                null
            );
        }

        this.handle = handle;
        this.resultKind = resultKind;
        runtime.RegisterOperation(this);
    }

    internal ulong NativeId => handle.Value;

    /// <summary>Whether this binding has released the native handle.</summary>
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

    /// <summary>Completes when notification readiness reports the terminal operation.</summary>
    public Task WaitAsync(CancellationToken cancellationToken = default)
    {
        var use = AcquireUse();
        return OperationAwaiter.WaitThen(
            runtime.WaitForOperationAsync(use.Handle, cancellationToken),
            static () => { },
            use.Dispose
        );
    }

    /// <summary>Requests cancellation of a pending operation.</summary>
    public void Cancel()
    {
        using var use = AcquireUse();
        NativeStatus.Check(NativeMethods.mln_operation_cancel(use.Handle));
    }

    /// <summary>Copies the terminal status and diagnostic into managed memory.</summary>
    public OperationCompletion GetCompletion()
    {
        using var use = AcquireUse();
        mln_status status = default;
        NativeStatus.Check(NativeMethods.mln_operation_get_status(use.Handle, &status));

        nuint size = 0;
        NativeStatus.Check(NativeMethods.mln_operation_copy_diagnostic(use.Handle, null, 0, &size));
        if (size == 0)
        {
            return new OperationCompletion(
                NativeStatus.StatusFromRaw((int)status),
                (int)status,
                string.Empty
            );
        }
        if (size > int.MaxValue)
        {
            throw new OverflowException("The operation diagnostic is too large.");
        }

        var bytes = new byte[(int)size];
        fixed (byte* pointer = bytes)
        {
            nuint copiedSize = 0;
            NativeStatus.Check(
                NativeMethods.mln_operation_copy_diagnostic(
                    use.Handle,
                    (sbyte*)pointer,
                    (nuint)bytes.Length,
                    &copiedSize
                )
            );
            if (copiedSize != size)
            {
                throw new InvalidOperationException(
                    "The operation diagnostic size changed while it was copied."
                );
            }
        }

        return new OperationCompletion(
            NativeStatus.StatusFromRaw((int)status),
            (int)status,
            System.Text.Encoding.UTF8.GetString(bytes)
        );
    }

    /// <summary>Discards the completed operation's untaken result.</summary>
    public void DiscardResult()
    {
        using var use = AcquireResultUse(runtime, resultKind);
        NativeStatus.Check(NativeMethods.mln_operation_discard_result(use.Handle));
        MarkResultConsumed(use);
    }

    internal UseLease AcquireResultUse(
        RuntimeHandle expectedRuntime,
        OperationResultKind expectedResultKind
    )
    {
        lock (gate)
        {
            RequireOpenLocked();
            if (!ReferenceEquals(runtime, expectedRuntime))
            {
                throw InvalidState("OperationHandle belongs to a different RuntimeHandle.");
            }
            if (resultKind != expectedResultKind)
            {
                throw InvalidState("OperationHandle has a different result type.");
            }
            if (resultConsumed)
            {
                throw InvalidState("OperationHandle result is already consumed.");
            }
            if (resultUseActive)
            {
                throw InvalidState("OperationHandle result is already being consumed.");
            }
            activeUses++;
            resultUseActive = true;
            return new UseLease(this, handle, true);
        }
    }

    internal void MarkResultConsumed(UseLease use)
    {
        if (!ReferenceEquals(use.Owner, this))
        {
            throw new ArgumentException(
                "The operation use belongs to a different handle.",
                nameof(use)
            );
        }
        lock (gate)
        {
            resultConsumed = true;
        }
    }

    /// <summary>Releases the native observer and any untaken result.</summary>
    public void Close()
    {
        lock (gate)
        {
            if (closed)
            {
                return;
            }
            if (closing)
            {
                while (!closed)
                {
                    Monitor.Wait(gate);
                }
                return;
            }
            closing = true;
            while (activeUses != 0)
            {
                Monitor.Wait(gate);
            }
        }

        NativeMethods.mln_operation_release(handle);
        runtime.UnregisterOperation(this);
        lock (gate)
        {
            closed = true;
            Monitor.PulseAll(gate);
        }
    }

    /// <inheritdoc />
    public void Dispose() => Close();

    private UseLease AcquireUse()
    {
        lock (gate)
        {
            RequireOpenLocked();
            activeUses++;
            return new UseLease(this, handle, false);
        }
    }

    private void RequireOpenLocked()
    {
        if (closed || closing)
        {
            throw InvalidState("OperationHandle is already closing or closed.");
        }
    }

    private void ReleaseUse(bool useResult)
    {
        lock (gate)
        {
            if (useResult)
            {
                resultUseActive = false;
            }
            activeUses--;
            if (activeUses == 0)
            {
                Monitor.PulseAll(gate);
            }
        }
    }

    private static InvalidStateException InvalidState(string diagnostic) =>
        new(MaplibreStatus.InvalidState, null, diagnostic, null);

    internal sealed class UseLease : IDisposable
    {
        private readonly bool useResult;
        private OperationHandle? owner;

        internal UseLease(OperationHandle owner, MlnOperation handle, bool useResult)
        {
            this.owner = owner;
            this.useResult = useResult;
            Handle = handle;
        }

        internal OperationHandle? Owner => owner;
        internal MlnOperation Handle { get; }

        public void Dispose()
        {
            var current = Interlocked.Exchange(ref owner, null);
            current?.ReleaseUse(useResult);
        }
    }
}
