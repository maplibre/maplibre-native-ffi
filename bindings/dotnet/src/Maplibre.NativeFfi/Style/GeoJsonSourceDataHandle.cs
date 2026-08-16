using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;

namespace Maplibre.NativeFfi.Style;

/// <summary>Prepared GeoJSON source data handle, usable from any thread.</summary>
/// <remarks>
/// <see cref="Create" /> parses and tiles one complete GeoJSON document with its source
/// options baked in, without a runtime or a map. The prepared value is immutable, so one
/// handle may be installed on any number of sources with
/// <see cref="Map.MapHandle.AddGeoJsonSourceData" /> and
/// <see cref="Map.MapHandle.SetGeoJsonSourceData" /> and released at any time afterward;
/// release never invalidates a source the data was installed on.
/// </remarks>
public sealed unsafe class GeoJsonSourceDataHandle : IDisposable
{
    private readonly NativeHandleState<MlnGeoJsonSourceData> state;

    private GeoJsonSourceDataHandle(MlnGeoJsonSourceData handle)
    {
        state = new NativeHandleState<MlnGeoJsonSourceData>(
            handle,
            static handle =>
            {
                NativeMethods.mln_geojson_source_data_destroy(handle);
                return mln_status.MLN_STATUS_OK;
            },
            nameof(GeoJsonSourceDataHandle)
        );
    }

    /// <summary>Parses and tiles one UTF-8 GeoJSON document into prepared source data.</summary>
    /// <remarks>Callable from any thread; no runtime or map is required.</remarks>
    public static GeoJsonSourceDataHandle Create(byte[] data, GeoJsonSourceOptions? options)
    {
        using var nativeData = NativeStringView.From(data, nameof(data));
        using var nativeOptions = options is null ? null : NativeGeoJsonSourceOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        MlnGeoJsonSourceData prepared = default;
        NativeStatus.Check(
            NativeMethods.mln_geojson_source_data_create(
                nativeData.Value,
                nativeOptions is null ? null : &optionsValue,
                &prepared
            )
        );
        return new GeoJsonSourceDataHandle(prepared);
    }

    /// <summary>Whether this wrapper has successfully closed its native handle.</summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>Runs <paramref name="use" /> with release held off until it returns.</summary>
    internal void WithLive(Action<MlnGeoJsonSourceData> use) => state.WithLive(use);

    /// <summary>Destroys the prepared data; sources it was installed on keep their reference.</summary>
    public void Close()
    {
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
    }
}
