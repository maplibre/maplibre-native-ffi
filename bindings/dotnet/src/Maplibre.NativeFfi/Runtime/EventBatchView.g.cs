// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A borrowed span of runtime events in one delivery.
/// </summary>
/// <remarks>
/// See <c>mln_event_batch_view</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct EventBatchView
{
    public EventBatchView(ImmutableArray<RuntimeEvent> Events)
    {
        this.Events = Events;
    }

    /// <summary>
    /// Borrowed array of event_count events in queue order.
    /// </summary>
    public ImmutableArray<RuntimeEvent> Events
    {
        get => EventsStorage.AsImmutable();
        init => EventsStorage = ValueArray.From(value);
    }
    internal ValueArray<RuntimeEvent> EventsStorage { get; init; }
}
