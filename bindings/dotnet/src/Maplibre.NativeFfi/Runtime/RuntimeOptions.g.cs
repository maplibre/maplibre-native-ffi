// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Runtime;

public readonly partial record struct RuntimeOptions(
    uint Flags,
    string? AssetPath,
    string? CachePath,
    RuntimeEventMask EventMask,
    Wake EventWake
)
{
    public static RuntimeOptions Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_runtime_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyRuntimeOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_runtime_options_default()
            );
        }
    }
}
