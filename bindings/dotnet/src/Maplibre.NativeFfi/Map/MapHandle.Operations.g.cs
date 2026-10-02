// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Map;

public sealed unsafe partial class MapHandle : IDisposable, IAsyncDisposable, INativeOwner<MlnMap>
{
    private readonly NativeHandleState<MlnMap> state;
    private volatile Task teardown = Task.CompletedTask;

    internal MapHandle(RuntimeHandle parent, MlnMap handle)
    {
        state = new(handle, StartRelease, nameof(MapHandle), Abandon, retainedParent: parent);
    }

    internal static MapHandle Adopt(RuntimeHandle parent, MlnMap handle) =>
        NativeHandleState<MlnMap>.Adopt(handle, () => new MapHandle(parent, handle), Abandon);

    private static mln_status Abandon(MlnMap live, mln_diagnostic* diagnostic) =>
        NativeMethods.mln_map_dispose(live, diagnostic);

    NativeHandleState<MlnMap> INativeOwner<MlnMap>.State => state;
    internal MlnMap Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_map_dispose");
        state.Retire();
    }

    public Task<CommandCompletion> AddColorReliefLayerAsync(
        string layerId,
        string sourceId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_color_relief_layer");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_color_relief_layer(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(sourceId),
                    scope.Utf8(beforeLayerId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddCustomGeometrySourceAsync(
        string sourceId,
        CustomGeometrySourceOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_custom_geometry_source");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_custom_geometry_source(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Value(NativeCustomGeometrySourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddCustomMvtVectorSourceAsync(
        string sourceId,
        CustomMvtVectorSourceOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_custom_mvt_vector_source");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_custom_mvt_vector_source(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Value(NativeCustomMvtVectorSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddGeojsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_geojson_source_data");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_geojson_source_data(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Use(data),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddGeojsonSourceUrlAsync(
        string sourceId,
        string url,
        GeojsonSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_geojson_source_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_geojson_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    options is null
                        ? null
                        : scope.Value(NativeGeojsonSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddHillshadeLayerAsync(
        string layerId,
        string sourceId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_hillshade_layer");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_hillshade_layer(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(sourceId),
                    scope.Utf8(beforeLayerId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddImageSourceImageAsync(
        string sourceId,
        LatLng[] coordinates,
        PremultipliedRgba8Image image,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_image_source_image");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_image_source_image(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                    checked((nuint)coordinates.Length),
                    scope.Value(NativePremultipliedRgba8Image(image, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddImageSourceUrlAsync(
        string sourceId,
        LatLng[] coordinates,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_image_source_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_image_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                    checked((nuint)coordinates.Length),
                    scope.Utf8(url),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddLocationIndicatorLayerAsync(
        string layerId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_location_indicator_layer");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_location_indicator_layer(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(beforeLayerId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddRasterDemSourceTilesAsync(
        string sourceId,
        string[] tiles,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_raster_dem_source_tiles");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_raster_dem_source_tiles(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_buffer_view, string>(tiles, item => scope.Utf8(item)),
                    checked((nuint)tiles.Length),
                    options is null
                        ? null
                        : scope.Value(NativeStyleTileSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddRasterDemSourceUrlAsync(
        string sourceId,
        string url,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_raster_dem_source_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_raster_dem_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    options is null
                        ? null
                        : scope.Value(NativeStyleTileSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddRasterSourceTilesAsync(
        string sourceId,
        string[] tiles,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_raster_source_tiles");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_raster_source_tiles(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_buffer_view, string>(tiles, item => scope.Utf8(item)),
                    checked((nuint)tiles.Length),
                    options is null
                        ? null
                        : scope.Value(NativeStyleTileSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddRasterSourceUrlAsync(
        string sourceId,
        string url,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_raster_source_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_raster_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    options is null
                        ? null
                        : scope.Value(NativeStyleTileSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddStyleLayerJsonAsync(
        byte[] layerJson,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_style_layer_json");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_style_layer_json(
                    Handle,
                    scope.Buffer(layerJson),
                    scope.Utf8(beforeLayerId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddStyleSourceJsonAsync(
        string sourceId,
        byte[] sourceJson,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_style_source_json");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_style_source_json(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Buffer(sourceJson),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddVectorSourceTilesAsync(
        string sourceId,
        string[] tiles,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_vector_source_tiles");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_vector_source_tiles(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_buffer_view, string>(tiles, item => scope.Utf8(item)),
                    checked((nuint)tiles.Length),
                    options is null
                        ? null
                        : scope.Value(NativeStyleTileSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> AddVectorSourceUrlAsync(
        string sourceId,
        string url,
        StyleTileSourceOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_add_vector_source_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_add_vector_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    options is null
                        ? null
                        : scope.Value(NativeStyleTileSourceOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> ApplyCameraDeltaAsync(
        CameraDelta delta,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_apply_camera_delta");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_apply_camera_delta(
                    Handle,
                    scope.Value(NativeCameraDelta(delta)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CameraOptions> CameraForGeometryAsync(
        byte[] geometry,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_camera_for_geometry");
        return scope.Query<mln_camera_options, CameraOptions>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_camera_for_geometry(
                    Handle,
                    scope.Buffer(geometry),
                    fitOptions is null ? null : scope.Value(NativeCameraFitOptions(fitOptions)),
                    completion,
                    diagnostic
                ),
            CopyCameraOptions,
            cancellationToken
        );
    }

    public Task<CameraOptions> CameraForLatLngBoundsAsync(
        LatLngBounds bounds,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_camera_for_lat_lng_bounds");
        return scope.Query<mln_camera_options, CameraOptions>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_camera_for_lat_lng_bounds(
                    Handle,
                    NativeLatLngBounds(bounds),
                    fitOptions is null ? null : scope.Value(NativeCameraFitOptions(fitOptions)),
                    completion,
                    diagnostic
                ),
            CopyCameraOptions,
            cancellationToken
        );
    }

    public Task<CameraOptions> CameraForLatLngsAsync(
        LatLng[] coordinates,
        CameraFitOptions? fitOptions,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_camera_for_lat_lngs");
        return scope.Query<mln_camera_options, CameraOptions>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_camera_for_lat_lngs(
                    Handle,
                    scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                    checked((nuint)coordinates.Length),
                    fitOptions is null ? null : scope.Value(NativeCameraFitOptions(fitOptions)),
                    completion,
                    diagnostic
                ),
            CopyCameraOptions,
            cancellationToken
        );
    }

    public Task<CameraQueryResult> CameraQueryAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_camera_query");
        return scope.Query<mln_camera_query_result, CameraQueryResult>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_camera_query(Handle, completion, diagnostic),
            CopyCameraQueryResult,
            cancellationToken
        );
    }

    public (CameraOptions Camera, ulong Generation) CameraSnapshotGet()
    {
        using var read = state.Read(this, "mln_map_camera_snapshot_get");
        var outCamera = new mln_camera_options { size = (uint)sizeof(mln_camera_options) };
        ulong outGeneration = default;
        Check(
            NativeMethods.mln_map_camera_snapshot_get(
                read.Handle,
                &outCamera,
                &outGeneration,
                Diagnostic
            )
        );
        return (CopyCameraOptions(outCamera), outGeneration);
    }

    public Task<CommandCompletion> CancelTransitionsAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_cancel_transitions");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_cancel_transitions(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public Task<string?> CopyLayerSourceIdAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_copy_layer_source_id");
        return scope.QueryOptional<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_copy_layer_source_id(
                    Handle,
                    scope.Utf8(layerId),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyOptionalUtf8View,
            cancellationToken
        );
    }

    public Task<string?> CopyLayerSourceLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_copy_layer_source_layer");
        return scope.QueryOptional<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_copy_layer_source_layer(
                    Handle,
                    scope.Utf8(layerId),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyOptionalUtf8View,
            cancellationToken
        );
    }

    public Task<byte[]?> CopyStyleImagePremultipliedRgba8Async(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_copy_style_image_premultiplied_rgba8");
        return scope.QueryOptional<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_copy_style_image_premultiplied_rgba8(
                    Handle,
                    scope.Utf8(imageId),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<StyleImageStretchesResult?> CopyStyleImageStretchesAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_copy_style_image_stretches");
        return scope.QueryOptionalValue<
            mln_style_image_stretches_result,
            StyleImageStretchesResult
        >(
            (completion, diagnostic) =>
                NativeMethods.mln_map_copy_style_image_stretches(
                    Handle,
                    scope.Utf8(imageId),
                    completion,
                    diagnostic
                ),
            CopyStyleImageStretchesResult,
            cancellationToken
        );
    }

    public Task<string?> CopyStyleSourceAttributionAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_copy_style_source_attribution");
        return scope.QueryOptional<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_copy_style_source_attribution(
                    Handle,
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyUtf8View,
            cancellationToken
        );
    }

    public Task<string?> CopyStyleSourceUrlAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_copy_style_source_url");
        return scope.QueryOptional<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_copy_style_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyUtf8View,
            cancellationToken
        );
    }

    public Task<CommandCompletion> DumpDebugLogsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_dump_debug_logs");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_dump_debug_logs(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public Task<byte[]> GetFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_feature_state");
        return scope.Query<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_feature_state(
                    Handle,
                    scope.Value(NativeFeatureStateSelector(selector, scope)),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<byte[]> GetGlobalStateAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_get_global_state");
        return scope.Query<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_global_state(Handle, completion, diagnostic),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<LatLng[]?> GetImageSourceCoordinatesAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_image_source_coordinates");
        return scope.QueryOptionalArray<mln_lat_lng, LatLng>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_image_source_coordinates(
                    Handle,
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            CopyLatLng,
            cancellationToken
        );
    }

    public Task<byte[]?> GetLayerFilterAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_layer_filter");
        return scope.QueryOptional<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_layer_filter(
                    Handle,
                    scope.Utf8(layerId),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<byte[]?> GetLayerPropertyAsync(
        string layerId,
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_layer_property");
        return scope.QueryOptional<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_layer_property(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(propertyName),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<StyleImageResult?> GetStyleImageInfoAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_style_image_info");
        return scope.QueryOptionalValue<mln_style_image_result, StyleImageResult>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_image_info(
                    Handle,
                    scope.Utf8(imageId),
                    completion,
                    diagnostic
                ),
            CopyStyleImageResult,
            cancellationToken
        );
    }

    public Task<StyleLayerResult?> GetStyleLayerInfoAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_style_layer_info");
        return scope.QueryOptionalValue<mln_style_layer_result, StyleLayerResult>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_layer_info(
                    Handle,
                    scope.Utf8(layerId),
                    completion,
                    diagnostic
                ),
            CopyStyleLayerResult,
            cancellationToken
        );
    }

    public Task<byte[]?> GetStyleLayerJsonAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_style_layer_json");
        return scope.QueryOptional<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_layer_json(
                    Handle,
                    scope.Utf8(layerId),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<byte[]?> GetStyleLightPropertyAsync(
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_style_light_property");
        return scope.QueryOptional<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_light_property(
                    Handle,
                    scope.Utf8(propertyName),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<StyleSourceResult?> GetStyleSourceInfoAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_style_source_info");
        return scope.QueryOptional<mln_style_source_result, StyleSourceResult>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_source_info(
                    Handle,
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            CopyStyleSourceResult,
            cancellationToken
        );
    }

    public Task<StyleSourceTileUrlsResult?> GetStyleSourceTileUrlsAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_style_source_tile_urls");
        return scope.QueryOptionalValue<
            mln_style_source_tile_urls_result,
            StyleSourceTileUrlsResult
        >(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_source_tile_urls(
                    Handle,
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            CopyStyleSourceTileUrlsResult,
            cancellationToken
        );
    }

    public Task<StyleTransitionOptions> GetStyleTransitionOptionsAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_get_style_transition_options");
        return scope.Query<mln_style_transition_options, StyleTransitionOptions>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_transition_options(Handle, completion, diagnostic),
            CopyStyleTransitionOptions,
            cancellationToken
        );
    }

    public Task<CommandCompletion> InvalidateCustomGeometrySourceRegionAsync(
        string sourceId,
        LatLngBounds bounds,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_map_invalidate_custom_geometry_source_region"
        );
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                    Handle,
                    scope.Utf8(sourceId),
                    NativeLatLngBounds(bounds),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> InvalidateCustomGeometrySourceTileAsync(
        string sourceId,
        CanonicalTileId tileId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_map_invalidate_custom_geometry_source_tile"
        );
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                    Handle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> InvalidateCustomMvtVectorSourceTileAsync(
        string sourceId,
        CanonicalTileId tileId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_map_invalidate_custom_mvt_vector_source_tile"
        );
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_invalidate_custom_mvt_vector_source_tile(
                    Handle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<LatLngBounds> LatLngBoundsForCameraAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_lat_lng_bounds_for_camera");
        return scope.Query<mln_lat_lng_bounds, LatLngBounds>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_lat_lng_bounds_for_camera(
                    Handle,
                    scope.Value(NativeCameraOptions(camera)),
                    completion,
                    diagnostic
                ),
            CopyLatLngBounds,
            cancellationToken
        );
    }

    public Task<LatLngBounds> LatLngBoundsForCameraUnwrappedAsync(
        CameraOptions camera,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_lat_lng_bounds_for_camera_unwrapped");
        return scope.Query<mln_lat_lng_bounds, LatLngBounds>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped(
                    Handle,
                    scope.Value(NativeCameraOptions(camera)),
                    completion,
                    diagnostic
                ),
            CopyLatLngBounds,
            cancellationToken
        );
    }

    public Task<LatLng> LatLngForPixelAsync(
        ScreenPoint point,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_lat_lng_for_pixel");
        return scope.Query<mln_lat_lng, LatLng>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_lat_lng_for_pixel(
                    Handle,
                    NativeScreenPoint(point),
                    completion,
                    diagnostic
                ),
            CopyLatLng,
            cancellationToken
        );
    }

    public Task<LatLng> LatLngForPixelUnwrappedAsync(
        ScreenPoint point,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_lat_lng_for_pixel_unwrapped");
        return scope.Query<mln_lat_lng, LatLng>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_lat_lng_for_pixel_unwrapped(
                    Handle,
                    NativeScreenPoint(point),
                    completion,
                    diagnostic
                ),
            CopyLatLng,
            cancellationToken
        );
    }

    public Task<LatLng[]> LatLngsForPixelsAsync(
        ScreenPoint[] points,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_lat_lngs_for_pixels");
        return scope.QueryArray<mln_lat_lng, LatLng>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_lat_lngs_for_pixels(
                    Handle,
                    scope.Array<mln_screen_point, ScreenPoint>(
                        points,
                        item => NativeScreenPoint(item)
                    ),
                    checked((nuint)points.Length),
                    completion,
                    diagnostic
                ),
            CopyLatLng,
            cancellationToken
        );
    }

    public Task<LatLng[]> LatLngsForPixelsUnwrappedAsync(
        ScreenPoint[] points,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_lat_lngs_for_pixels_unwrapped");
        return scope.QueryArray<mln_lat_lng, LatLng>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_lat_lngs_for_pixels_unwrapped(
                    Handle,
                    scope.Array<mln_screen_point, ScreenPoint>(
                        points,
                        item => NativeScreenPoint(item)
                    ),
                    checked((nuint)points.Length),
                    completion,
                    diagnostic
                ),
            CopyLatLng,
            cancellationToken
        );
    }

    public Task<string[]> StyleLayerIdsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_list_style_layer_ids");
        return scope.QueryArray<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_layer_ids(Handle, completion, diagnostic),
            ValueStructs.CopyUtf8View,
            cancellationToken
        );
    }

    public Task<StyleLayerEntry[]> StyleLayersAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_list_style_layers");
        return scope.QueryArray<mln_style_layer_entry, StyleLayerEntry>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_layers(Handle, completion, diagnostic),
            CopyStyleLayerEntry,
            cancellationToken
        );
    }

    public Task<string[]> StyleSourceIdsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_list_style_source_ids");
        return scope.QueryArray<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_source_ids(Handle, completion, diagnostic),
            ValueStructs.CopyUtf8View,
            cancellationToken
        );
    }

    public Task<byte[]> LoadedStyleJsonAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_loaded_style_json");
        return scope.Query<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_loaded_style_json(Handle, completion, diagnostic),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<double> MetersPerPixelAtLatitudeAsync(
        double latitude,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_meters_per_pixel_at_latitude");
        return scope.Query<double, double>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_meters_per_pixel_at_latitude(
                    Handle,
                    latitude,
                    completion,
                    diagnostic
                ),
            static value => value,
            cancellationToken
        );
    }

    public Task<CommandCompletion> MoveStyleLayerAsync(
        string layerId,
        string beforeLayerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_move_style_layer");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_move_style_layer(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(beforeLayerId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<ScreenPoint> PixelForLatLngAsync(
        LatLng coordinate,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_pixel_for_lat_lng");
        return scope.Query<mln_screen_point, ScreenPoint>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_pixel_for_lat_lng(
                    Handle,
                    NativeLatLng(coordinate),
                    completion,
                    diagnostic
                ),
            CopyScreenPoint,
            cancellationToken
        );
    }

    public Task<ScreenPoint[]> PixelsForLatLngsAsync(
        LatLng[] coordinates,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_pixels_for_lat_lngs");
        return scope.QueryArray<mln_screen_point, ScreenPoint>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_pixels_for_lat_lngs(
                    Handle,
                    scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                    checked((nuint)coordinates.Length),
                    completion,
                    diagnostic
                ),
            CopyScreenPoint,
            cancellationToken
        );
    }

    public Task<MapProjectionHandle> ProjectionCreateAsync()
    {
        using var scope = new NativeCallScope(this, "mln_map_projection_create");
        return scope.Query<MlnMapProjection, MapProjectionHandle>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_projection_create(Handle, completion, diagnostic),
            handle => MapProjectionHandle.Adopt(handle)
        );
    }

    public void Close() => CloseAsync().GetAwaiter().GetResult();

    public Task CloseAsync()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_map_release");
        state.Close();
        return teardown;
    }

    public ValueTask DisposeAsync() => new(CloseAsync());

    private mln_status StartRelease(MlnMap handle, mln_diagnostic* _)
    {
        teardown = NativeCompletion.SubmitUnit(
            (completion, diagnostic) =>
                NativeMethods.mln_map_release(handle, completion, diagnostic)
        );
        return mln_status.MLN_STATUS_OK;
    }

    public Task<CommandCompletion> RemoveFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_remove_feature_state");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_remove_feature_state(
                    Handle,
                    scope.Value(NativeFeatureStateSelector(selector, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> RemoveStyleImageAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_remove_style_image");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_remove_style_image(
                    Handle,
                    scope.Utf8(imageId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> RemoveStyleLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_remove_style_layer");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_remove_style_layer(
                    Handle,
                    scope.Utf8(layerId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> RemoveStyleSourceAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_remove_style_source");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_remove_style_source(
                    Handle,
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> RequestRepaintAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_request_repaint");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_request_repaint(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public Task RequestStillImageAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_request_still_image");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_map_request_still_image(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public Task<CommandCompletion> ResizeAsync(
        LogicalExtent extent,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_resize");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_resize(
                    Handle,
                    NativeLogicalExtent(extent),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetBoundsAsync(
        BoundOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_bounds");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_bounds(
                    Handle,
                    scope.Value(NativeBoundOptions(options)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetCustomGeometrySourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_custom_geometry_source_tile_data");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                    Handle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    scope.Buffer(data),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetCustomMvtVectorSourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        byte[] data,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_map_set_custom_mvt_vector_source_tile_data"
        );
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_custom_mvt_vector_source_tile_data(
                    Handle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    scope.Buffer(data),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetCustomMvtVectorSourceTileErrorAsync(
        string sourceId,
        CanonicalTileId tileId,
        string message,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_map_set_custom_mvt_vector_source_tile_error"
        );
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_custom_mvt_vector_source_tile_error(
                    Handle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    scope.Utf8(message),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetDebugOptionsAsync(
        MapDebugOption options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_debug_options");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_debug_options(
                    Handle,
                    (uint)options,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetEventMaskAsync(
        RuntimeEventMask mask,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_event_mask");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_event_mask(Handle, (ulong)mask, completion, diagnostic),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetFeatureStateAsync(
        FeatureStateSelector selector,
        byte[] state,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_feature_state");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_feature_state(
                    Handle,
                    scope.Value(NativeFeatureStateSelector(selector, scope)),
                    scope.Buffer(state),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetFreeCameraOptionsAsync(
        FreeCameraOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_free_camera_options");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_free_camera_options(
                    Handle,
                    scope.Value(NativeFreeCameraOptions(options)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetGeojsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_geojson_source_data");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_geojson_source_data(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Use(data),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetGeojsonSourceSynchronousTilingAsync(
        string sourceId,
        bool enabled,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_map_set_geojson_source_synchronous_tiling"
        );
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
                    Handle,
                    scope.Utf8(sourceId),
                    (byte)(enabled ? 1 : 0),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetGeojsonSourceUrlAsync(
        string sourceId,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_geojson_source_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_geojson_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetGlobalStatePropertyAsync(
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_global_state_property");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_global_state_property(
                    Handle,
                    scope.Utf8(propertyName),
                    scope.Buffer(value),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetImageSourceCoordinatesAsync(
        string sourceId,
        LatLng[] coordinates,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_image_source_coordinates");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_image_source_coordinates(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                    checked((nuint)coordinates.Length),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetImageSourceImageAsync(
        string sourceId,
        PremultipliedRgba8Image image,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_image_source_image");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_image_source_image(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Value(NativePremultipliedRgba8Image(image, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetImageSourceUrlAsync(
        string sourceId,
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_image_source_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_image_source_url(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLayerFilterAsync(
        string layerId,
        byte[]? filter,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_layer_filter");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_layer_filter(
                    Handle,
                    scope.Utf8(layerId),
                    filter is null ? null : scope.Value(scope.Buffer(filter)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLayerMaxZoomAsync(
        string layerId,
        double maxZoom,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_layer_max_zoom");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_layer_max_zoom(
                    Handle,
                    scope.Utf8(layerId),
                    maxZoom,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLayerMinZoomAsync(
        string layerId,
        double minZoom,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_layer_min_zoom");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_layer_min_zoom(
                    Handle,
                    scope.Utf8(layerId),
                    minZoom,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLayerPropertyAsync(
        string layerId,
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_layer_property");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_layer_property(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(propertyName),
                    scope.Buffer(value),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLayerSourceIdAsync(
        string layerId,
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_layer_source_id");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_layer_source_id(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLayerSourceLayerAsync(
        string layerId,
        string sourceLayer,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_layer_source_layer");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_layer_source_layer(
                    Handle,
                    scope.Utf8(layerId),
                    scope.Utf8(sourceLayer),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLayerVisibilityAsync(
        string layerId,
        StyleLayerVisibility visibility,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_layer_visibility");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_layer_visibility(
                    Handle,
                    scope.Utf8(layerId),
                    (uint)visibility,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLocationIndicatorAccuracyRadiusAsync(
        string layerId,
        double radius,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(
            this,
            "mln_map_set_location_indicator_accuracy_radius"
        );
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_location_indicator_accuracy_radius(
                    Handle,
                    scope.Utf8(layerId),
                    radius,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLocationIndicatorBearingAsync(
        string layerId,
        double bearing,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_location_indicator_bearing");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_location_indicator_bearing(
                    Handle,
                    scope.Utf8(layerId),
                    bearing,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLocationIndicatorImageNameAsync(
        string layerId,
        LocationIndicatorImageKind imageKind,
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_location_indicator_image_name");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_location_indicator_image_name(
                    Handle,
                    scope.Utf8(layerId),
                    (uint)imageKind,
                    scope.Utf8(imageId),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetLocationIndicatorLocationAsync(
        string layerId,
        LatLng coordinate,
        double altitude,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_location_indicator_location");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_location_indicator_location(
                    Handle,
                    scope.Utf8(layerId),
                    NativeLatLng(coordinate),
                    altitude,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetProjectionModeAsync(
        ProjectionMode mode,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_projection_mode");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_projection_mode(
                    Handle,
                    scope.Value(NativeProjectionMode(mode)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetRenderingStatsViewEnabledAsync(
        bool enabled,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_rendering_stats_view_enabled");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_rendering_stats_view_enabled(
                    Handle,
                    (byte)(enabled ? 1 : 0),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetStyleImageAsync(
        string imageId,
        PremultipliedRgba8Image image,
        StyleImageOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_style_image");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_image(
                    Handle,
                    scope.Utf8(imageId),
                    scope.Value(NativePremultipliedRgba8Image(image, scope)),
                    options is null ? null : scope.Value(NativeStyleImageOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetStyleJsonAsync(
        byte[] json,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_style_json");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_json(
                    Handle,
                    scope.Buffer(json),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetStyleLightJsonAsync(
        byte[] lightJson,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_style_light_json");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_light_json(
                    Handle,
                    scope.Buffer(lightJson),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetStyleLightPropertyAsync(
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_style_light_property");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_light_property(
                    Handle,
                    scope.Utf8(propertyName),
                    scope.Buffer(value),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetStyleSourceVolatileAsync(
        string sourceId,
        bool isVolatile,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_style_source_volatile");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_source_volatile(
                    Handle,
                    scope.Utf8(sourceId),
                    (byte)(isVolatile ? 1 : 0),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetStyleTransitionOptionsAsync(
        StyleTransitionOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_style_transition_options");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_transition_options(
                    Handle,
                    scope.Value(NativeStyleTransitionOptions(options)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetStyleUrlAsync(
        string url,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_style_url");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_url(
                    Handle,
                    scope.CStringArgument(url),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetTileOptionsAsync(
        MapTileOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_tile_options");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_tile_options(
                    Handle,
                    scope.Value(NativeMapTileOptions(options)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task<CommandCompletion> SetViewportOptionsAsync(
        MapViewportOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_set_viewport_options");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_viewport_options(
                    Handle,
                    scope.Value(NativeMapViewportOptions(options)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public MapSnapshot SnapshotGet()
    {
        using var read = state.Read(this, "mln_map_snapshot_get");
        var outSnapshot = new mln_map_snapshot { size = (uint)sizeof(mln_map_snapshot) };
        Check(NativeMethods.mln_map_snapshot_get(read.Handle, &outSnapshot, Diagnostic));
        return CopyMapSnapshot(outSnapshot);
    }

    public Task<string> StyleUrlAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_style_url");
        return scope.Query<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_style_url(Handle, completion, diagnostic),
            ValueStructs.CopyUtf8View,
            cancellationToken
        );
    }

    public Task<CommandCompletion> UpdateCameraAsync(
        CameraUpdate update,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_update_camera");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_update_camera(
                    Handle,
                    scope.Value(NativeCameraUpdate(update)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public RenderSessionHandle MetalBorrowedTextureAttach(
        MetalBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_metal_borrowed_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_metal_borrowed_texture_attach(
                    Handle,
                    scope.Value(NativeMetalBorrowedTextureDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle MetalOwnedTextureAttach(
        MetalOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_metal_owned_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_metal_owned_texture_attach(
                    Handle,
                    scope.Value(NativeMetalOwnedTextureDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle MetalSurfaceAttach(
        MetalSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_metal_surface_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_metal_surface_attach(
                    Handle,
                    scope.Value(NativeMetalSurfaceDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle OpenglBorrowedTextureAttach(
        OpenglBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_opengl_borrowed_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_opengl_borrowed_texture_attach(
                    Handle,
                    scope.Value(NativeOpenglBorrowedTextureDescriptor(descriptor, scope)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle OpenglOwnedTextureAttach(
        OpenglOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_opengl_owned_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_opengl_owned_texture_attach(
                    Handle,
                    scope.Value(NativeOpenglOwnedTextureDescriptor(descriptor, scope)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle OpenglSurfaceAttach(
        OpenglSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_opengl_surface_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_opengl_surface_attach(
                    Handle,
                    scope.Value(NativeOpenglSurfaceDescriptor(descriptor, scope)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle VulkanBorrowedTextureAttach(
        VulkanBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_vulkan_borrowed_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_vulkan_borrowed_texture_attach(
                    Handle,
                    scope.Value(NativeVulkanBorrowedTextureDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle VulkanOwnedTextureAttach(
        VulkanOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_vulkan_owned_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_vulkan_owned_texture_attach(
                    Handle,
                    scope.Value(NativeVulkanOwnedTextureDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle VulkanSurfaceAttach(
        VulkanSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_vulkan_surface_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_vulkan_surface_attach(
                    Handle,
                    scope.Value(NativeVulkanSurfaceDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle WebgpuBorrowedTextureAttach(
        WebgpuBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_webgpu_borrowed_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_webgpu_borrowed_texture_attach(
                    Handle,
                    scope.Value(NativeWebgpuBorrowedTextureDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle WebgpuOwnedTextureAttach(
        WebgpuOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_webgpu_owned_texture_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_webgpu_owned_texture_attach(
                    Handle,
                    scope.Value(NativeWebgpuOwnedTextureDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }

    public RenderSessionHandle WebgpuSurfaceAttach(
        WebgpuSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this, "mln_webgpu_surface_attach");
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_webgpu_surface_attach(
                    Handle,
                    scope.Value(NativeWebgpuSurfaceDescriptor(descriptor)),
                    scope.Value(NativeRenderSessionAttachOptions(options, scope)),
                    output,
                    completion,
                    diagnostic
                ),
            (handle, attachment) => RenderSessionHandle.Adopt(this, handle, attachment)
        );
    }
}
