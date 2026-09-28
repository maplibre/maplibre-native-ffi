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

namespace Maplibre.NativeFfi.Map;

public sealed unsafe partial class MapHandle : IDisposable
{
    private readonly NativeHandleState<MlnMap> state;
    private volatile Task teardown = Task.CompletedTask;
    private readonly ulong nativeId;
    internal ulong NativeId => nativeId;

    internal MapHandle(RuntimeHandle parent, MlnMap handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnMap>(
            handle,
            StartRelease,
            nameof(MapHandle),
            static live => NativeMethods.mln_map_dispose(live),
            retainedParent: parent
        );
    }

    internal static MapHandle Adopt(RuntimeHandle parent, MlnMap handle)
    {
        MapHandle? owner = null;
        try
        {
            owner = new MapHandle(parent, handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_map_dispose(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnMap Handle => state.Handle;

    internal NativeHandleState<MlnMap>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_dispose"
        );
        state.Retire();
    }

    public Task<CommandCompletion> AddColorReliefLayerAsync(
        string layerId,
        string sourceId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_color_relief_layer"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_color_relief_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddCustomGeometrySourceAsync(
        string sourceId,
        CustomGeometrySourceOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_custom_geometry_source"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = NativeCustomGeometrySourceOptions(options, scope);
            return NativeMethods.mln_map_add_custom_geometry_source(
                Handle,
                nativeSourceId.Value,
                &nativeOptions,
                completion
            );
        });
        scope.Accept(this.CallbackOwner);
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddCustomMvtVectorSourceAsync(
        string sourceId,
        CustomMvtVectorSourceOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_custom_mvt_vector_source"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = NativeCustomMvtVectorSourceOptions(options, scope);
            return NativeMethods.mln_map_add_custom_mvt_vector_source(
                Handle,
                nativeSourceId.Value,
                &nativeOptions,
                completion
            );
        });
        scope.Accept(this.CallbackOwner);
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddGeojsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_geojson_source_data"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        ArgumentNullException.ThrowIfNull(data);
        using var useData = data.Borrow();
        var handleData = useData.Handle;
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_geojson_source_data(
                    Handle,
                    nativeSourceId.Value,
                    handleData,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddGeojsonSourceUrlAsync(
        string sourceId,
        string url,
        GeojsonSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_geojson_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = options is null
                ? default(mln_geojson_source_options)
                : NativeGeojsonSourceOptions(options, scope);
            return NativeMethods.mln_map_add_geojson_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddHillshadeLayerAsync(
        string layerId,
        string sourceId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_hillshade_layer"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_hillshade_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddImageSourceImageAsync(
        string sourceId,
        LatLng[] coordinates,
        PremultipliedRgba8Image image,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_image_source_image"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeImage = NativePremultipliedRgba8Image(image, scope);
            return NativeMethods.mln_map_add_image_source_image(
                Handle,
                nativeSourceId.Value,
                scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                checked((nuint)coordinates.Length),
                &nativeImage,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddImageSourceUrlAsync(
        string sourceId,
        LatLng[] coordinates,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_image_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        var operation = NativeCompletion.SubmitCommand(completion =>
            NativeMethods.mln_map_add_image_source_url(
                Handle,
                nativeSourceId.Value,
                scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                checked((nuint)coordinates.Length),
                nativeUrl.Value,
                completion
            )
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddLocationIndicatorLayerAsync(
        string layerId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_location_indicator_layer"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_location_indicator_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddRasterDemSourceTilesAsync(
        string sourceId,
        string[] tiles,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_raster_dem_source_tiles"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = options is null
                ? default(mln_style_tile_source_options)
                : NativeStyleTileSourceOptions(options, scope);
            return NativeMethods.mln_map_add_raster_dem_source_tiles(
                Handle,
                nativeSourceId.Value,
                scope.Array<mln_buffer_view, string>(tiles, item => scope.Utf8(item)),
                checked((nuint)tiles.Length),
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddRasterDemSourceUrlAsync(
        string sourceId,
        string url,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_raster_dem_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = options is null
                ? default(mln_style_tile_source_options)
                : NativeStyleTileSourceOptions(options, scope);
            return NativeMethods.mln_map_add_raster_dem_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddRasterSourceTilesAsync(
        string sourceId,
        string[] tiles,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_raster_source_tiles"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = options is null
                ? default(mln_style_tile_source_options)
                : NativeStyleTileSourceOptions(options, scope);
            return NativeMethods.mln_map_add_raster_source_tiles(
                Handle,
                nativeSourceId.Value,
                scope.Array<mln_buffer_view, string>(tiles, item => scope.Utf8(item)),
                checked((nuint)tiles.Length),
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddRasterSourceUrlAsync(
        string sourceId,
        string url,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_raster_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = options is null
                ? default(mln_style_tile_source_options)
                : NativeStyleTileSourceOptions(options, scope);
            return NativeMethods.mln_map_add_raster_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddStyleLayerJsonAsync(
        byte[] layerJson,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_style_layer_json"
        );
        using var nativeLayerJson = NativeStringView.From(layerJson, nameof(layerJson));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_style_layer_json(
                    Handle,
                    nativeLayerJson.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddStyleSourceJsonAsync(
        string sourceId,
        byte[] sourceJson,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_style_source_json"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeSourceJson = NativeStringView.From(sourceJson, nameof(sourceJson));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_add_style_source_json(
                    Handle,
                    nativeSourceId.Value,
                    nativeSourceJson.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddVectorSourceTilesAsync(
        string sourceId,
        string[] tiles,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_vector_source_tiles"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = options is null
                ? default(mln_style_tile_source_options)
                : NativeStyleTileSourceOptions(options, scope);
            return NativeMethods.mln_map_add_vector_source_tiles(
                Handle,
                nativeSourceId.Value,
                scope.Array<mln_buffer_view, string>(tiles, item => scope.Utf8(item)),
                checked((nuint)tiles.Length),
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> AddVectorSourceUrlAsync(
        string sourceId,
        string url,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_add_vector_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeOptions = options is null
                ? default(mln_style_tile_source_options)
                : NativeStyleTileSourceOptions(options, scope);
            return NativeMethods.mln_map_add_vector_source_url(
                Handle,
                nativeSourceId.Value,
                nativeUrl.Value,
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> ApplyCameraDeltaAsync(
        CameraDelta delta,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_apply_camera_delta"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeDelta = NativeCameraDelta(delta);
                return NativeMethods.mln_map_apply_camera_delta(Handle, &nativeDelta, completion);
            })
            .WaitAsync(cancellationToken);
    }

    public Task<CameraOptions> CameraForGeometryAsync(
        byte[] geometry,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_camera_for_geometry"
        );
        using var nativeGeometry = NativeStringView.From(geometry, nameof(geometry));
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeFitOptions = fitOptions is null
                        ? default(mln_camera_fit_options)
                        : NativeCameraFitOptions(fitOptions);
                    return NativeMethods.mln_map_camera_for_geometry(
                        Handle,
                        nativeGeometry.Value,
                        fitOptions is null ? null : &nativeFitOptions,
                        completion
                    );
                },
                result => CopyCameraOptions(NativeCompletion.Value<mln_camera_options>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CameraOptions> CameraForLatLngBoundsAsync(
        LatLngBounds bounds,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_camera_for_lat_lng_bounds"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeFitOptions = fitOptions is null
                        ? default(mln_camera_fit_options)
                        : NativeCameraFitOptions(fitOptions);
                    return NativeMethods.mln_map_camera_for_lat_lng_bounds(
                        Handle,
                        NativeLatLngBounds(bounds),
                        fitOptions is null ? null : &nativeFitOptions,
                        completion
                    );
                },
                result => CopyCameraOptions(NativeCompletion.Value<mln_camera_options>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CameraOptions> CameraForLatLngsAsync(
        LatLng[] coordinates,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_camera_for_lat_lngs"
        );
        var operation = NativeCompletion.Submit(
            completion =>
            {
                var nativeFitOptions = fitOptions is null
                    ? default(mln_camera_fit_options)
                    : NativeCameraFitOptions(fitOptions);
                return NativeMethods.mln_map_camera_for_lat_lngs(
                    Handle,
                    scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                    checked((nuint)coordinates.Length),
                    fitOptions is null ? null : &nativeFitOptions,
                    completion
                );
            },
            result => CopyCameraOptions(NativeCompletion.Value<mln_camera_options>(result))
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CameraQueryResult> CameraQueryAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_camera_query"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_camera_query(Handle, completion),
                result =>
                    CopyCameraQueryResult(NativeCompletion.Value<mln_camera_query_result>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public (CameraOptions Camera, ulong Generation) CameraSnapshotGet()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_camera_snapshot_get"
        );
        var outCamera = new mln_camera_options { size = (uint)sizeof(mln_camera_options) };
        ulong outGeneration = default;
        NativeStatus.Check(
            NativeMethods.mln_map_camera_snapshot_get(read.Handle, &outCamera, &outGeneration)
        );
        return (CopyCameraOptions(outCamera), outGeneration);
    }

    public Task<CommandCompletion> CancelTransitionsAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_cancel_transitions"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_cancel_transitions(Handle, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<string?> CopyLayerSourceIdAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_copy_layer_source_id"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_copy_layer_source_id(
                        Handle,
                        nativeLayerId.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return value.size == 0
                        ? null
                        : RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<string?> CopyLayerSourceLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_copy_layer_source_layer"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_copy_layer_source_layer(
                        Handle,
                        nativeLayerId.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return value.size == 0
                        ? null
                        : RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<byte[]?> CopyStyleImagePremultipliedRgba8Async(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_copy_style_image_premultiplied_rgba8"
        );
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_copy_style_image_premultiplied_rgba8(
                        Handle,
                        nativeImageId.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return ValueStructs.CopyBufferView(value);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleImageStretchesResult?> CopyStyleImageStretchesAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_copy_style_image_stretches"
        );
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_copy_style_image_stretches(
                        Handle,
                        nativeImageId.Value,
                        completion
                    ),
                result =>
                    result->value_count == 0
                        ? (StyleImageStretchesResult?)null
                        : CopyStyleImageStretchesResult(
                            NativeCompletion.Value<mln_style_image_stretches_result>(result)
                        )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<string?> CopyStyleSourceAttributionAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_copy_style_source_attribution"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_copy_style_source_attribution(
                        Handle,
                        nativeSourceId.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<string?> CopyStyleSourceUrlAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_copy_style_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_copy_style_source_url(
                        Handle,
                        nativeSourceId.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> DumpDebugLogsAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_dump_debug_logs"
        );
        return NativeCompletion
            .SubmitCommand(completion => NativeMethods.mln_map_dump_debug_logs(Handle, completion))
            .WaitAsync(cancellationToken);
    }

    public Task<byte[]> GetFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_feature_state"
        );
        var operation = NativeCompletion.Submit(
            completion =>
            {
                var nativeSelector = NativeFeatureStateSelector(selector, scope);
                return NativeMethods.mln_map_get_feature_state(Handle, &nativeSelector, completion);
            },
            result =>
            {
                var value = NativeCompletion.Value<mln_buffer_view>(result);
                return ValueStructs.CopyBufferView(value);
            }
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<byte[]> GetGlobalStateAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_global_state"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_get_global_state(Handle, completion),
                result =>
                {
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return ValueStructs.CopyBufferView(value);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<LatLng[]?> GetImageSourceCoordinatesAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_image_source_coordinates"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_image_source_coordinates(
                        Handle,
                        nativeSourceId.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value == null)
                        return null;
                    var values = NativeCompletion.Values<mln_lat_lng>(result);
                    var copied = new LatLng[values.Length];
                    for (var index = 0; index < values.Length; index++)
                        copied[index] = CopyLatLng(values[index]);
                    return copied;
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<byte[]?> GetLayerFilterAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_layer_filter"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_layer_filter(Handle, nativeLayerId.Value, completion),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return ValueStructs.CopyBufferView(value);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<byte[]?> GetLayerPropertyAsync(
        string layerId,
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_layer_property"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_layer_property(
                        Handle,
                        nativeLayerId.Value,
                        nativePropertyName.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return ValueStructs.CopyBufferView(value);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleImageResult?> GetStyleImageInfoAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_style_image_info"
        );
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_image_info(
                        Handle,
                        nativeImageId.Value,
                        completion
                    ),
                result =>
                    result->value_count == 0
                        ? (StyleImageResult?)null
                        : CopyStyleImageResult(
                            NativeCompletion.Value<mln_style_image_result>(result)
                        )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleLayerResult?> GetStyleLayerInfoAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_style_layer_info"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_layer_info(
                        Handle,
                        nativeLayerId.Value,
                        completion
                    ),
                result =>
                    result->value_count == 0
                        ? (StyleLayerResult?)null
                        : CopyStyleLayerResult(
                            NativeCompletion.Value<mln_style_layer_result>(result)
                        )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<byte[]?> GetStyleLayerJsonAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_style_layer_json"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_layer_json(
                        Handle,
                        nativeLayerId.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return ValueStructs.CopyBufferView(value);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<byte[]?> GetStyleLightPropertyAsync(
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_style_light_property"
        );
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_light_property(
                        Handle,
                        nativePropertyName.Value,
                        completion
                    ),
                result =>
                {
                    if (result->value_count == 0)
                        return null;
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return ValueStructs.CopyBufferView(value);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleSourceResult?> GetStyleSourceInfoAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_style_source_info"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_source_info(
                        Handle,
                        nativeSourceId.Value,
                        completion
                    ),
                result =>
                    result->value_count == 0
                        ? (StyleSourceResult?)null
                        : CopyStyleSourceResult(
                            NativeCompletion.Value<mln_style_source_result>(result)
                        )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleSourceTileUrlsResult?> GetStyleSourceTileUrlsAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_style_source_tile_urls"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_source_tile_urls(
                        Handle,
                        nativeSourceId.Value,
                        completion
                    ),
                result =>
                    result->value_count == 0
                        ? (StyleSourceTileUrlsResult?)null
                        : CopyStyleSourceTileUrlsResult(
                            NativeCompletion.Value<mln_style_source_tile_urls_result>(result)
                        )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleTransitionOptions> GetStyleTransitionOptionsAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_get_style_transition_options"
        );
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_transition_options(Handle, completion),
                result =>
                    CopyStyleTransitionOptions(
                        NativeCompletion.Value<mln_style_transition_options>(result)
                    )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> InvalidateCustomGeometrySourceRegionAsync(
        string sourceId,
        LatLngBounds bounds,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_invalidate_custom_geometry_source_region"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                    Handle,
                    nativeSourceId.Value,
                    NativeLatLngBounds(bounds),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> InvalidateCustomGeometrySourceTileAsync(
        string sourceId,
        CanonicalTileId tileId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_invalidate_custom_geometry_source_tile"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                    Handle,
                    nativeSourceId.Value,
                    NativeCanonicalTileId(tileId),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> InvalidateCustomMvtVectorSourceTileAsync(
        string sourceId,
        CanonicalTileId tileId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_invalidate_custom_mvt_vector_source_tile"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_invalidate_custom_mvt_vector_source_tile(
                    Handle,
                    nativeSourceId.Value,
                    NativeCanonicalTileId(tileId),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<LatLngBounds> LatLngBoundsForCameraAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_lat_lng_bounds_for_camera"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeCamera = NativeCameraOptions(camera);
                    return NativeMethods.mln_map_lat_lng_bounds_for_camera(
                        Handle,
                        &nativeCamera,
                        completion
                    );
                },
                result => CopyLatLngBounds(NativeCompletion.Value<mln_lat_lng_bounds>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public Task<LatLngBounds> LatLngBoundsForCameraUnwrappedAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_lat_lng_bounds_for_camera_unwrapped"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeCamera = NativeCameraOptions(camera);
                    return NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped(
                        Handle,
                        &nativeCamera,
                        completion
                    );
                },
                result => CopyLatLngBounds(NativeCompletion.Value<mln_lat_lng_bounds>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public Task<LatLng> LatLngForPixelAsync(
        ScreenPoint point,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_lat_lng_for_pixel"
        );
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_lat_lng_for_pixel(
                        Handle,
                        NativeScreenPoint(point),
                        completion
                    ),
                result => CopyLatLng(NativeCompletion.Value<mln_lat_lng>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public Task<LatLng> LatLngForPixelUnwrappedAsync(
        ScreenPoint point,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_lat_lng_for_pixel_unwrapped"
        );
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_lat_lng_for_pixel_unwrapped(
                        Handle,
                        NativeScreenPoint(point),
                        completion
                    ),
                result => CopyLatLng(NativeCompletion.Value<mln_lat_lng>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public Task<LatLng[]> LatLngsForPixelsAsync(
        ScreenPoint[] points,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_lat_lngs_for_pixels"
        );
        var operation = NativeCompletion.Submit(
            completion =>
                NativeMethods.mln_map_lat_lngs_for_pixels(
                    Handle,
                    scope.Array<mln_screen_point, ScreenPoint>(
                        points,
                        item => NativeScreenPoint(item)
                    ),
                    checked((nuint)points.Length),
                    completion
                ),
            result =>
            {
                var values = NativeCompletion.Values<mln_lat_lng>(result);
                var copied = new LatLng[values.Length];
                for (var index = 0; index < values.Length; index++)
                    copied[index] = CopyLatLng(values[index]);
                return copied;
            }
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<LatLng[]> LatLngsForPixelsUnwrappedAsync(
        ScreenPoint[] points,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_lat_lngs_for_pixels_unwrapped"
        );
        var operation = NativeCompletion.Submit(
            completion =>
                NativeMethods.mln_map_lat_lngs_for_pixels_unwrapped(
                    Handle,
                    scope.Array<mln_screen_point, ScreenPoint>(
                        points,
                        item => NativeScreenPoint(item)
                    ),
                    checked((nuint)points.Length),
                    completion
                ),
            result =>
            {
                var values = NativeCompletion.Values<mln_lat_lng>(result);
                var copied = new LatLng[values.Length];
                for (var index = 0; index < values.Length; index++)
                    copied[index] = CopyLatLng(values[index]);
                return copied;
            }
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<string[]> StyleLayerIdsAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_list_style_layer_ids"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_list_style_layer_ids(Handle, completion),
                result =>
                {
                    var values = NativeCompletion.Values<mln_buffer_view>(result);
                    var copied = new string[values.Length];
                    for (var index = 0; index < values.Length; index++)
                    {
                        var value = values[index];
                        copied[index] = RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
                    }
                    return copied;
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleLayerEntry[]> StyleLayersAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_list_style_layers"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_list_style_layers(Handle, completion),
                result =>
                {
                    var values = NativeCompletion.Values<mln_style_layer_entry>(result);
                    var copied = new StyleLayerEntry[values.Length];
                    for (var index = 0; index < values.Length; index++)
                        copied[index] = CopyStyleLayerEntry(values[index]);
                    return copied;
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<string[]> StyleSourceIdsAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_list_style_source_ids"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_list_style_source_ids(Handle, completion),
                result =>
                {
                    var values = NativeCompletion.Values<mln_buffer_view>(result);
                    var copied = new string[values.Length];
                    for (var index = 0; index < values.Length; index++)
                    {
                        var value = values[index];
                        copied[index] = RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
                    }
                    return copied;
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<byte[]> LoadedStyleJsonAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_loaded_style_json"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_loaded_style_json(Handle, completion),
                result =>
                {
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return ValueStructs.CopyBufferView(value);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<double> MetersPerPixelAtLatitudeAsync(
        double latitude,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_meters_per_pixel_at_latitude"
        );
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_meters_per_pixel_at_latitude(
                        Handle,
                        latitude,
                        completion
                    ),
                result => NativeCompletion.Value<double>(result)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> MoveStyleLayerAsync(
        string layerId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_move_style_layer"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeBeforeLayerId = NativeStringView.From(beforeLayerId, nameof(beforeLayerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_move_style_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeBeforeLayerId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<ScreenPoint> PixelForLatLngAsync(
        LatLng coordinate,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_pixel_for_lat_lng"
        );
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_pixel_for_lat_lng(
                        Handle,
                        NativeLatLng(coordinate),
                        completion
                    ),
                result => CopyScreenPoint(NativeCompletion.Value<mln_screen_point>(result))
            )
            .WaitAsync(cancellationToken);
    }

    public Task<ScreenPoint[]> PixelsForLatLngsAsync(
        LatLng[] coordinates,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_pixels_for_lat_lngs"
        );
        var operation = NativeCompletion.Submit(
            completion =>
                NativeMethods.mln_map_pixels_for_lat_lngs(
                    Handle,
                    scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                    checked((nuint)coordinates.Length),
                    completion
                ),
            result =>
            {
                var values = NativeCompletion.Values<mln_screen_point>(result);
                var copied = new ScreenPoint[values.Length];
                for (var index = 0; index < values.Length; index++)
                    copied[index] = CopyScreenPoint(values[index]);
                return copied;
            }
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<MapProjectionHandle> ProjectionCreateAsync()
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_projection_create"
        );
        return NativeCompletion.Submit(
            completion => NativeMethods.mln_map_projection_create(Handle, completion),
            result => MapProjectionHandle.Adopt(NativeCompletion.Value<MlnMapProjection>(result))
        );
    }

    public void Close() => CloseAsync().GetAwaiter().GetResult();

    public Task CloseAsync()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_release"
        );
        state.Close();
        return teardown;
    }

    private mln_status StartRelease(MlnMap handle)
    {
        teardown = NativeCompletion.SubmitUnit(completion =>
            NativeMethods.mln_map_release(handle, completion)
        );
        return mln_status.MLN_STATUS_OK;
    }

    public Task<CommandCompletion> RemoveFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_remove_feature_state"
        );
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeSelector = NativeFeatureStateSelector(selector, scope);
            return NativeMethods.mln_map_remove_feature_state(Handle, &nativeSelector, completion);
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> RemoveStyleImageAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_remove_style_image"
        );
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_remove_style_image(Handle, nativeImageId.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> RemoveStyleLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_remove_style_layer"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_remove_style_layer(Handle, nativeLayerId.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> RemoveStyleSourceAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_remove_style_source"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_remove_style_source(Handle, nativeSourceId.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> RequestRepaintAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_request_repaint"
        );
        return NativeCompletion
            .SubmitCommand(completion => NativeMethods.mln_map_request_repaint(Handle, completion))
            .WaitAsync(cancellationToken);
    }

    public Task RequestStillImageAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_request_still_image"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_request_still_image(Handle, completion),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> ResizeAsync(
        LogicalExtent extent,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_resize"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_resize(Handle, NativeLogicalExtent(extent), completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetBoundsAsync(
        BoundOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_bounds"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = NativeBoundOptions(options);
                return NativeMethods.mln_map_set_bounds(Handle, &nativeOptions, completion);
            })
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetCustomGeometrySourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_custom_geometry_source_tile_data"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeStringView.From(data, nameof(data));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                    Handle,
                    nativeSourceId.Value,
                    NativeCanonicalTileId(tileId),
                    nativeData.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetCustomMvtVectorSourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_custom_mvt_vector_source_tile_data"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeData = NativeStringView.From(data, nameof(data));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_custom_mvt_vector_source_tile_data(
                    Handle,
                    nativeSourceId.Value,
                    NativeCanonicalTileId(tileId),
                    nativeData.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetCustomMvtVectorSourceTileErrorAsync(
        string sourceId,
        CanonicalTileId tileId,
        string message,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_custom_mvt_vector_source_tile_error"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeMessage = NativeStringView.From(message, nameof(message));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_custom_mvt_vector_source_tile_error(
                    Handle,
                    nativeSourceId.Value,
                    NativeCanonicalTileId(tileId),
                    nativeMessage.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetDebugOptionsAsync(
        MapDebugOption options,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_debug_options"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_debug_options(Handle, (uint)options, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetEventMaskAsync(
        RuntimeEventMask mask,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_event_mask"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_event_mask(Handle, (ulong)mask, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetFeatureStateAsync(
        FeatureStateSelector selector,
        byte[] state,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_feature_state"
        );
        using var nativeState = NativeStringView.From(state, nameof(state));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeSelector = NativeFeatureStateSelector(selector, scope);
            return NativeMethods.mln_map_set_feature_state(
                Handle,
                &nativeSelector,
                nativeState.Value,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetFreeCameraOptionsAsync(
        FreeCameraOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_free_camera_options"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = NativeFreeCameraOptions(options);
                return NativeMethods.mln_map_set_free_camera_options(
                    Handle,
                    &nativeOptions,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetGeojsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_geojson_source_data"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        ArgumentNullException.ThrowIfNull(data);
        using var useData = data.Borrow();
        var handleData = useData.Handle;
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_geojson_source_data(
                    Handle,
                    nativeSourceId.Value,
                    handleData,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetGeojsonSourceSynchronousTilingAsync(
        string sourceId,
        bool enabled,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_geojson_source_synchronous_tiling"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
                    Handle,
                    nativeSourceId.Value,
                    (byte)(enabled ? 1 : 0),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetGeojsonSourceUrlAsync(
        string sourceId,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_geojson_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_geojson_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetGlobalStatePropertyAsync(
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_global_state_property"
        );
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_global_state_property(
                    Handle,
                    nativePropertyName.Value,
                    nativeValue.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetImageSourceCoordinatesAsync(
        string sourceId,
        LatLng[] coordinates,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_image_source_coordinates"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
            NativeMethods.mln_map_set_image_source_coordinates(
                Handle,
                nativeSourceId.Value,
                scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                checked((nuint)coordinates.Length),
                completion
            )
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetImageSourceImageAsync(
        string sourceId,
        PremultipliedRgba8Image image,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_image_source_image"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeImage = NativePremultipliedRgba8Image(image, scope);
            return NativeMethods.mln_map_set_image_source_image(
                Handle,
                nativeSourceId.Value,
                &nativeImage,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetImageSourceUrlAsync(
        string sourceId,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_image_source_url"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeUrl = NativeStringView.From(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_image_source_url(
                    Handle,
                    nativeSourceId.Value,
                    nativeUrl.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLayerFilterAsync(
        string layerId,
        byte[]? filter,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_layer_filter"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeFilter = filter is null ? default(mln_buffer_view) : scope.Buffer(filter);
            return NativeMethods.mln_map_set_layer_filter(
                Handle,
                nativeLayerId.Value,
                filter is null ? null : &nativeFilter,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLayerMaxZoomAsync(
        string layerId,
        double maxZoom,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_layer_max_zoom"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_max_zoom(
                    Handle,
                    nativeLayerId.Value,
                    maxZoom,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLayerMinZoomAsync(
        string layerId,
        double minZoom,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_layer_min_zoom"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_min_zoom(
                    Handle,
                    nativeLayerId.Value,
                    minZoom,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLayerPropertyAsync(
        string layerId,
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_layer_property"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_property(
                    Handle,
                    nativeLayerId.Value,
                    nativePropertyName.Value,
                    nativeValue.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLayerSourceIdAsync(
        string layerId,
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_layer_source_id"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_source_id(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLayerSourceLayerAsync(
        string layerId,
        string sourceLayer,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_layer_source_layer"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeSourceLayer = NativeStringView.From(sourceLayer, nameof(sourceLayer));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_source_layer(
                    Handle,
                    nativeLayerId.Value,
                    nativeSourceLayer.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLayerVisibilityAsync(
        string layerId,
        StyleLayerVisibility visibility,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_layer_visibility"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_layer_visibility(
                    Handle,
                    nativeLayerId.Value,
                    (uint)visibility,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLocationIndicatorAccuracyRadiusAsync(
        string layerId,
        double radius,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_location_indicator_accuracy_radius"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_accuracy_radius(
                    Handle,
                    nativeLayerId.Value,
                    radius,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLocationIndicatorBearingAsync(
        string layerId,
        double bearing,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_location_indicator_bearing"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_bearing(
                    Handle,
                    nativeLayerId.Value,
                    bearing,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLocationIndicatorImageNameAsync(
        string layerId,
        LocationIndicatorImageKind imageKind,
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_location_indicator_image_name"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_image_name(
                    Handle,
                    nativeLayerId.Value,
                    (uint)imageKind,
                    nativeImageId.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetLocationIndicatorLocationAsync(
        string layerId,
        LatLng coordinate,
        double altitude,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_location_indicator_location"
        );
        using var nativeLayerId = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_location_indicator_location(
                    Handle,
                    nativeLayerId.Value,
                    NativeLatLng(coordinate),
                    altitude,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetProjectionModeAsync(
        ProjectionMode mode,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_projection_mode"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeMode = NativeProjectionMode(mode);
                return NativeMethods.mln_map_set_projection_mode(Handle, &nativeMode, completion);
            })
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetRenderingStatsViewEnabledAsync(
        bool enabled,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_rendering_stats_view_enabled"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_rendering_stats_view_enabled(
                    Handle,
                    (byte)(enabled ? 1 : 0),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetStyleImageAsync(
        string imageId,
        PremultipliedRgba8Image image,
        StyleImageOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_style_image"
        );
        using var nativeImageId = NativeStringView.From(imageId, nameof(imageId));
        var operation = NativeCompletion.SubmitCommand(completion =>
        {
            var nativeImage = NativePremultipliedRgba8Image(image, scope);
            var nativeOptions = options is null
                ? default(mln_style_image_options)
                : NativeStyleImageOptions(options, scope);
            return NativeMethods.mln_map_set_style_image(
                Handle,
                nativeImageId.Value,
                &nativeImage,
                options is null ? null : &nativeOptions,
                completion
            );
        });
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetStyleJsonAsync(
        byte[] json,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_style_json"
        );
        using var nativeJson = NativeStringView.From(json, nameof(json));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_json(Handle, nativeJson.Value, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetStyleLightJsonAsync(
        byte[] lightJson,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_style_light_json"
        );
        using var nativeLightJson = NativeStringView.From(lightJson, nameof(lightJson));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_light_json(
                    Handle,
                    nativeLightJson.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetStyleLightPropertyAsync(
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_style_light_property"
        );
        using var nativePropertyName = NativeStringView.From(propertyName, nameof(propertyName));
        using var nativeValue = NativeStringView.From(value, nameof(value));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_light_property(
                    Handle,
                    nativePropertyName.Value,
                    nativeValue.Value,
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetStyleSourceVolatileAsync(
        string sourceId,
        bool isVolatile,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_style_source_volatile"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_source_volatile(
                    Handle,
                    nativeSourceId.Value,
                    (byte)(isVolatile ? 1 : 0),
                    completion
                )
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetStyleTransitionOptionsAsync(
        StyleTransitionOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_style_transition_options"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = NativeStyleTransitionOptions(options);
                return NativeMethods.mln_map_set_style_transition_options(
                    Handle,
                    &nativeOptions,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetStyleUrlAsync(
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_style_url"
        );
        ArgumentNullException.ThrowIfNull(url);
        using var nativeUrl = NativeUtf8String.FromNullableString(url, nameof(url));
        return NativeCompletion
            .SubmitCommand(completion =>
                NativeMethods.mln_map_set_style_url(Handle, nativeUrl.Pointer, completion)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetTileOptionsAsync(
        MapTileOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_tile_options"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = NativeMapTileOptions(options);
                return NativeMethods.mln_map_set_tile_options(Handle, &nativeOptions, completion);
            })
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> SetViewportOptionsAsync(
        MapViewportOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_set_viewport_options"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeOptions = NativeMapViewportOptions(options);
                return NativeMethods.mln_map_set_viewport_options(
                    Handle,
                    &nativeOptions,
                    completion
                );
            })
            .WaitAsync(cancellationToken);
    }

    public MapSnapshot SnapshotGet()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_snapshot_get"
        );
        var outSnapshot = new mln_map_snapshot { size = (uint)sizeof(mln_map_snapshot) };
        NativeStatus.Check(NativeMethods.mln_map_snapshot_get(read.Handle, &outSnapshot));
        return CopyMapSnapshot(outSnapshot);
    }

    public Task<string> StyleUrlAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_style_url"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_map_style_url(Handle, completion),
                result =>
                {
                    var value = NativeCompletion.Value<mln_buffer_view>(result);
                    return RuntimeStructs.CopyUtf8((sbyte*)value.data, value.size);
                }
            )
            .WaitAsync(cancellationToken);
    }

    public Task<CommandCompletion> UpdateCameraAsync(
        CameraUpdate update,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_map_update_camera"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeUpdate = NativeCameraUpdate(update);
                return NativeMethods.mln_map_update_camera(Handle, &nativeUpdate, completion);
            })
            .WaitAsync(cancellationToken);
    }

    public RenderSessionHandle MetalBorrowedTextureAttach(
        MetalBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_metal_borrowed_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeMetalBorrowedTextureDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_metal_borrowed_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle MetalOwnedTextureAttach(
        MetalOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_metal_owned_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeMetalOwnedTextureDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_metal_owned_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle MetalSurfaceAttach(
        MetalSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_metal_surface_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeMetalSurfaceDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_metal_surface_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle OpenglBorrowedTextureAttach(
        OpenglBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_opengl_borrowed_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeOpenglBorrowedTextureDescriptor(descriptor, scope);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_opengl_borrowed_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle OpenglOwnedTextureAttach(
        OpenglOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_opengl_owned_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeOpenglOwnedTextureDescriptor(descriptor, scope);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_opengl_owned_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle OpenglSurfaceAttach(
        OpenglSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_opengl_surface_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeOpenglSurfaceDescriptor(descriptor, scope);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_opengl_surface_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle VulkanBorrowedTextureAttach(
        VulkanBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_vulkan_borrowed_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeVulkanBorrowedTextureDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_vulkan_borrowed_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle VulkanOwnedTextureAttach(
        VulkanOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_vulkan_owned_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeVulkanOwnedTextureDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_vulkan_owned_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle VulkanSurfaceAttach(
        VulkanSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_vulkan_surface_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeVulkanSurfaceDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_vulkan_surface_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle WebgpuBorrowedTextureAttach(
        WebgpuBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_webgpu_borrowed_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeWebgpuBorrowedTextureDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_webgpu_borrowed_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle WebgpuOwnedTextureAttach(
        WebgpuOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_webgpu_owned_texture_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeWebgpuOwnedTextureDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_webgpu_owned_texture_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }

    public RenderSessionHandle WebgpuSurfaceAttach(
        WebgpuSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_webgpu_surface_attach"
        );
        MlnRenderSession outSession = default;
        var attachment = NativeCompletion.SubmitUnit(completion =>
        {
            MlnRenderSession nativeOutSession = default;
            var nativeDescriptor = NativeWebgpuSurfaceDescriptor(descriptor);
            var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
            var status = NativeMethods.mln_webgpu_surface_attach(
                Handle,
                &nativeDescriptor,
                &nativeOptions,
                &nativeOutSession,
                completion
            );
            if (status == mln_status.MLN_STATUS_OK)
                outSession = nativeOutSession;
            return status;
        });
        var owner = RenderSessionHandle.Adopt(this, outSession, attachment);
        scope.Accept(owner.CallbackOwner);
        return owner;
    }
}
