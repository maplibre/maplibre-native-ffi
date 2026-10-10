// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record FreeCameraOptions
{
    public Vec3? Position { get; set; }
    public Quaternion? Orientation { get; set; }
    public static FreeCameraOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_free_camera_options_default");
            return GeneratedValues.CopyFreeCameraOptions(
                NativeMethods.mln_free_camera_options_default()
            );
        }
    }
}
