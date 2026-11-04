// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A camera change that fits an area, for
/// <c>MapHandle.FitCameraToLatLngBoundsAsync()</c> and the other fit commands.
/// </summary>
/// <remarks>
/// <para>A jump uses only the end handler of <c>animation</c>, as in
/// <c>CameraUpdate</c>.</para>
/// <para>See <c>mln_camera_fit_update</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct CameraFitUpdate(
    CameraUpdateMode Mode,
    CameraFitOptions Fit,
    AnimationOptions Animation
)
{
    public CameraFitUpdate()
        : this(default, new(), new()) { }
}
