// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Receives selected events after their snapshots have been published. Map
/// events arrive on the runtime's native scheduler thread, and offline events
/// may arrive on a MapLibre worker thread.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public delegate void RuntimeEventCallback(RuntimeEvent[] events);
