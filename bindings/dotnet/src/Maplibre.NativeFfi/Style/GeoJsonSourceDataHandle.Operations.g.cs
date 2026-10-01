// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Style;

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
        NativeMethods.mln_geojson_source_data_destroy(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnGeoJsonSourceData> INativeOwner<MlnGeoJsonSourceData>.State => state;
    internal MlnGeoJsonSourceData Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_geojson_source_data_destroy");
        state.Retire();
    }

    public static GeoJsonSourceDataHandle Create(byte[] data, GeojsonSourceOptions? options)
    {
        using var scope = new NativeCallScope(null, "mln_geojson_source_data_create");
        var nativeOptions = options is null
            ? default(mln_geojson_source_options)
            : NativeGeojsonSourceOptions(options, scope);
        MlnGeoJsonSourceData outData = default;
        Check(
            NativeMethods.mln_geojson_source_data_create(
                scope.Buffer(data),
                options is null ? null : &nativeOptions,
                &outData,
                Diagnostic
            )
        );
        scope.Accept();
        return GeoJsonSourceDataHandle.Adopt(outData);
    }

    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_geojson_source_data_destroy");
        state.Close();
    }
}
