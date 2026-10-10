// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct RuntimeOptions(
    uint Flags,
    string? AssetPath,
    string? CachePath,
    RuntimeEventMask EventMask,
    Wake EventWake
)
{
    public RuntimeOptions()
        : this(default, default, default, RuntimeEventMask.All, default!) { }

    public static RuntimeOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_runtime_options_default");
            return GeneratedValues.CopyRuntimeOptions(NativeMethods.mln_runtime_options_default());
        }
    }
}
