// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_bound_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_bound_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum BoundOptionField : uint
{
    /// <summary>
    /// Selects <c>mln_bound_options.bounds</c> as a geographic constraint that
    /// the camera center stays inside. Mutually exclusive with
    /// <c>MLN_BOUND_OPTION_UNBOUNDED</c>.
    /// </summary>
    Bounds = 1,
    MinZoom = 2,
    MaxZoom = 4,
    MinPitch = 8,
    MaxPitch = 16,

    /// <summary>
    /// Selects the unbounded geographic constraint, which leaves every camera
    /// center unconstrained and lets the map pan freely across the
    /// antimeridian. This differs from world bounds of -90/-180 to 90/180,
    /// which clamp longitude to that range. Mutually exclusive with
    /// <c>MLN_BOUND_OPTION_BOUNDS</c>, and leaves
    /// <c>mln_bound_options.bounds</c> unread.
    /// </summary>
    Unbounded = 32,
}
