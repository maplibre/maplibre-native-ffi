// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public sealed record MapViewportOptions
{
    public NorthOrientation? NorthOrientation { get; set; }
    public ConstrainMode? ConstrainMode { get; set; }
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
