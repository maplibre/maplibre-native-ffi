// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The state of a render session at one moment.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_snapshot</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// The target's logical size, and the map's scale factor.
/// </param>
/// <param name="FailureStatus">
/// <c>Status.Ok</c> until the session fails. Then <c>Status.TargetLost</c> when
/// it failed because it lost its target, or <c>Status.NativeError</c> when it
/// failed for another reason.
/// </param>
public readonly partial record struct RenderSessionSnapshot(
    RenderSessionState State,
    LogicalExtent Extent,
    ulong Generation,
    ulong FrameGeneration,
    uint AcquiredFrameCount,
    Status FailureStatus
);
