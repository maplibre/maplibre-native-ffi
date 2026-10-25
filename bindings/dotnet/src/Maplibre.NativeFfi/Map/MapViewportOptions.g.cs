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
    public NorthOrientation? NorthOrientation { get; init; }
    public ConstrainMode? ConstrainMode { get; init; }
    public ViewportMode? ViewportMode { get; init; }
    public EdgeInsets? FrustumOffset { get; init; }
    public static MapViewportOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyMapViewportOptions(
                NativeMethods.mln_map_viewport_options_default()
            );
        }
    }
}
