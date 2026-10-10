// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options used when creating a runtime.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
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
