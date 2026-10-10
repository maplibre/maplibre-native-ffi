// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record ProjectionMode
{
    public bool? Axonometric { get; set; }
    public double? XSkew { get; set; }
    public double? YSkew { get; set; }
    public static ProjectionMode Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_projection_mode_default");
            return GeneratedValues.CopyProjectionMode(NativeMethods.mln_projection_mode_default());
        }
    }
}
