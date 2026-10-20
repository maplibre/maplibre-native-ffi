// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Any-thread render-session snapshot.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_snapshot</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="State">
/// One <c>mln_render_session_state</c> value.
/// </param>
/// <param name="Extent">
/// Logical extent applied by the driver.
/// </param>
/// <param name="Generation">
/// Increases when the session's lifecycle state, target, or applied extent
/// changes. An acquired texture frame reports the value it rendered under.
/// </param>
/// <param name="MapUpdateGeneration">
/// Generation of the latest map render update that the session has seen.
/// </param>
/// <param name="RenderedUpdateGeneration">
/// Map render update generation that the latest rendered frame drew. Zero
/// before the first frame after attachment or a target replacement.
/// </param>
/// <param name="FrameGeneration">
/// Frame generation of the latest rendered frame, zero before the first.
/// </param>
/// <param name="AcquiredFrameCount">
/// Acquired frames that the host has not released or disposed.
/// </param>
/// <param name="PendingChanges">
/// True when a map update or an accepted resize arrived after the latest
/// rendered frame.
/// </param>
public readonly partial record struct RenderSessionSnapshot(
    RenderSessionState State,
    LogicalExtent Extent,
    ulong Generation,
    ulong MapUpdateGeneration,
    ulong RenderedUpdateGeneration,
    ulong FrameGeneration,
    uint AcquiredFrameCount,
    bool PendingChanges
);
