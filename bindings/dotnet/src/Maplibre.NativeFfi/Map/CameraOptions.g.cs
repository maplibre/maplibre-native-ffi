// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Camera fields used by snapshots and camera updates.
/// </summary>
/// <remarks>
/// See <c>mln_camera_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record CameraOptions
{
    public LatLng? Center { get; set; }
    public double? CenterAltitude { get; set; }
    public EdgeInsets? Padding { get; set; }

    /// <summary>
    /// Optional screen-space focal point in logical map pixels.
    /// </summary>
    public ScreenPoint? Anchor { get; set; }
    public double? Zoom { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
    public double? Roll { get; set; }
    public double? FieldOfView { get; set; }
    public static CameraOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_camera_options_default");
            return GeneratedValues.CopyCameraOptions(NativeMethods.mln_camera_options_default());
        }
    }
}
