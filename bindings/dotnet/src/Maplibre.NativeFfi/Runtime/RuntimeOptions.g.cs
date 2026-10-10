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
/// <param name="AssetPath">
/// Directory root for asset:// URLs. Copied during runtime creation. Null or
/// empty selects <c>/android_asset</c> on Android and <c>.</c> elsewhere.
/// </param>
/// <param name="CachePath">
/// Cache database path. Copied during runtime creation.
/// </param>
/// <param name="EventMask">
/// Runtime-scoped event types this runtime queues, as a bitwise OR of
/// <c>mln_runtime_event_mask</c> values.
/// </param>
/// <param name="EventWake">
/// Wakes the receiver when the runtime event queue becomes nonempty.
/// </param>
public readonly partial record struct RuntimeOptions(
    string? AssetPath,
    string? CachePath,
    RuntimeEventMask EventMask,
    Wake EventWake
)
{
    public RuntimeOptions()
        : this(default, default, RuntimeEventMask.All, default!) { }

    public static RuntimeOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_runtime_options_default");
            return GeneratedValues.CopyRuntimeOptions(NativeMethods.mln_runtime_options_default());
        }
    }
}
