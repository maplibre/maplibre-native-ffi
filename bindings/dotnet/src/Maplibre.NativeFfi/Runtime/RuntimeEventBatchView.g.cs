// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A borrowed view of one owned runtime-event batch.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_batch_view</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct RuntimeEventBatchView
{
    public RuntimeEventBatchView(RuntimeEvent[] Events)
    {
        this.Events = Events;
    }

    public RuntimeEvent[] Events
    {
        get => EventsStorage.ToArray();
        init => EventsStorage = ValueArray.Copy(value);
    }
    internal ValueArray<RuntimeEvent> EventsStorage { get; init; }
}
