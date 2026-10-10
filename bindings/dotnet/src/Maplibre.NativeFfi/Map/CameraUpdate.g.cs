// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One atomic absolute camera update.
/// </summary>
/// <remarks>
/// See <c>mln_camera_update</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct CameraUpdate(
    CameraUpdateMode Mode,
    CameraOptions Camera,
    AnimationOptions Animation,
    GesturePhase GesturePhase
)
{
    public static CameraUpdate Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_camera_update_default");
            return GeneratedValues.CopyCameraUpdate(NativeMethods.mln_camera_update_default());
        }
    }
}
