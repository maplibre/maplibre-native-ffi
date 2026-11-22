// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What an input or a tick did.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct GestureResult
{
    public GestureResult(
        bool Handled,
        GestureNextTick NextTick,
        double NextTickMs,
        bool BoxZoomActive,
        ScreenBox BoxZoom,
        ImmutableArray<GestureEvent> Events
    )
    {
        this.Handled = Handled;
        this.NextTick = NextTick;
        this.NextTickMs = NextTickMs;
        this.BoxZoomActive = BoxZoomActive;
        this.BoxZoom = BoxZoom;
        this.Events = Events;
    }

    /// <summary>
    /// Whether the recognizer used the input, so the host stops the platform's
    /// default for it. Ticks report false.
    /// </summary>
    public bool Handled { get; init; }
    public GestureNextTick NextTick { get; init; }

    /// <summary>
    /// For <c>GestureNextTick.At</c>.
    /// </summary>
    public double NextTickMs { get; init; }

    /// <summary>
    /// Whether a box-zoom drag is drawing <c>box_zoom</c>, which the host
    /// shows, in logical pixels corner to corner.
    /// </summary>
    public bool BoxZoomActive { get; init; }
    public ScreenBox BoxZoom { get; init; }
    public ImmutableArray<GestureEvent> Events
    {
        get => EventsStorage.AsImmutable();
        init => EventsStorage = ValueArray.From(value);
    }
    internal ValueArray<GestureEvent> EventsStorage { get; init; }
}
