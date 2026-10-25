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

    /// <summary>
    /// Completes once the handle is ready, and faults with a
    /// <see cref="MaplibreException" /> when it fails to become ready.
    /// </summary>
    public Task Ready { get; }

    internal MapHandle(RuntimeHandle parent, MlnMap handle, Task completion)
    {
        Ready = completion
            .ContinueWith(
                static (finished, retained) =>
                {
                    GC.KeepAlive(retained);
                    return finished;
                },
                this,
                CancellationToken.None,
                TaskContinuationOptions.ExecuteSynchronously,
                TaskScheduler.Default
            )
            .Unwrap();
        state = new(handle, StartRelease, nameof(MapHandle), Abandon, retainedParent: parent);
    }

    internal static MapHandle Adopt(RuntimeHandle parent, MlnMap handle, Task completion) =>
        NativeHandleState<MlnMap>.Adopt(
            handle,
            () => new MapHandle(parent, handle, completion),
            Abandon
        );

    private static mln_status Abandon(MlnMap live, mln_diagnostic* diagnostic) =>
        NativeMethods.mln_map_dispose(live, diagnostic);

    NativeHandleState<MlnMap> INativeOwner<MlnMap>.State => state;
    internal MlnMap Handle => state.Handle;

    /// <summary>
    /// The native handle value, which runtime events report as their
    /// source. It stays readable after close, so events drained later
    /// still match this handle.
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
    /// Adds a custom geometry source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_custom_geometry_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> AddCustomGeometrySourceAsync(
        string sourceId,
        CustomGeometrySourceOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeCustomGeometrySourceOptions(options, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_custom_geometry_source(
            Handle,
            scope.Utf8(sourceId),
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        CustomMvtVectorSourceOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeCustomMvtVectorSourceOptions(options, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_custom_mvt_vector_source(
            Handle,
            scope.Utf8(sourceId),
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Adds a GeoJSON source with prepared inline data.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_geojson_source_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> AddGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_geojson_source_data(
            Handle,
            scope.Utf8(sourceId),
            scope.Use(data),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Adds a GeoJSON source with URL data.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_geojson_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> AddGeoJsonSourceUrlAsync(
        string sourceId,
        string url,
        GeoJsonSourceOptions? options = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = options is null
            ? default(mln_geojson_source_options)
            : NativeGeoJsonSourceOptions(options, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_geojson_source_url(
            Handle,
            scope.Utf8(sourceId),
            scope.Utf8(url),
            options is null ? null : &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        PremultipliedRgba8Image image
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeImage = NativePremultipliedRgba8Image(image, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_image_source_image(
            Handle,
            scope.Utf8(sourceId),
            scope.Array<mln_lat_lng, LatLng>(
                coordinates,
                static (item, scope) => NativeLatLng(item)
            ),
            checked((nuint)coordinates.Length),
            &nativeImage,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        string url
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_image_source_url(
            Handle,
            scope.Utf8(sourceId),
            scope.Array<mln_lat_lng, LatLng>(
                coordinates,
                static (item, scope) => NativeLatLng(item)
            ),
            checked((nuint)coordinates.Length),
            scope.Utf8(url),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Adds one style layer from a full style-spec layer JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_style_layer_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> AddStyleLayerJsonAsync(string layerJson, string beforeLayerId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_style_layer_json(
            Handle,
            scope.Utf8(layerJson),
            scope.Utf8(beforeLayerId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Adds one style source from a style-spec source JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_add_style_source_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> AddStyleSourceJsonAsync(string sourceId, string sourceJson)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_style_source_json(
            Handle,
            scope.Utf8(sourceId),
            scope.Utf8(sourceJson),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits one atomic relative camera update.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_apply_camera_delta</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> ApplyCameraDeltaAsync(CameraDelta delta)
    {
        using var scope = new NativeCallScope(this);
        var nativeDelta = NativeCameraDelta(delta, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_apply_camera_delta(
            Handle,
            &nativeDelta,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts attachment of a ring of caller-owned Metal textures.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_metal_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    public RenderSessionHandle AttachMetalBorrowedTexture(
        MetalBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeMetalBorrowedTextureDescriptor(descriptor, scope);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_metal_borrowed_texture(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of a session-owned Metal texture ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_metal_owned_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    public RenderSessionHandle AttachMetalOwnedTexture(
        MetalOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeMetalOwnedTextureDescriptor(descriptor);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_metal_owned_texture(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of a Metal surface target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_metal_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    public RenderSessionHandle AttachMetalSurface(
        MetalSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeMetalSurfaceDescriptor(descriptor);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_metal_surface(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of a ring of caller-owned OpenGL textures.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_opengl_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachOpenGlBorrowedTexture(
        OpenGlBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeOpenGlBorrowedTextureDescriptor(descriptor, scope);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_opengl_borrowed_texture(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of a session-owned OpenGL texture ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_opengl_owned_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachOpenGlOwnedTexture(
        OpenGlOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeOpenGlOwnedTextureDescriptor(descriptor, scope);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_opengl_owned_texture(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of an OpenGL surface target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_opengl_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachOpenGlSurface(
        OpenGlSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeOpenGlSurfaceDescriptor(descriptor, scope);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_opengl_surface(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of a ring of caller-owned Vulkan images.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_vulkan_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachVulkanBorrowedTexture(
        VulkanBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeVulkanBorrowedTextureDescriptor(descriptor, scope);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_vulkan_borrowed_texture(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of a session-owned Vulkan texture ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_vulkan_owned_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachVulkanOwnedTexture(
        VulkanOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeVulkanOwnedTextureDescriptor(descriptor);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_vulkan_owned_texture(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Starts attachment of a Vulkan surface target.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_vulkan_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachVulkanSurface(
        VulkanSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeVulkanSurfaceDescriptor(descriptor);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_vulkan_surface(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Begins a command group, which holds this map's render updates until the
    /// group ends.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_begin_command_group</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> BeginCommandGroupAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_begin_command_group(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered query for a camera that fits a GeoJSON geometry.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_camera_for_geometry</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CameraOptions> CameraForGeometryAsync(
        string geometry,
        CameraFitOptions? fitOptions = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeFitOptions = fitOptions is null
            ? default(mln_camera_fit_options)
            : NativeCameraFitOptions(fitOptions);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_camera_for_geometry(
            Handle,
            scope.Utf8(geometry),
            fitOptions is null ? null : &nativeFitOptions,
            NativeCompletion.Start<CameraOptions>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_camera_options, CameraOptions>(
                        nativeResult,
                        CopyCameraOptions
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        CameraFitOptions? fitOptions = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeFitOptions = fitOptions is null
            ? default(mln_camera_fit_options)
            : NativeCameraFitOptions(fitOptions);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_camera_for_lat_lng_bounds(
            Handle,
            NativeLatLngBounds(bounds),
            fitOptions is null ? null : &nativeFitOptions,
            NativeCompletion.Start<CameraOptions>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_camera_options, CameraOptions>(
                        nativeResult,
                        CopyCameraOptions
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        CameraFitOptions? fitOptions = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeFitOptions = fitOptions is null
            ? default(mln_camera_fit_options)
            : NativeCameraFitOptions(fitOptions);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_camera_for_lat_lngs(
            Handle,
            scope.Array<mln_lat_lng, LatLng>(
                coordinates,
                static (item, scope) => NativeLatLng(item)
            ),
            checked((nuint)coordinates.Length),
            fitOptions is null ? null : &nativeFitOptions,
            NativeCompletion.Start<CameraOptions>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_camera_options, CameraOptions>(
                        nativeResult,
                        CopyCameraOptions
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Cancels the camera transitions of the commands whose animation carried
    /// transition_id, and leaves every other transition running.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_cancel_camera_transition</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> CancelCameraTransitionAsync(ulong transitionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_cancel_camera_transition(
            Handle,
            transitionId,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Cancels the camera transitions running when this command commits.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_cancel_transitions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> CancelTransitionsAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_cancel_transitions(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits an ordered debug-log command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_dump_debug_logs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> DumpDebugLogsAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_dump_debug_logs(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Ends the innermost command group that
    /// <c>mln_map_begin_command_group()</c> began.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_end_command_group</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> EndCommandGroupAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_end_command_group(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered camera read.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CameraQueryResult> GetCameraAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_camera(
            Handle,
            NativeCompletion.Start<CameraQueryResult>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_camera_query_result, CameraQueryResult>(
                        nativeResult,
                        CopyCameraQueryResult
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered read of per-feature state from this map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string> GetFeatureStateAsync(FeatureStateSelector selector)
    {
        using var scope = new NativeCallScope(this);
        var nativeSelector = NativeFeatureStateSelector(selector, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_feature_state(
            Handle,
            &nativeSelector,
            NativeCompletion.Start<string>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_buffer_view, string>(
                        nativeResult,
                        ValueStructs.CopyUtf8View
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
    public Task<string> GetGlobalStateAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_global_state(
            Handle,
            NativeCompletion.Start<string>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_buffer_view, string>(
                        nativeResult,
                        ValueStructs.CopyUtf8View
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Copies image source coordinates.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_image_source_coordinates</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<LatLng[]> GetImageSourceCoordinatesAsync(string sourceId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_image_source_coordinates(
            Handle,
            scope.Utf8(sourceId),
            NativeCompletion.Start<LatLng[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_lat_lng, LatLng>(nativeResult, CopyLatLng),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered copy of the last successfully parsed style document.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_loaded_style_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string> GetLoadedStyleJsonAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_loaded_style_json(
            Handle,
            NativeCompletion.Start<string>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_buffer_view, string>(
                        nativeResult,
                        ValueStructs.CopyUtf8View
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Copies the map's latest published snapshot.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_snapshot</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public MapSnapshot GetSnapshot()
    {
        using var call = Enter(this);
        var outSnapshot = new mln_map_snapshot { size = (uint)sizeof(mln_map_snapshot) };
        Check(NativeMethods.mln_map_get_snapshot(Handle, &outSnapshot, Diagnostic));
        return CopyMapSnapshot(outSnapshot);
    }

    /// <summary>
    /// Copies one complete runtime style image.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleImageInfo?> GetStyleImageAsync(string imageId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_image(
            Handle,
            scope.Utf8(imageId),
            NativeCompletion.Start<StyleImageInfo?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Optional<mln_style_image_info, StyleImageInfo>(
                        nativeResult,
                        CopyStyleImageInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Copies the complete metadata of one style layer.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleLayerInfo?> GetStyleLayerAsync(string layerId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_layer(
            Handle,
            scope.Utf8(layerId),
            NativeCompletion.Start<StyleLayerInfo?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.OptionalValue<mln_style_layer_info, StyleLayerInfo>(
                        nativeResult,
                        CopyStyleLayerInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Serializes one style layer as a full style-spec layer JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_layer_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string?> GetStyleLayerJsonAsync(string layerId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_layer_json(
            Handle,
            scope.Utf8(layerId),
            NativeCompletion.Start<string?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Optional<mln_buffer_view, string>(
                        nativeResult,
                        ValueStructs.CopyUtf8View
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Serializes one layer property as a style-spec JSON value.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string?> GetStyleLayerPropertyAsync(string layerId, string propertyName)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_layer_property(
            Handle,
            scope.Utf8(layerId),
            scope.Utf8(propertyName),
            NativeCompletion.Start<string?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Optional<mln_buffer_view, string>(
                        nativeResult,
                        ValueStructs.CopyUtf8View
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Serializes one style light property as a style-spec JSON value.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_light_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string?> GetStyleLightPropertyAsync(string propertyName)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_light_property(
            Handle,
            scope.Utf8(propertyName),
            NativeCompletion.Start<string?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Optional<mln_buffer_view, string>(
                        nativeResult,
                        ValueStructs.CopyUtf8View
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Copies the complete metadata of one style source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleSourceInfo?> GetStyleSourceAsync(string sourceId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_source(
            Handle,
            scope.Utf8(sourceId),
            NativeCompletion.Start<StyleSourceInfo?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Optional<mln_style_source_info, StyleSourceInfo>(
                        nativeResult,
                        CopyStyleSourceInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Reads the style's global transition options.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_transition_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleTransitionOptions> GetStyleTransitionOptionsAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_transition_options(
            Handle,
            NativeCompletion.Start<StyleTransitionOptions>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_style_transition_options, StyleTransitionOptions>(
                        nativeResult,
                        CopyStyleTransitionOptions
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered copy of the last requested style URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string> GetStyleUrlAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_get_style_url(
            Handle,
            NativeCompletion.Start<string>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_buffer_view, string>(
                        nativeResult,
                        ValueStructs.CopyUtf8View
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        LatLngBounds bounds
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_invalidate_custom_geometry_source_region(
            Handle,
            scope.Utf8(sourceId),
            NativeLatLngBounds(bounds),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        CanonicalTileId tileId
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
            Handle,
            scope.Utf8(sourceId),
            NativeCanonicalTileId(tileId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        CanonicalTileId tileId
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_invalidate_custom_mvt_vector_source_tile(
            Handle,
            scope.Utf8(sourceId),
            NativeCanonicalTileId(tileId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered wrapped-bounds query for a copied camera.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lng_bounds_for_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<LatLngBounds> LatLngBoundsForCameraAsync(CameraOptions camera)
    {
        using var scope = new NativeCallScope(this);
        var nativeCamera = NativeCameraOptions(camera);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_lat_lng_bounds_for_camera(
            Handle,
            &nativeCamera,
            NativeCompletion.Start<LatLngBounds>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_lat_lng_bounds, LatLngBounds>(
                        nativeResult,
                        CopyLatLngBounds
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered unwrapped-bounds query for a copied camera.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_lat_lng_bounds_for_camera_unwrapped</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<LatLngBounds> LatLngBoundsForCameraUnwrappedAsync(CameraOptions camera)
    {
        using var scope = new NativeCallScope(this);
        var nativeCamera = NativeCameraOptions(camera);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_lat_lng_bounds_for_camera_unwrapped(
            Handle,
            &nativeCamera,
            NativeCompletion.Start<LatLngBounds>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_lat_lng_bounds, LatLngBounds>(
                        nativeResult,
                        CopyLatLngBounds
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered query of every style layer in style order.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_list_style_layers</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleLayerInfo[]> ListStyleLayersAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_list_style_layers(
            Handle,
            NativeCompletion.Start<StyleLayerInfo[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_style_layer_info, StyleLayerInfo>(
                        nativeResult,
                        CopyStyleLayerInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Lists every style source in style order.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_list_style_sources</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleSourceInfo[]> ListStyleSourcesAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_list_style_sources(
            Handle,
            NativeCompletion.Start<StyleSourceInfo[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_style_source_info, StyleSourceInfo>(
                        nativeResult,
                        CopyStyleSourceInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Moves one style layer before another layer or to the top.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_move_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> MoveStyleLayerAsync(string layerId, string beforeLayerId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_move_style_layer(
            Handle,
            scope.Utf8(layerId),
            scope.Utf8(beforeLayerId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Releases a map asynchronously.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task CloseAsync()
    {
        state.Close(idempotent: false);
        return teardown;
    }

    public ValueTask DisposeAsync() => state.Close() ? new(teardown) : default;

    private mln_status StartRelease(MlnMap handle, mln_diagnostic* _)
    {
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_release(
            handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        teardown = nativePending.Submitted(nativeStatus);
        return mln_status.MLN_STATUS_OK;
    }

    /// <summary>
    /// Removes per-feature state from this map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> RemoveFeatureStateAsync(FeatureStateSelector selector)
    {
        using var scope = new NativeCallScope(this);
        var nativeSelector = NativeFeatureStateSelector(selector, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_remove_feature_state(
            Handle,
            &nativeSelector,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Removes one runtime style image by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_style_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> RemoveStyleImageAsync(string imageId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_remove_style_image(
            Handle,
            scope.Utf8(imageId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Removes one style layer by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> RemoveStyleLayerAsync(string layerId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_remove_style_layer(
            Handle,
            scope.Utf8(layerId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Removes one style source by ID.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_remove_style_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> RemoveStyleSourceAsync(string sourceId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_remove_style_source(
            Handle,
            scope.Utf8(sourceId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Requests a repaint for a continuous map.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_request_repaint</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> RequestRepaintAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_request_repaint(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Requests one still image for a static or tile map, which renders itself.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_request_still_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task RequestStillImageAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_request_still_image(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits the sole post-creation logical extent update.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_resize</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> ResizeAsync(LogicalExtent extent)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_resize(
            Handle,
            NativeLogicalExtent(extent),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a copied camera-constraint command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_bounds</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetBoundsAsync(BoundOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeBoundOptions(options);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_bounds(
            Handle,
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        string data
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_custom_geometry_source_tile_data(
            Handle,
            scope.Utf8(sourceId),
            NativeCanonicalTileId(tileId),
            scope.Utf8(data),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Reports a custom geometry source error for one canonical tile.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_custom_geometry_source_tile_error</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetCustomGeometrySourceTileErrorAsync(
        string sourceId,
        CanonicalTileId tileId,
        string message
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_custom_geometry_source_tile_error(
            Handle,
            scope.Utf8(sourceId),
            NativeCanonicalTileId(tileId),
            scope.Utf8(message),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        ReadOnlySpan<byte> data
    )
    {
        fixed (byte* pinnedData = data)
        {
            using var scope = new NativeCallScope(this);
            mln_completion nativeCompletion;
            var nativeStatus = NativeMethods.mln_map_set_custom_mvt_vector_source_tile_data(
                Handle,
                scope.Utf8(sourceId),
                NativeCanonicalTileId(tileId),
                new mln_buffer_view { data = pinnedData, size = (nuint)data.Length },
                NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
                Diagnostic
            );
            return scope.Accepted(nativePending.Submitted(nativeStatus));
        }
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
        string message
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_custom_mvt_vector_source_tile_error(
            Handle,
            scope.Utf8(sourceId),
            NativeCanonicalTileId(tileId),
            scope.Utf8(message),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a debug-overlay command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_debug_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetDebugOptionsAsync(MapDebugFlag options)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_debug_options(
            Handle,
            (uint)options,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Selects which map-originated event types this map queues.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_event_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetEventMaskAsync(RuntimeEventFlag mask)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_event_mask(
            Handle,
            (ulong)mask,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a copied per-feature-state command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetFeatureStateAsync(FeatureStateSelector selector, string state)
    {
        using var scope = new NativeCallScope(this);
        var nativeSelector = NativeFeatureStateSelector(selector, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_feature_state(
            Handle,
            &nativeSelector,
            scope.Utf8(state),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a copied free-camera command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_free_camera_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetFreeCameraOptionsAsync(FreeCameraOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeFreeCameraOptions(options);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_free_camera_options(
            Handle,
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Updates one GeoJSON source with prepared inline data.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_geojson_source_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_geojson_source_data(
            Handle,
            scope.Utf8(sourceId),
            scope.Use(data),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Overrides one GeoJSON source's synchronous tiling at runtime.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_geojson_source_synchronous_tiling</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceSynchronousTilingAsync(
        string sourceId,
        bool enabled
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
            Handle,
            scope.Utf8(sourceId),
            (byte)(enabled ? 1 : 0),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Updates one GeoJSON source to load data from a URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_geojson_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceUrlAsync(string sourceId, string url)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_geojson_source_url(
            Handle,
            scope.Utf8(sourceId),
            scope.Utf8(url),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a global-state JSON value. JSON null restores the style default.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_global_state_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetGlobalStatePropertyAsync(string propertyName, string value)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_global_state_property(
            Handle,
            scope.Utf8(propertyName),
            scope.Utf8(value),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        LatLng[] coordinates
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_image_source_coordinates(
            Handle,
            scope.Utf8(sourceId),
            scope.Array<mln_lat_lng, LatLng>(
                coordinates,
                static (item, scope) => NativeLatLng(item)
            ),
            checked((nuint)coordinates.Length),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        PremultipliedRgba8Image image
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeImage = NativePremultipliedRgba8Image(image, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_image_source_image(
            Handle,
            scope.Utf8(sourceId),
            &nativeImage,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Updates an image source to load its image from a URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_image_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetImageSourceUrlAsync(string sourceId, string url)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_image_source_url(
            Handle,
            scope.Utf8(sourceId),
            scope.Utf8(url),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits copied axonometric rendering option fields.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_projection_mode</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetProjectionModeAsync(ProjectionMode mode)
    {
        using var scope = new NativeCallScope(this);
        var nativeMode = NativeProjectionMode(mode);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_projection_mode(
            Handle,
            &nativeMode,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a rendering-stats visibility command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_rendering_stats_view_enabled</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetRenderingStatsViewEnabledAsync(bool enabled)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_rendering_stats_view_enabled(
            Handle,
            (byte)(enabled ? 1 : 0),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
        StyleImageOptions? options = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeImage = NativePremultipliedRgba8Image(image, scope);
        var nativeOptions = options is null
            ? default(mln_style_image_options)
            : NativeStyleImageOptions(options, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_image(
            Handle,
            scope.Utf8(imageId),
            &nativeImage,
            options is null ? null : &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Queues an inline style JSON command. A committed command means the map
    /// applied the JSON, not that the style loaded: style loading events report
    /// the resource and parse results that follow.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleJsonAsync(string json)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_json(
            Handle,
            scope.Utf8(json),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets one layer property using its MapLibre style-spec property name.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleLayerPropertyAsync(
        string layerId,
        string propertyName,
        string value
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_layer_property(
            Handle,
            scope.Utf8(layerId),
            scope.Utf8(propertyName),
            scope.Utf8(value),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets the style light from a style-spec light JSON object.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_light_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleLightJsonAsync(string lightJson)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_light_json(
            Handle,
            scope.Utf8(lightJson),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets one style light property using its MapLibre style-spec property
    /// name.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_light_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleLightPropertyAsync(string propertyName, string value)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_light_property(
            Handle,
            scope.Utf8(propertyName),
            scope.Utf8(value),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets whether one style source stores fetched tiles in the persistent
    /// cache.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_source_volatile</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleSourceVolatileAsync(string sourceId, bool isVolatile)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_source_volatile(
            Handle,
            scope.Utf8(sourceId),
            (byte)(isVolatile ? 1 : 0),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets the style's global transition options.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_transition_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleTransitionOptionsAsync(StyleTransitionOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeStyleTransitionOptions(options);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_transition_options(
            Handle,
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Queues a style URL command. A committed command means the map accepted
    /// the URL, not that the style loaded: style loading events report the
    /// network, decode, and parse results that follow.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleUrlAsync(string url)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_style_url(
            Handle,
            scope.Utf8(url),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a copied tile-options command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_tile_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetTileOptionsAsync(MapTileOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeMapTileOptions(options);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_tile_options(
            Handle,
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits a copied viewport-options command.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_viewport_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetViewportOptionsAsync(MapViewportOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeMapViewportOptions(options);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_viewport_options(
            Handle,
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits one atomic camera update.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_update_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> UpdateCameraAsync(CameraUpdate update)
    {
        using var scope = new NativeCallScope(this);
        var nativeUpdate = NativeCameraUpdate(update, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_update_camera(
            Handle,
            &nativeUpdate,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Submits each command without a completion. Native logs a failure as
    /// a warning, and the caller sees none.
    /// </summary>
    public DiscardingCommands Discarding => new(this);

    /// <summary>
    /// The commands of a handle, each submitted without a completion.
    /// Native logs a failure as a warning, and the caller sees none.
    /// </summary>
    public readonly struct DiscardingCommands
    {
        private readonly MapHandle owner;

        internal DiscardingCommands(MapHandle owner)
        {
            this.owner = owner;
        }

        public void AddCustomGeometrySource(string sourceId, CustomGeometrySourceOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeCustomGeometrySourceOptions(options, scope);
            Check(
                NativeMethods.mln_map_add_custom_geometry_source(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void AddCustomMvtVectorSource(string sourceId, CustomMvtVectorSourceOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeCustomMvtVectorSourceOptions(options, scope);
            Check(
                NativeMethods.mln_map_add_custom_mvt_vector_source(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void AddGeoJsonSourceData(string sourceId, GeoJsonSourceDataHandle data)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_add_geojson_source_data(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Use(data),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void AddGeoJsonSourceUrl(
            string sourceId,
            string url,
            GeoJsonSourceOptions? options = null
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = options is null
                ? default(mln_geojson_source_options)
                : NativeGeoJsonSourceOptions(options, scope);
            Check(
                NativeMethods.mln_map_add_geojson_source_url(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    options is null ? null : &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void AddImageSourceImage(
            string sourceId,
            LatLng[] coordinates,
            PremultipliedRgba8Image image
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeImage = NativePremultipliedRgba8Image(image, scope);
            Check(
                NativeMethods.mln_map_add_image_source_image(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_lat_lng, LatLng>(
                        coordinates,
                        static (item, scope) => NativeLatLng(item)
                    ),
                    checked((nuint)coordinates.Length),
                    &nativeImage,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void AddImageSourceUrl(string sourceId, LatLng[] coordinates, string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_add_image_source_url(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_lat_lng, LatLng>(
                        coordinates,
                        static (item, scope) => NativeLatLng(item)
                    ),
                    checked((nuint)coordinates.Length),
                    scope.Utf8(url),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void AddStyleLayerJson(string layerJson, string beforeLayerId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_add_style_layer_json(
                    receiverHandle,
                    scope.Utf8(layerJson),
                    scope.Utf8(beforeLayerId),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void AddStyleSourceJson(string sourceId, string sourceJson)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_add_style_source_json(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Utf8(sourceJson),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void ApplyCameraDelta(CameraDelta delta)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeDelta = NativeCameraDelta(delta, scope);
            Check(
                NativeMethods.mln_map_apply_camera_delta(
                    receiverHandle,
                    &nativeDelta,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void BeginCommandGroup()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(NativeMethods.mln_map_begin_command_group(receiverHandle, null, Diagnostic));
            scope.Accept();
        }

        public void CancelCameraTransition(ulong transitionId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_cancel_camera_transition(
                    receiverHandle,
                    transitionId,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void CancelTransitions()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(NativeMethods.mln_map_cancel_transitions(receiverHandle, null, Diagnostic));
            scope.Accept();
        }

        public void DumpDebugLogs()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(NativeMethods.mln_map_dump_debug_logs(receiverHandle, null, Diagnostic));
            scope.Accept();
        }

        public void EndCommandGroup()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(NativeMethods.mln_map_end_command_group(receiverHandle, null, Diagnostic));
            scope.Accept();
        }

        public void InvalidateCustomGeometrySourceRegion(string sourceId, LatLngBounds bounds)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    NativeLatLngBounds(bounds),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void InvalidateCustomGeometrySourceTile(string sourceId, CanonicalTileId tileId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void InvalidateCustomMvtVectorSourceTile(string sourceId, CanonicalTileId tileId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_invalidate_custom_mvt_vector_source_tile(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void MoveStyleLayer(string layerId, string beforeLayerId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_move_style_layer(
                    receiverHandle,
                    scope.Utf8(layerId),
                    scope.Utf8(beforeLayerId),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void RemoveFeatureState(FeatureStateSelector selector)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeSelector = NativeFeatureStateSelector(selector, scope);
            Check(
                NativeMethods.mln_map_remove_feature_state(
                    receiverHandle,
                    &nativeSelector,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void RemoveStyleImage(string imageId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_remove_style_image(
                    receiverHandle,
                    scope.Utf8(imageId),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void RemoveStyleLayer(string layerId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_remove_style_layer(
                    receiverHandle,
                    scope.Utf8(layerId),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void RemoveStyleSource(string sourceId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_remove_style_source(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void RequestRepaint()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(NativeMethods.mln_map_request_repaint(receiverHandle, null, Diagnostic));
            scope.Accept();
        }

        public void Resize(LogicalExtent extent)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_resize(
                    receiverHandle,
                    NativeLogicalExtent(extent),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetBounds(BoundOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeBoundOptions(options);
            Check(
                NativeMethods.mln_map_set_bounds(receiverHandle, &nativeOptions, null, Diagnostic)
            );
            scope.Accept();
        }

        public void SetCustomGeometrySourceTileData(
            string sourceId,
            CanonicalTileId tileId,
            string data
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    scope.Utf8(data),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetCustomGeometrySourceTileError(
            string sourceId,
            CanonicalTileId tileId,
            string message
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_custom_geometry_source_tile_error(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    scope.Utf8(message),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetCustomMvtVectorSourceTileData(
            string sourceId,
            CanonicalTileId tileId,
            ReadOnlySpan<byte> data
        )
        {
            fixed (byte* pinnedData = data)
            {
                var receiverHandle = owner.Handle;
                using var scope = new NativeCallScope(owner);
                Check(
                    NativeMethods.mln_map_set_custom_mvt_vector_source_tile_data(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeCanonicalTileId(tileId),
                        new mln_buffer_view { data = pinnedData, size = (nuint)data.Length },
                        null,
                        Diagnostic
                    )
                );
                scope.Accept();
            }
        }

        public void SetCustomMvtVectorSourceTileError(
            string sourceId,
            CanonicalTileId tileId,
            string message
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_custom_mvt_vector_source_tile_error(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    NativeCanonicalTileId(tileId),
                    scope.Utf8(message),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetDebugOptions(MapDebugFlag options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_debug_options(
                    receiverHandle,
                    (uint)options,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetEventMask(RuntimeEventFlag mask)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_event_mask(receiverHandle, (ulong)mask, null, Diagnostic)
            );
            scope.Accept();
        }

        public void SetFeatureState(FeatureStateSelector selector, string state)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeSelector = NativeFeatureStateSelector(selector, scope);
            Check(
                NativeMethods.mln_map_set_feature_state(
                    receiverHandle,
                    &nativeSelector,
                    scope.Utf8(state),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetFreeCameraOptions(FreeCameraOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeFreeCameraOptions(options);
            Check(
                NativeMethods.mln_map_set_free_camera_options(
                    receiverHandle,
                    &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetGeoJsonSourceData(string sourceId, GeoJsonSourceDataHandle data)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_geojson_source_data(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Use(data),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetGeoJsonSourceSynchronousTiling(string sourceId, bool enabled)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    (byte)(enabled ? 1 : 0),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetGeoJsonSourceUrl(string sourceId, string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_geojson_source_url(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetGlobalStateProperty(string propertyName, string value)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_global_state_property(
                    receiverHandle,
                    scope.Utf8(propertyName),
                    scope.Utf8(value),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetImageSourceCoordinates(string sourceId, LatLng[] coordinates)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_image_source_coordinates(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Array<mln_lat_lng, LatLng>(
                        coordinates,
                        static (item, scope) => NativeLatLng(item)
                    ),
                    checked((nuint)coordinates.Length),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetImageSourceImage(string sourceId, PremultipliedRgba8Image image)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeImage = NativePremultipliedRgba8Image(image, scope);
            Check(
                NativeMethods.mln_map_set_image_source_image(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    &nativeImage,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetImageSourceUrl(string sourceId, string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_image_source_url(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Utf8(url),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetProjectionMode(ProjectionMode mode)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeMode = NativeProjectionMode(mode);
            Check(
                NativeMethods.mln_map_set_projection_mode(
                    receiverHandle,
                    &nativeMode,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetRenderingStatsViewEnabled(bool enabled)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_rendering_stats_view_enabled(
                    receiverHandle,
                    (byte)(enabled ? 1 : 0),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleImage(
            string imageId,
            PremultipliedRgba8Image image,
            StyleImageOptions? options = null
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeImage = NativePremultipliedRgba8Image(image, scope);
            var nativeOptions = options is null
                ? default(mln_style_image_options)
                : NativeStyleImageOptions(options, scope);
            Check(
                NativeMethods.mln_map_set_style_image(
                    receiverHandle,
                    scope.Utf8(imageId),
                    &nativeImage,
                    options is null ? null : &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleJson(string json)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_style_json(
                    receiverHandle,
                    scope.Utf8(json),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleLayerProperty(string layerId, string propertyName, string value)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_style_layer_property(
                    receiverHandle,
                    scope.Utf8(layerId),
                    scope.Utf8(propertyName),
                    scope.Utf8(value),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleLightJson(string lightJson)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_style_light_json(
                    receiverHandle,
                    scope.Utf8(lightJson),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleLightProperty(string propertyName, string value)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_style_light_property(
                    receiverHandle,
                    scope.Utf8(propertyName),
                    scope.Utf8(value),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleSourceVolatile(string sourceId, bool isVolatile)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_style_source_volatile(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    (byte)(isVolatile ? 1 : 0),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleTransitionOptions(StyleTransitionOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeStyleTransitionOptions(options);
            Check(
                NativeMethods.mln_map_set_style_transition_options(
                    receiverHandle,
                    &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetStyleUrl(string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_style_url(
                    receiverHandle,
                    scope.Utf8(url),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetTileOptions(MapTileOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeMapTileOptions(options);
            Check(
                NativeMethods.mln_map_set_tile_options(
                    receiverHandle,
                    &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetViewportOptions(MapViewportOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeMapViewportOptions(options);
            Check(
                NativeMethods.mln_map_set_viewport_options(
                    receiverHandle,
                    &nativeOptions,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void UpdateCamera(CameraUpdate update)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeUpdate = NativeCameraUpdate(update, scope);
            Check(
                NativeMethods.mln_map_update_camera(receiverHandle, &nativeUpdate, null, Diagnostic)
            );
            scope.Accept();
        }
    }
}
