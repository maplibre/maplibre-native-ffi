// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for <c>RuntimeHandle.Create()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="AssetPath">
/// The directory that <c>asset://</c> URLs load from. Empty uses the working
/// directory, or the app's assets on Android.
/// </param>
/// <param name="CachePath">
/// The cache database file, which holds the ambient cache and offline regions.
/// Empty keeps them in memory.
/// </param>
public readonly partial record struct RuntimeOptions(string AssetPath, string CachePath)
{
    public RuntimeOptions()
        : this("", "") { }
}
