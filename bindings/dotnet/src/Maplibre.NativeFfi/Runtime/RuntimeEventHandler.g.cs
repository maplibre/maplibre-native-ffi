// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Runtime and map event callback. Map events arrive on the runtime's native
/// scheduler thread, and offline events may arrive on a MapLibre worker thread.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RuntimeEventHandler
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public RuntimeEventHandler(RuntimeEventCallback Callback)
    {
        this.Callback = Callback;
    }

    /// <summary>
    /// Receives each event batch.
    /// </summary>
    public required RuntimeEventCallback Callback { get; init; }
}
