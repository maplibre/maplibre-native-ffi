// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Optional map camera constraint fields.
/// </summary>
/// <remarks>
/// See <c>mln_bound_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record BoundOptions
{
    /// <summary>
    /// Read when fields contains <c>MLN_BOUND_OPTION_BOUNDS</c>.
    /// </summary>
    public LatLngBounds? Bounds { get; set; }
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public double? MinPitch { get; set; }
    public double? MaxPitch { get; set; }

    /// <summary>
    /// Selects the unbounded geographic constraint, which leaves every camera
    /// center unconstrained and lets the map pan freely across the
    /// antimeridian. This differs from world bounds of -90/-180 to 90/180,
    /// which clamp longitude to that range. Mutually exclusive with
    /// <c>MLN_BOUND_OPTION_BOUNDS</c>, and leaves
    /// <c>mln_bound_options.bounds</c> unread.
    /// </summary>
    public bool Unbounded { get; set; }
    public static BoundOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_bound_options_default");
            return GeneratedValues.CopyBoundOptions(NativeMethods.mln_bound_options_default());
        }
    }
}
