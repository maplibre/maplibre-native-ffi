// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Live map viewport and render-transform controls.
/// </summary>
/// <remarks>
/// See <c>mln_map_viewport_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record MapViewportOptions
{
    /// <summary>
    /// One of <c>mln_north_orientation</c>.
    /// </summary>
    public NorthOrientation? NorthOrientation { get; set; }

    /// <summary>
    /// One of <c>mln_constrain_mode</c>.
    /// </summary>
    public ConstrainMode? ConstrainMode { get; set; }

    /// <summary>
    /// One of <c>mln_viewport_mode</c>.
    /// </summary>
    public ViewportMode? ViewportMode { get; set; }
    public EdgeInsets? FrustumOffset { get; set; }
    public static MapViewportOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_map_viewport_options_default");
            return GeneratedValues.CopyMapViewportOptions(
                NativeMethods.mln_map_viewport_options_default()
            );
        }
    }
}
