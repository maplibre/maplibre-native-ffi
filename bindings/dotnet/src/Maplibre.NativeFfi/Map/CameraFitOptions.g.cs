// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record CameraFitOptions
{
    public EdgeInsets? Padding { get; set; }
    public double? Bearing { get; set; }
    public double? Pitch { get; set; }
    public static CameraFitOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_camera_fit_options_default");
            return GeneratedValues.CopyCameraFitOptions(
                NativeMethods.mln_camera_fit_options_default()
            );
        }
    }
}
