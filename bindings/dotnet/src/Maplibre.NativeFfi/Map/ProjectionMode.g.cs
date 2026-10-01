// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public sealed record ProjectionMode
{
    public bool? Axonometric { get; set; }
    public double? XSkew { get; set; }
    public double? YSkew { get; set; }
    public static ProjectionMode Default
    {
        get
        {
            using var call = Enter(null, "mln_projection_mode_default");
            return CopyProjectionMode(NativeMethods.mln_projection_mode_default());
        }
    }
}
