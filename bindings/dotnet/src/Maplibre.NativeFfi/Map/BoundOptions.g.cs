// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public sealed record BoundOptions
{
    public LatLngBounds? Bounds { get; set; }
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public double? MinPitch { get; set; }
    public double? MaxPitch { get; set; }
    public bool Unbounded { get; set; }
    public static BoundOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_bound_options_default");
            return GeneratedValues.CopyBoundOptions(NativeMethods.mln_bound_options_default());
        }
    }
}
