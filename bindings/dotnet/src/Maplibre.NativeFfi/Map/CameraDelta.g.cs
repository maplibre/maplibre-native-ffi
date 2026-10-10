// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One relative camera operation.
/// </summary>
/// <remarks>
/// See <c>mln_camera_delta</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record CameraDelta
{
    public CameraDeltaKind Kind { get; set; }
    public ScreenPoint Offset { get; set; }
    public double Amount { get; set; }
    public ScreenPoint? Anchor { get; set; }
    public required AnimationOptions Animation { get; set; }
    public static CameraDelta Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_camera_delta_default");
            return GeneratedValues.CopyCameraDelta(NativeMethods.mln_camera_delta_default());
        }
    }
}
