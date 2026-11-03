// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A maintenance action on a runtime's cache database.
/// </summary>
/// <remarks>
/// <para>The cache database is the file that <c>RuntimeOptions.CachePath</c>
/// names. It holds the ambient cache, meaning the resources that maps loaded,
/// and the offline regions.</para>
/// <para>See <c>mln_cache_database_operation</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public enum CacheDatabaseOperation : uint
{
    /// <summary>
    /// Deletes the ambient cache's resources and frees the space they used.
    /// Offline regions keep their resources.
    /// </summary>
    Clear = 1,

    /// <summary>
    /// Keeps the ambient cache's resources, but makes maps check each one with
    /// the server before they use it again. Offline regions keep their
    /// resources as they are.
    /// </summary>
    Invalidate = 2,

    /// <summary>
    /// Compacts the whole database file to free the disk space that it no
    /// longer uses.
    /// </summary>
    Pack = 3,

    /// <summary>
    /// Deletes the whole database, offline regions included, and starts an
    /// empty one.
    /// </summary>
    Reset = 4,
}
