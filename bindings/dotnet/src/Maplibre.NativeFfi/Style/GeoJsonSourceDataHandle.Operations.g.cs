// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Style;

public sealed unsafe partial class GeoJsonSourceDataHandle : IDisposable
{
    private readonly NativeHandleState<MlnGeoJsonSourceData> state;
    private readonly ulong nativeId;
    internal ulong NativeId => nativeId;

    internal GeoJsonSourceDataHandle(MlnGeoJsonSourceData handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnGeoJsonSourceData>(
            handle,
            static live =>
            {
                NativeMethods.mln_geojson_source_data_destroy(live);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(GeoJsonSourceDataHandle),
            static live =>
            {
                NativeMethods.mln_geojson_source_data_destroy(live);
                return mln_status.MLN_STATUS_OK;
            }
        );
    }

    internal static GeoJsonSourceDataHandle Adopt(MlnGeoJsonSourceData handle)
    {
        GeoJsonSourceDataHandle? owner = null;
        try
        {
            owner = new GeoJsonSourceDataHandle(handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_geojson_source_data_destroy(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnGeoJsonSourceData Handle => state.Handle;

    internal NativeHandleState<MlnGeoJsonSourceData>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_geojson_source_data_destroy"
        );
        state.Retire();
    }

    public static GeoJsonSourceDataHandle Create(byte[] data, GeojsonSourceOptions? options)
    {
        using var scope = new NativeCallScope();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            null,
            "mln_geojson_source_data_create"
        );
        global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
        using var nativeData = NativeStringView.From(data, nameof(data));
        var nativeOptions = options is null
            ? default(mln_geojson_source_options)
            : NativeGeojsonSourceOptions(options, scope);
        MlnGeoJsonSourceData outData = default;
        NativeStatus.Check(
            NativeMethods.mln_geojson_source_data_create(
                nativeData.Value,
                options is null ? null : &nativeOptions,
                &outData
            )
        );
        var owner = GeoJsonSourceDataHandle.Adopt(outData);
        scope.Accept();
        return owner;
    }

    public void Close()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_geojson_source_data_destroy"
        );
        state.Close();
    }
}
