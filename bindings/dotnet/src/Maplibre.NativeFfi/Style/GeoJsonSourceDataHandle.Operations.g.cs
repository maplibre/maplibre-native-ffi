// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

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
        NativeMethods.mln_geojson_source_data_destroy(live);
        return mln_status.MLN_STATUS_OK;
    }

    NativeHandleState<MlnGeojsonSourceData> INativeOwner<MlnGeojsonSourceData>.State => state;
    internal MlnGeojsonSourceData Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_geojson_source_data_destroy");
        state.Retire();
    }

    public static GeojsonSourceDataHandle Create(byte[] data, GeojsonSourceOptions? options)
    {
        using var scope = new NativeCallScope(null, "mln_geojson_source_data_create");
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

    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_geojson_source_data_destroy");
        state.Close();
    }
}
