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
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_camera_delta_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyCameraDelta(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_camera_delta_default()
            );
        }
    }
}
