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
/// Directory root for asset:// URLs. Copied during runtime creation. Empty
/// selects <c>/android_asset</c> on Android and <c>.</c> elsewhere.
/// </param>
/// <param name="CachePath">
/// Cache database path. Copied during runtime creation. Empty selects an
/// in-memory cache.
/// </param>
public readonly partial record struct RuntimeOptions(string AssetPath, string CachePath)
{
    public RuntimeOptions()
        : this(default!, default!)
    {
        this = Default;
    }

    public static RuntimeOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyRuntimeOptions(NativeMethods.mln_runtime_options_default());
        }
    }
}
