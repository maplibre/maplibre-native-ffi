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

    // Runtime events report their source by this identity.
    public ulong Id => state.Handle.Value;

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
        CustomGeometrySourceOptions options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
    /// Submits one atomic relative camera update.
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
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_apply_camera_delta(
                    Handle,
                    scope.Value(NativeCameraDelta(delta, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts attachment of a ring of caller-owned Metal textures.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_metal_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachMetalBorrowedTexture(
        MetalBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_metal_borrowed_texture(
                    Handle,
                    scope.Value(NativeMetalBorrowedTextureDescriptor(descriptor, scope)),
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
    /// See <c>mln_map_attach_metal_owned_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachMetalOwnedTexture(
        MetalOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_metal_owned_texture(
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
    /// See <c>mln_map_attach_metal_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachMetalSurface(
        MetalSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_metal_surface(
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
    /// Starts attachment of a ring of caller-owned OpenGL textures.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_opengl_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachOpenglBorrowedTexture(
        OpenglBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_opengl_borrowed_texture(
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
    /// See <c>mln_map_attach_opengl_owned_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachOpenglOwnedTexture(
        OpenglOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_opengl_owned_texture(
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
    /// See <c>mln_map_attach_opengl_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachOpenglSurface(
        OpenglSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_opengl_surface(
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
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_vulkan_borrowed_texture(
                    Handle,
                    scope.Value(NativeVulkanBorrowedTextureDescriptor(descriptor, scope)),
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
    /// See <c>mln_map_attach_vulkan_owned_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachVulkanOwnedTexture(
        VulkanOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_vulkan_owned_texture(
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
    /// See <c>mln_map_attach_vulkan_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachVulkanSurface(
        VulkanSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_vulkan_surface(
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
    /// Starts attachment of a ring of caller-owned WebGPU textures.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_attach_webgpu_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachWebgpuBorrowedTexture(
        WebgpuBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_webgpu_borrowed_texture(
                    Handle,
                    scope.Value(NativeWebgpuBorrowedTextureDescriptor(descriptor, scope)),
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
    /// See <c>mln_map_attach_webgpu_owned_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachWebgpuOwnedTexture(
        WebgpuOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_webgpu_owned_texture(
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
    /// See <c>mln_map_attach_webgpu_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public RenderSessionHandle AttachWebgpuSurface(
        WebgpuSurfaceDescriptor descriptor,
        RenderSessionAttachOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Attach<MlnRenderSession, RenderSessionHandle>(
            (output, completion, diagnostic) =>
                NativeMethods.mln_map_attach_webgpu_surface(
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

    /// <summary>
    /// Begins a command group, which holds this map's render updates until the
    /// group ends.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_begin_command_group</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> BeginCommandGroupAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_begin_command_group(Handle, completion, diagnostic),
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
    /// Cancels the camera transitions of the commands whose animation carried
    /// transition_id, and leaves every other transition running.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_cancel_camera_transition</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> CancelCameraTransitionAsync(
        ulong transitionId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_cancel_camera_transition(
                    Handle,
                    transitionId,
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
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
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_cancel_transitions(Handle, completion, diagnostic),
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
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_dump_debug_logs(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Ends the innermost command group that
    /// <c>mln_map_begin_command_group()</c> began.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_end_command_group</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> EndCommandGroupAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_end_command_group(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered camera read.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CameraQueryResult> GetCameraAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this);
        return scope.Query<mln_camera_query_result, CameraQueryResult>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_camera(Handle, completion, diagnostic),
            CopyCameraQueryResult,
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
    public Task<LatLng[]> GetImageSourceCoordinatesAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryArray<mln_lat_lng, LatLng>(
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
    /// Starts an ordered copy of the last successfully parsed style document.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_loaded_style_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<byte[]> GetLoadedStyleJsonAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this);
        return scope.Query<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_loaded_style_json(Handle, completion, diagnostic),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
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
    public Task<StyleImageInfo?> GetStyleImageAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryOptional<mln_style_image_info, StyleImageInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_image(
                    Handle,
                    scope.Utf8(imageId),
                    completion,
                    diagnostic
                ),
            CopyStyleImageInfo,
            cancellationToken
        );
    }

    /// <summary>
    /// Copies the complete metadata of one style layer.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleLayerInfo?> GetStyleLayerAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryOptionalValue<mln_style_layer_info, StyleLayerInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_layer(
                    Handle,
                    scope.Utf8(layerId),
                    completion,
                    diagnostic
                ),
            CopyStyleLayerInfo,
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
        using var scope = new NativeCallScope(this);
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
    /// Serializes one layer property as a style-spec JSON value.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<byte[]?> GetStyleLayerPropertyAsync(
        string layerId,
        string propertyName,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryOptional<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_layer_property(
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
        using var scope = new NativeCallScope(this);
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
    /// Copies the complete metadata of one style source.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleSourceInfo?> GetStyleSourceAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryOptional<mln_style_source_info, StyleSourceInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_source(
                    Handle,
                    scope.Utf8(sourceId),
                    completion,
                    diagnostic
                ),
            CopyStyleSourceInfo,
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
        using var scope = new NativeCallScope(this);
        return scope.Query<mln_style_transition_options, StyleTransitionOptions>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_transition_options(Handle, completion, diagnostic),
            CopyStyleTransitionOptions,
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered copy of the last requested style URL.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_get_style_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string> GetStyleUrlAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this);
        return scope.Query<mln_buffer_view, string>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_get_style_url(Handle, completion, diagnostic),
            ValueStructs.CopyUtf8View,
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
    /// Starts an ordered query of every style layer in style order.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_list_style_layers</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleLayerInfo[]> ListStyleLayersAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryArray<mln_style_layer_info, StyleLayerInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_layers(Handle, completion, diagnostic),
            CopyStyleLayerInfo,
            cancellationToken
        );
    }

    /// <summary>
    /// Lists every style source in style order.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_list_style_sources</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<StyleSourceInfo[]> ListStyleSourcesAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryArray<mln_style_source_info, StyleSourceInfo>(
            (completion, diagnostic) =>
                NativeMethods.mln_map_list_style_sources(Handle, completion, diagnostic),
            CopyStyleSourceInfo,
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
    /// Sets one layer property using its MapLibre style-spec property name.
    /// </summary>
    /// <remarks>
    /// See <c>mln_map_set_style_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> SetStyleLayerPropertyAsync(
        string layerId,
        string propertyName,
        byte[] value,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_layer_property(
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_set_style_url(
                    Handle,
                    scope.Utf8(url),
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_map_update_camera(
                    Handle,
                    scope.Value(NativeCameraUpdate(update, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public DiscardingCommands Discarding => new(this);

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
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_custom_geometry_source(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Value(NativeCustomGeometrySourceOptions(options, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void AddCustomMvtVectorSource(string sourceId, CustomMvtVectorSourceOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_custom_mvt_vector_source(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Value(NativeCustomMvtVectorSourceOptions(options, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void AddGeojsonSourceData(string sourceId, GeojsonSourceDataHandle data)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_geojson_source_data(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Use(data),
                        completion,
                        diagnostic
                    )
            );
        }

        public void AddGeojsonSourceUrl(string sourceId, string url, GeojsonSourceOptions? options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_geojson_source_url(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Utf8(url),
                        options is null
                            ? null
                            : scope.Value(NativeGeojsonSourceOptions(options, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void AddImageSourceImage(
            string sourceId,
            LatLng[] coordinates,
            PremultipliedRgba8Image image
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_image_source_image(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                        checked((nuint)coordinates.Length),
                        scope.Value(NativePremultipliedRgba8Image(image, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void AddImageSourceUrl(string sourceId, LatLng[] coordinates, string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_image_source_url(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                        checked((nuint)coordinates.Length),
                        scope.Utf8(url),
                        completion,
                        diagnostic
                    )
            );
        }

        public void AddStyleLayerJson(byte[] layerJson, string beforeLayerId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_style_layer_json(
                        receiverHandle,
                        scope.Buffer(layerJson),
                        scope.Utf8(beforeLayerId),
                        completion,
                        diagnostic
                    )
            );
        }

        public void AddStyleSourceJson(string sourceId, byte[] sourceJson)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_add_style_source_json(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Buffer(sourceJson),
                        completion,
                        diagnostic
                    )
            );
        }

        public void ApplyCameraDelta(CameraDelta delta)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_apply_camera_delta(
                        receiverHandle,
                        scope.Value(NativeCameraDelta(delta, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void BeginCommandGroup()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_begin_command_group(
                        receiverHandle,
                        completion,
                        diagnostic
                    )
            );
        }

        public void CancelCameraTransition(ulong transitionId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_cancel_camera_transition(
                        receiverHandle,
                        transitionId,
                        completion,
                        diagnostic
                    )
            );
        }

        public void CancelTransitions()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_cancel_transitions(receiverHandle, completion, diagnostic)
            );
        }

        public void DumpDebugLogs()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_dump_debug_logs(receiverHandle, completion, diagnostic)
            );
        }

        public void EndCommandGroup()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_end_command_group(receiverHandle, completion, diagnostic)
            );
        }

        public void InvalidateCustomGeometrySourceRegion(string sourceId, LatLngBounds bounds)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_invalidate_custom_geometry_source_region(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeLatLngBounds(bounds),
                        completion,
                        diagnostic
                    )
            );
        }

        public void InvalidateCustomGeometrySourceTile(string sourceId, CanonicalTileId tileId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_invalidate_custom_geometry_source_tile(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeCanonicalTileId(tileId),
                        completion,
                        diagnostic
                    )
            );
        }

        public void InvalidateCustomMvtVectorSourceTile(string sourceId, CanonicalTileId tileId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_invalidate_custom_mvt_vector_source_tile(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeCanonicalTileId(tileId),
                        completion,
                        diagnostic
                    )
            );
        }

        public void MoveStyleLayer(string layerId, string beforeLayerId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_move_style_layer(
                        receiverHandle,
                        scope.Utf8(layerId),
                        scope.Utf8(beforeLayerId),
                        completion,
                        diagnostic
                    )
            );
        }

        public void RemoveFeatureState(FeatureStateSelector selector)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_remove_feature_state(
                        receiverHandle,
                        scope.Value(NativeFeatureStateSelector(selector, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void RemoveStyleImage(string imageId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_remove_style_image(
                        receiverHandle,
                        scope.Utf8(imageId),
                        completion,
                        diagnostic
                    )
            );
        }

        public void RemoveStyleLayer(string layerId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_remove_style_layer(
                        receiverHandle,
                        scope.Utf8(layerId),
                        completion,
                        diagnostic
                    )
            );
        }

        public void RemoveStyleSource(string sourceId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_remove_style_source(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        completion,
                        diagnostic
                    )
            );
        }

        public void RequestRepaint()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_request_repaint(receiverHandle, completion, diagnostic)
            );
        }

        public void Resize(LogicalExtent extent)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_resize(
                        receiverHandle,
                        NativeLogicalExtent(extent),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetBounds(BoundOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_bounds(
                        receiverHandle,
                        scope.Value(NativeBoundOptions(options)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetCustomGeometrySourceTileData(
            string sourceId,
            CanonicalTileId tileId,
            byte[] data
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_custom_geometry_source_tile_data(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeCanonicalTileId(tileId),
                        scope.Buffer(data),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetCustomMvtVectorSourceTileData(
            string sourceId,
            CanonicalTileId tileId,
            byte[] data
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_custom_mvt_vector_source_tile_data(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeCanonicalTileId(tileId),
                        scope.Buffer(data),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetCustomMvtVectorSourceTileError(
            string sourceId,
            CanonicalTileId tileId,
            string message
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_custom_mvt_vector_source_tile_error(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeCanonicalTileId(tileId),
                        scope.Utf8(message),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetDebugOptions(MapDebugOption options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_debug_options(
                        receiverHandle,
                        (uint)options,
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetEventMask(RuntimeEventMask mask)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_event_mask(
                        receiverHandle,
                        (ulong)mask,
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetFeatureState(FeatureStateSelector selector, byte[] state)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_feature_state(
                        receiverHandle,
                        scope.Value(NativeFeatureStateSelector(selector, scope)),
                        scope.Buffer(state),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetFreeCameraOptions(FreeCameraOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_free_camera_options(
                        receiverHandle,
                        scope.Value(NativeFreeCameraOptions(options)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetGeojsonSourceData(string sourceId, GeojsonSourceDataHandle data)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_geojson_source_data(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Use(data),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetGeojsonSourceSynchronousTiling(string sourceId, bool enabled)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_geojson_source_synchronous_tiling(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        (byte)(enabled ? 1 : 0),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetGeojsonSourceUrl(string sourceId, string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_geojson_source_url(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Utf8(url),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetGlobalStateProperty(string propertyName, byte[] value)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_global_state_property(
                        receiverHandle,
                        scope.Utf8(propertyName),
                        scope.Buffer(value),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetImageSourceCoordinates(string sourceId, LatLng[] coordinates)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_image_source_coordinates(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Array<mln_lat_lng, LatLng>(coordinates, item => NativeLatLng(item)),
                        checked((nuint)coordinates.Length),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetImageSourceImage(string sourceId, PremultipliedRgba8Image image)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_image_source_image(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Value(NativePremultipliedRgba8Image(image, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetImageSourceUrl(string sourceId, string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_image_source_url(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        scope.Utf8(url),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetProjectionMode(ProjectionMode mode)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_projection_mode(
                        receiverHandle,
                        scope.Value(NativeProjectionMode(mode)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetRenderingStatsViewEnabled(bool enabled)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_rendering_stats_view_enabled(
                        receiverHandle,
                        (byte)(enabled ? 1 : 0),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleImage(
            string imageId,
            PremultipliedRgba8Image image,
            StyleImageOptions? options
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_image(
                        receiverHandle,
                        scope.Utf8(imageId),
                        scope.Value(NativePremultipliedRgba8Image(image, scope)),
                        options is null
                            ? null
                            : scope.Value(NativeStyleImageOptions(options, scope)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleJson(byte[] json)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_json(
                        receiverHandle,
                        scope.Buffer(json),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleLayerProperty(string layerId, string propertyName, byte[] value)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_layer_property(
                        receiverHandle,
                        scope.Utf8(layerId),
                        scope.Utf8(propertyName),
                        scope.Buffer(value),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleLightJson(byte[] lightJson)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_light_json(
                        receiverHandle,
                        scope.Buffer(lightJson),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleLightProperty(string propertyName, byte[] value)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_light_property(
                        receiverHandle,
                        scope.Utf8(propertyName),
                        scope.Buffer(value),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleSourceVolatile(string sourceId, bool isVolatile)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_source_volatile(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        (byte)(isVolatile ? 1 : 0),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleTransitionOptions(StyleTransitionOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_transition_options(
                        receiverHandle,
                        scope.Value(NativeStyleTransitionOptions(options)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetStyleUrl(string url)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_style_url(
                        receiverHandle,
                        scope.Utf8(url),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetTileOptions(MapTileOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_tile_options(
                        receiverHandle,
                        scope.Value(NativeMapTileOptions(options)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void SetViewportOptions(MapViewportOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_set_viewport_options(
                        receiverHandle,
                        scope.Value(NativeMapViewportOptions(options)),
                        completion,
                        diagnostic
                    )
            );
        }

        public void UpdateCamera(CameraUpdate update)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_map_update_camera(
                        receiverHandle,
                        scope.Value(NativeCameraUpdate(update, scope)),
                        completion,
                        diagnostic
                    )
            );
        }
    }
}
