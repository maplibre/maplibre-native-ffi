// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

public sealed record AnimationOptions
{
    public double? DurationMs { get; set; }
    public double? Velocity { get; set; }
    public double? MinZoom { get; set; }
    public UnitBezier? Easing { get; set; }
    public ulong? TransitionId { get; set; }
    public static AnimationOptions Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_animation_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyAnimationOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_animation_options_default()
            );
        }
    }
}
