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
public sealed unsafe partial class GeojsonSourceDataHandle
    : IDisposable,
        INativeOwner<MlnGeojsonSourceData>
{
    private readonly NativeHandleState<MlnGeojsonSourceData> state;

    internal GeojsonSourceDataHandle(MlnGeojsonSourceData handle)
    {
        state = new(handle, Abandon, nameof(GeojsonSourceDataHandle), Abandon);
    }

    internal static GeojsonSourceDataHandle Adopt(MlnGeojsonSourceData handle) =>
        NativeHandleState<MlnGeojsonSourceData>.Adopt(
            handle,
            () => new GeojsonSourceDataHandle(handle),
            Abandon
        );

    private static mln_status Abandon(MlnGeojsonSourceData live, mln_diagnostic* diagnostic)
    {
        NativeMethods.mln_geojson_source_data_release(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnGeojsonSourceData> INativeOwner<MlnGeojsonSourceData>.State => state;
    internal MlnGeojsonSourceData Handle => state.Handle;

    // Runtime events report their source by this identity.
    public ulong Id => state.Handle.Value;

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
    public static GeojsonSourceDataHandle Create(byte[] data, GeojsonSourceOptions? options = null)
    {
        using var scope = new NativeCallScope(null);
        var nativeOptions = options is null
            ? default(mln_geojson_source_options)
            : NativeGeojsonSourceOptions(options, scope);
        MlnGeojsonSourceData outData = default;
        Check(
            NativeMethods.mln_geojson_source_data_create(
                scope.Buffer(data),
                options is null ? null : &nativeOptions,
                &outData,
                Diagnostic
            )
        );
        scope.Accept();
        return GeojsonSourceDataHandle.Adopt(outData);
    }
}
