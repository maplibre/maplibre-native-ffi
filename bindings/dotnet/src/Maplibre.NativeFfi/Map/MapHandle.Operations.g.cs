// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A map, which holds map state independent of any render target.
/// </summary>
/// <remarks>
/// See <c>mln_map</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
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

    /// <summary>
    /// Adds a color-relief layer for a raster DEM source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_color_relief_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a custom geometry source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_custom_geometry_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a custom MVT vector source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_custom_mvt_vector_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a GeoJSON source with prepared inline data.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_geojson_source_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> AddGeojsonSourceDataAsync(
        string sourceId,
        GeojsonSourceDataHandle data,
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

    /// <summary>
    /// Adds a GeoJSON source with URL data.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_geojson_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a hillshade layer for a raster DEM source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_hillshade_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds an image source with inline image pixels.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_image_source_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds an image source that loads its image from a URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_image_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a source-free location indicator layer.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_location_indicator_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a raster DEM source with inline tile URLs.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_raster_dem_source_tiles</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a raster DEM source with a TileJSON URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_raster_dem_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a raster source with inline tile URLs.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_raster_source_tiles</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a raster source with a TileJSON URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_raster_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds one style layer from a full style-spec layer JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_style_layer_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds one style source from a style-spec source JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_style_source_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a vector source with inline tile URLs.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_vector_source_tiles</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Adds a vector source with a TileJSON URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_vector_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits one copied relative camera update.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_apply_camera_delta</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered query for a camera that fits a GeoJSON geometry.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_camera_for_geometry</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered query for a camera that fits geographic bounds.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_camera_for_lat_lng_bounds</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered query for a camera that fits geographic coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_camera_for_lat_lngs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered camera read.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_camera_query</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies the camera from the latest immutable map snapshot.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_camera_snapshot_get</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Cancels the camera transitions running when this command commits.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_cancel_transitions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one layer's source ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_copy_layer_source_id</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one layer's source-layer ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_copy_layer_source_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one runtime style image as tightly packed premultiplied RGBA8
    /// pixels.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_copy_style_image_premultiplied_rgba8</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one runtime style image's stretchable intervals.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_copy_style_image_stretches</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one style source attribution string.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_copy_style_source_attribution</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one style source URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_copy_style_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits an ordered debug-log command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_dump_debug_logs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> DumpDebugLogsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_dump_debug_logs");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_dump_debug_logs(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered read of per-feature state from this map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Queries the global-state JSON object, including style defaults.
    /// Completion borrows one <c>mln_buffer_view</c> for the duration of the
    /// callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_global_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies image source coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_image_source_coordinates</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Serializes one layer filter as a style-spec JSON value.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_layer_filter</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Serializes one layer property as a style-spec JSON value.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one complete runtime style image.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_image_info</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies complete metadata for one style layer.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_layer_info</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Serializes one style layer as a full style-spec layer JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_layer_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Serializes one style light property as a style-spec JSON value.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_light_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies complete metadata for one style source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_source_info</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies one style source's inline TileJSON tile URLs.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_source_tile_urls</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Reads the style's global transition options.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_transition_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Invalidates custom geometry source data inside one geographic region.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_invalidate_custom_geometry_source_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Invalidates custom geometry source data for one canonical tile.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_invalidate_custom_geometry_source_tile</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Invalidates custom MVT vector source data for one canonical tile.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_invalidate_custom_mvt_vector_source_tile</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered wrapped-bounds query for a copied camera.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lng_bounds_for_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered unwrapped-bounds query for a copied camera.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lng_bounds_for_camera_unwrapped</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered conversion from a screen point to a geographic
    /// coordinate.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lng_for_pixel</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered conversion from a screen point to an unwrapped
    /// geographic coordinate.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lng_for_pixel_unwrapped</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered conversion of copied screen points to coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lngs_for_pixels</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered conversion of copied screen points to unwrapped
    /// coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lngs_for_pixels_unwrapped</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies style layer IDs in style order.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_list_style_layer_ids</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string[]> ListStyleLayerIdsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_list_style_layer_ids");
        return scope.QueryArray<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_layer_ids(Handle, completion, diagnostic),
            ValueStructs.CopyUtf8View,
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered query of every style layer in style order.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_list_style_layers</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleLayerEntry[]> ListStyleLayersAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_list_style_layers");
        return scope.QueryArray<mln_style_layer_entry, StyleLayerEntry>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_layers(Handle, completion, diagnostic),
            CopyStyleLayerEntry,
            cancellationToken
        );
    }

    /// <summary>
    /// Copies style source IDs in style order.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_list_style_source_ids</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string[]> ListStyleSourceIdsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_list_style_source_ids");
        return scope.QueryArray<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_source_ids(Handle, completion, diagnostic),
            ValueStructs.CopyUtf8View,
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered copy of the last successfully parsed style document.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_loaded_style_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered query of meters per logical pixel at a latitude and
    /// the current map zoom. The completion borrows one double.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_meters_per_pixel_at_latitude</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Moves one style layer before another layer or to the top.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_move_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered conversion from a geographic coordinate to a screen
    /// point.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_pixel_for_lat_lng</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered conversion of copied coordinates to screen points.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_pixels_for_lat_lngs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts creation of a standalone projection from the map's ordered
    /// transform state.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_projection_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public Task<MapProjectionHandle> ProjectionCreateAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_map_projection_create");
        return scope.Query<MlnMapProjection, MapProjectionHandle>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_projection_create(Handle, completion, diagnostic),
            handle => MapProjectionHandle.Adopt(handle),
            cancellationToken
        );
    }

    /// <summary>
    /// Releases a map after synchronous state preflight.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Removes per-feature state from this map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Removes one runtime style image by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_style_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Removes one style layer by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Removes one style source by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_style_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Requests a repaint for a continuous map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_request_repaint</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Requests one still image for a static or tile map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_request_still_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task RequestStillImageAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_map_request_still_image");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_map_request_still_image(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Submits the sole post-creation logical extent update.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_resize</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a copied camera-constraint command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_bounds</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets custom geometry source data for one canonical tile.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_custom_geometry_source_tile_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets custom MVT vector source data for one canonical tile.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_custom_mvt_vector_source_tile_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Reports a custom MVT vector source error for one canonical tile.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_custom_mvt_vector_source_tile_error</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a debug-overlay command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_debug_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Selects which map-originated event types this map queues.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_event_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a copied per-feature-state command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a copied free-camera command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_free_camera_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Updates one GeoJSON source with prepared inline data.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_geojson_source_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetGeojsonSourceDataAsync(
        string sourceId,
        GeojsonSourceDataHandle data,
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

    /// <summary>
    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_geojson_source_synchronous_tiling</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Updates one GeoJSON source to load data from a URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_geojson_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a global-state JSON value. JSON null restores the style default.
    /// Input is copied before return.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_global_state_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Updates image source coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_image_source_coordinates</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Updates an image source with inline image pixels.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_image_source_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Updates an image source to load its image from a URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_image_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets or clears one layer filter.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_layer_filter</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets the highest zoom at which one layer draws.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_layer_max_zoom</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets the lowest zoom at which one layer draws.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_layer_min_zoom</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets one layer property using its MapLibre style-spec property name.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets one layer's source ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_layer_source_id</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets one layer's source-layer ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_layer_source_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets whether one layer draws.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_layer_visibility</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets a location indicator layer accuracy radius in meters.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_location_indicator_accuracy_radius</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets a location indicator layer bearing in degrees.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_location_indicator_bearing</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets one location indicator image-name property.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_location_indicator_image_name</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets a location indicator layer location.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_location_indicator_location</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits copied axonometric rendering option fields.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_projection_mode</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a rendering-stats visibility command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_rendering_stats_view_enabled</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets one runtime style image.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Queues an inline style JSON command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets the style light from a style-spec light JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_light_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets one style light property using its MapLibre style-spec property
    /// name.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_light_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets whether one style source stores fetched tiles in the persistent
    /// cache.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_source_volatile</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Sets the style's global transition options.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_transition_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Queues a style URL command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a copied tile-options command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_tile_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits a copied viewport-options command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_viewport_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies the latest immutable state published by the map worker.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_snapshot_get</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public MapSnapshot SnapshotGet()
    {
        using var read = state.Read(this, "mln_map_snapshot_get");
        var outSnapshot = new mln_map_snapshot { size = (uint)sizeof(mln_map_snapshot) };
        Check(NativeMethods.mln_map_snapshot_get(read.Handle, &outSnapshot, Diagnostic));
        return CopyMapSnapshot(outSnapshot);
    }

    /// <summary>
    /// Starts an ordered copy of the last requested style URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_style_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Submits one atomic camera update.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_update_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a caller-owned Metal texture target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_metal_borrowed_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a session-owned Metal texture ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_metal_owned_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a Metal surface target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_metal_surface_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a caller-owned OpenGL texture target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_opengl_borrowed_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a session-owned OpenGL texture ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_opengl_owned_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of an OpenGL surface target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_opengl_surface_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a caller-owned Vulkan texture target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_vulkan_borrowed_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a session-owned Vulkan texture ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_vulkan_owned_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a Vulkan surface target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_vulkan_surface_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a caller-owned WebGPU texture target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_webgpu_borrowed_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a session-owned WebGPU texture ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_webgpu_owned_texture_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts attachment of a WebGPU surface target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_webgpu_surface_attach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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
