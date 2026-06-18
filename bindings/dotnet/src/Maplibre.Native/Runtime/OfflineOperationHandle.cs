using Maplibre.Native.Error;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native.Runtime;

/// <summary>Owner-thread offline database operation token.</summary>
public sealed class OfflineOperationHandle : IDisposable
{
    private readonly object gate = new();
    private readonly RuntimeHandle runtime;
    private bool closed;

    internal OfflineOperationHandle(
        RuntimeHandle runtime,
        ulong id,
        OfflineOperationKind kind,
        OfflineOperationResultKind resultKind
    )
    {
        this.runtime = runtime ?? throw new ArgumentNullException(nameof(runtime));
        if (id == 0)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                "Offline operation id must not be zero.",
                null
            );
        }

        Id = id;
        Kind = kind;
        ResultKind = resultKind;
    }

    /// <summary>Native offline operation identifier.</summary>
    public ulong Id { get; }

    /// <summary>Operation domain.</summary>
    public OfflineOperationKind Kind { get; }

    /// <summary>Expected result shape.</summary>
    public OfflineOperationResultKind ResultKind { get; }

    /// <summary>Whether this operation has been consumed or discarded.</summary>
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

    internal ulong RequireLive(RuntimeHandle expectedRuntime)
    {
        lock (gate)
        {
            if (closed)
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    "OfflineOperationHandle is already closed.",
                    null
                );
            }

            if (!ReferenceEquals(runtime, expectedRuntime))
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    "OfflineOperationHandle belongs to a different RuntimeHandle.",
                    null
                );
            }

            return Id;
        }
    }

    internal ulong RequireLive(
        RuntimeHandle expectedRuntime,
        OfflineOperationKind expectedKind,
        OfflineOperationResultKind expectedResultKind
    )
    {
        lock (gate)
        {
            var id = RequireLive(expectedRuntime);
            if (Kind != expectedKind || ResultKind != expectedResultKind)
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    $"OfflineOperationHandle has kind {Kind} and result kind {ResultKind}, expected {expectedKind} and {expectedResultKind}.",
                    null
                );
            }

            return id;
        }
    }

    internal void MarkConsumed()
    {
        lock (gate)
        {
            closed = true;
        }
    }

    /// <summary>Discards the native offline operation result on the runtime owner thread.</summary>
    public void Close() => runtime.DiscardOfflineOperation(this);

    /// <inheritdoc />
    public void Dispose()
    {
        if (IsClosed)
        {
            return;
        }

        try
        {
            Close();
        }
        catch
        {
            // Dispose is best-effort for owner-thread cleanup; Close reports errors.
        }
    }
}
