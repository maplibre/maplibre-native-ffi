// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One atomic relative camera update.
/// </summary>
/// <remarks>
/// See <c>mln_camera_delta</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record CameraDelta
{
    /// <summary>
    /// Pan in logical map pixels; the content moves by this offset.
    /// </summary>
    public ScreenPoint? Offset { get; init; }

    /// <summary>
    /// Positive zoom factor; 2 zooms in one level.
    /// </summary>
    public double? Scale { get; init; }

    /// <summary>
    /// Degrees added to the bearing.
    /// </summary>
    public double? Bearing { get; init; }

    /// <summary>
    /// Degrees added to the pitch; positive tilts further from straight down.
    /// </summary>
    public double? Pitch { get; init; }

    /// <summary>
    /// Screen point in logical map pixels that scale, bearing, and pitch keep
    /// fixed.
    /// </summary>
    public ScreenPoint? Anchor { get; init; }
    public required AnimationOptions Animation { get; init; }
    public GesturePhase GesturePhase { get; init; }
    public static CameraDelta Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyCameraDelta(NativeMethods.mln_camera_delta_default());
        }
    }
}
