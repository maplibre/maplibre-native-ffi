// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// An action on a runtime's ambient cache.
/// </summary>
/// <remarks>
/// <para>The ambient cache holds the resources that maps load, apart from the
/// ones that offline regions hold.</para>
/// <para>See <c>mln_ambient_cache_operation</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public enum AmbientCacheOperation : uint
{
    /// <summary>
    /// Deletes the whole database, offline regions included, and starts an
    /// empty one.
    /// </summary>
    ResetDatabase = 1,

    /// <summary>
    /// Shrinks the database file to free the disk space it no longer uses.
    /// </summary>
    PackDatabase = 2,

    /// <summary>
    /// Makes the library check each cached resource for changes before it uses
    /// the resource again.
    /// </summary>
    Invalidate = 3,
    Clear = 4,
}
