// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public readonly record struct RuntimeEventBatchView
{
    public RuntimeEventBatchView(RuntimeEvent[] Events)
        : this(Events, false) { }

    internal RuntimeEventBatchView(RuntimeEvent[] Events, bool adopt)
    {
        this.storageEvents = adopt ? Events : Events?.ToArray() ?? [];
    }

    private readonly RuntimeEvent[]? storageEvents;
    public RuntimeEvent[] Events
    {
        get => storageEvents?.ToArray() ?? [];
        init => storageEvents = value?.ToArray() ?? [];
    }
    internal RuntimeEvent[] EventsStorage
    {
        get => storageEvents ?? [];
        init => storageEvents = value;
    }

    public bool Equals(RuntimeEventBatchView other) =>
        global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            EventsStorage,
            other.EventsStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(EventsStorage));
        return hash.ToHashCode();
    }
}
