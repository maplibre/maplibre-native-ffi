// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// Prepared GeoJSON data that sources retain independently of its handle.
/// </summary>
/// <remarks>
/// See <c>mln_geojson_source_data</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class GeoJsonSourceDataHandle
    : IDisposable,
        INativeOwner<MlnGeoJsonSourceData>
{
    private readonly NativeHandleState<MlnGeoJsonSourceData> state;

    internal GeoJsonSourceDataHandle(MlnGeoJsonSourceData handle)
    {
        state = new(handle, Abandon, nameof(GeoJsonSourceDataHandle), Abandon);
    }

    internal static GeoJsonSourceDataHandle Adopt(MlnGeoJsonSourceData handle) =>
        NativeHandleState<MlnGeoJsonSourceData>.Adopt(
            handle,
            () => new GeoJsonSourceDataHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnGeoJsonSourceData live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_geojson_source_data_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnGeoJsonSourceData> INativeOwner<MlnGeoJsonSourceData>.State => state;
    internal MlnGeoJsonSourceData Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after close.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, close, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Prepares GeoJSON source data for installation on a map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_geojson_source_data_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public static GeoJsonSourceDataHandle Create(string data, GeoJsonSourceOptions? options = null)
    {
        using var scope = new NativeCallScope(null);
        var nativeOptions = options is null
            ? default(mln_geojson_source_options)
            : NativeGeoJsonSourceOptions(options, scope);
        MlnGeoJsonSourceData outData = default;
        Check(
            NativeMethods.mln_geojson_source_data_create(
                scope.Utf8(data),
                options is null ? null : &nativeOptions,
                &outData,
                Diagnostic
            )
        );
        scope.Accept();
        return GeoJsonSourceDataHandle.Adopt(outData);
    }
}
