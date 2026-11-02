// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The camera's position and orientation in three dimensions.
/// </summary>
/// <remarks>
/// See <c>mln_free_camera_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record FreeCameraOptions
{
    /// <summary>
    /// The camera's position in Web Mercator units, where the world is 1 wide,
    /// (0, 0) is its northwest corner, and (1, 1) is its southeast corner.
    /// <c>z</c> is the altitude in the same units.
    /// </summary>
    public Vec3? Position { get; init; }

    /// <summary>
    /// The camera's rotation from looking straight down with north at the top
    /// of the screen.
    /// </summary>
    public Quaternion? Orientation { get; init; }
}
