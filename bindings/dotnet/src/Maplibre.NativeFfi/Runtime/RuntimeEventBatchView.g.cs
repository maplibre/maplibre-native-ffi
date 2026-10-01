// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

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
