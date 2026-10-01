// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
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
            using var call = Enter(null, "mln_runtime_options_default");
            return CopyRuntimeOptions(NativeMethods.mln_runtime_options_default());
        }
    }
}
