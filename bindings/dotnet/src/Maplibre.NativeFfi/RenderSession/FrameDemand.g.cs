// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One nonblocking request for a frame.
/// </summary>
/// <remarks>
/// See <c>mln_frame_demand</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Flags">
/// A bitwise OR of <c>mln_frame_demand_flag</c> values. Defaults to
/// <c>MLN_FRAME_DEMAND_IF_NEEDED</c>.
/// </param>
/// <param name="Token">
/// Host identity returned with the terminal frame result.
/// </param>
/// <param name="CoalescingBoundary">
/// Demands coalesce only when this value and their flags match.
/// </param>
/// <param name="TimeoutNs">
/// Positive time allowed before driver work begins, in nanoseconds; zero has no
/// limit.
/// </param>
public readonly partial record struct FrameDemand(
    FrameDemandFlag Flags,
    ulong Token,
    ulong CoalescingBoundary,
    ulong TimeoutNs
)
{
    public FrameDemand()
        : this(FrameDemandFlag.IfNeeded, default, default, default) { }

    public static FrameDemand Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_frame_demand_default");
            return GeneratedValues.CopyFrameDemand(NativeMethods.mln_frame_demand_default());
        }
    }
}
