using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Pointer;
using Maplibre.Native.Internal.Status;

namespace Maplibre.Native.Runtime;

/// <summary>
/// Releases a runtime owner thread parked in <see cref="RuntimeHandle.Pump" />.
/// </summary>
/// <remarks>
/// A wake source is usable from any thread, which a host's task submission and
/// shutdown paths rely on. It stays usable after its runtime closes, and signalling
/// it then does nothing.
/// </remarks>
public sealed unsafe class WakeSource : IDisposable
{
    // Signal and close are both any-thread, so the gate orders them against each
    // other. Without it a signal that passed the live check could reach native
    // after a concurrent close retired the id, reporting the C API's stale-handle
    // status where every other binding reports success or its own closed error.
    private readonly Lock nativeCallGate = new();
    private readonly NativeHandleState<MlnWakeSource> state;

    internal WakeSource(MlnWakeSource source)
    {
        state = new NativeHandleState<MlnWakeSource>(
            source,
            static handle =>
            {
                NativeMethods.mln_wake_source_destroy(handle);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(WakeSource)
        );
    }

    /// <summary>Whether this wrapper has released its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Sets the runtime's wake flag and releases the parked owner thread.</summary>
    /// <remarks>
    /// A signal raised while the owner thread is running sets the wake flag, so the
    /// next <see cref="RuntimeHandle.Pump" /> returns without parking. Signalling
    /// after the runtime closes succeeds and does nothing.
    /// </remarks>
    public void Signal()
    {
        lock (nativeCallGate)
        {
            NativeStatus.Check(NativeMethods.mln_wake_source_signal(state.Handle));
        }
    }

    /// <summary>Releases the wake source.</summary>
    public void Close()
    {
        lock (nativeCallGate)
        {
            state.Close();
        }
    }

    /// <inheritdoc />
    public void Dispose()
    {
        lock (nativeCallGate)
        {
            state.TryClose();
        }
    }
}
