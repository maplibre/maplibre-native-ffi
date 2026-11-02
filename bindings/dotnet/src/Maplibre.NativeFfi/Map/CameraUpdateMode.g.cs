// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How <c>MapHandle.UpdateCameraAsync()</c> moves the camera.
/// </summary>
/// <remarks>
/// See <c>mln_camera_update_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum CameraUpdateMode : uint
{
    /// <summary>
    /// Moves the camera at once.
    /// </summary>
    Jump = 0,

    /// <summary>
    /// Animates the camera straight to the target.
    /// </summary>
    Ease = 1,

    /// <summary>
    /// Zooms out, travels to the target, and zooms back in.
    /// </summary>
    Fly = 2,
}
