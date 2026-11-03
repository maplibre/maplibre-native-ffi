// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A map, with its style and camera.
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
        // Mono may retain a completed continuation on its worker stack.
        // Clear its holder so it no longer roots this handle.
        Ready = completion
            .ContinueWith(
                static (finished, retained) =>
                {
                    ((System.Runtime.CompilerServices.StrongBox<object?>)retained!).Value = null;
                    return finished;
                },
                new System.Runtime.CompilerServices.StrongBox<object?>(this),
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
    /// Adds a source whose GeoJSON tiles come from callbacks.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_add_custom_geometry_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.AlreadyExists</c> when the style already
    /// has a source named <c>sourceId</c>.</description></item>
    /// </list>
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
    /// Adds a source whose vector tiles come from callbacks.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_add_custom_vector_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.AlreadyExists</c> when the style already
    /// has a source named <c>sourceId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> AddCustomVectorSourceAsync(
        string sourceId,
        CustomVectorSourceOptions options
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeCustomVectorSourceOptions(options, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_custom_vector_source(
            Handle,
            scope.Utf8(sourceId),
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Adds a GeoJSON source with prepared data.
    /// </summary>
    /// <remarks>
    /// <para>The source uses the options that <c>data</c> was prepared with.
    /// <c>data</c> can be released once this function returns.</para>
    /// <para>See <c>mln_map_add_geo_json_source_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.AlreadyExists</c> when the style already
    /// has a source named <c>sourceId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> AddGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_add_geo_json_source_data(
            Handle,
            scope.Utf8(sourceId),
            scope.Use(data),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Adds an image source that shows <c>image</c>.
    /// </summary>
    /// <remarks>
    /// <para><c>coordinates</c> are the image's four corners, clockwise from
    /// the top left.</para>
    /// <para>See <c>mln_map_add_image_source_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.AlreadyExists</c> when the style already
    /// has a source named <c>sourceId</c>.</description></item>
    /// </list>
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
    /// Adds a layer to the map's style.
    /// </summary>
    /// <remarks>
    /// <para><c>layerJson</c> is a
    /// [layer](https://maplibre.org/maplibre-style-spec/layers/) object. The
    /// layer goes below the layer named <c>beforeLayerId</c>, or above every
    /// layer when <c>beforeLayerId</c> is empty.</para>
    /// <para>See <c>mln_map_add_style_layer_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no layer
    /// named <c>beforeLayerId</c>, or no source with the ID that the layer
    /// uses.</description></item>
    /// <item><description><c>Status.AlreadyExists</c> when the style already
    /// has a layer with the new layer's ID.</description></item>
    /// </list>
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
    /// Adds a source to the map's style.
    /// </summary>
    /// <remarks>
    /// <para><c>sourceJson</c> is a
    /// [source](https://maplibre.org/maplibre-style-spec/sources/)
    /// object.</para>
    /// <para>See <c>mln_map_add_style_source_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.AlreadyExists</c> when the style already
    /// has a source named <c>sourceId</c>.</description></item>
    /// </list>
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
    /// Moves the camera relative to where it is, as a gesture does.
    /// </summary>
    /// <remarks>
    /// <para><c>delta</c> can pan the map's content by an offset on the screen,
    /// scale the zoom, and add to the bearing and pitch. An anchor keeps one
    /// screen point in place while the scale, bearing, or pitch changes. An
    /// anchor needs one of those changes, and combines with an offset only when
    /// the delta has no animation. The function finishes when the map applies
    /// the delta, before an animation ends.</para>
    /// <para>See <c>mln_map_apply_camera_delta</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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
    /// Creates a session that renders the map into a ring of your Metal
    /// textures.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_attach_metal_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.SupportedRenderBackendMask()</c> lacks
    /// <c>RenderBackendFlag.Metal</c>, or when the target supports none of the
    /// <c>producer_sync_kinds</c> in <c>options</c>.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
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
    /// Creates a session that renders the map to a Metal layer.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_attach_metal_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.SupportedRenderBackendMask()</c> lacks
    /// <c>RenderBackendFlag.Metal</c>.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
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
    /// Attaches a render session that renders the map offscreen.
    /// </summary>
    /// <remarks>
    /// <para>Read each image's pixels with
    /// <c>RenderSessionHandle.ReadTextureAsync()</c>.</para>
    /// <para>The session renders on <c>device</c>. When <c>device</c> is null,
    /// the session creates a device with the default options, and releases it
    /// when the session ends.</para>
    /// <para>See <c>mln_map_attach_offscreen</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> when the map already has a
    /// render session.</description></item>
    /// <item><description><c>Status.NativeError</c> when <c>device</c> is null
    /// and the platform can't create a device.</description></item>
    /// </list>
    /// </remarks>
    public RenderSessionHandle AttachOffscreen(
        OffscreenDescriptor descriptor,
        RenderSessionAttachOptions options,
        OffscreenDeviceHandle? device = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeOffscreenDescriptor(descriptor);
        var nativeOptions = NativeRenderSessionAttachOptions(options, scope);
        MlnRenderSession outSession = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_attach_offscreen(
            Handle,
            &nativeDescriptor,
            &nativeOptions,
            scope.UseOptional(device),
            &outSession,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return RenderSessionHandle.Adopt(this, outSession, nativeReady);
    }

    /// <summary>
    /// Creates a session that renders the map into a ring of your OpenGL
    /// textures.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_attach_open_gl_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.OpenGlSupportedContextProviderMask()</c> lacks the
    /// context's platform, or when the target supports none of the
    /// <c>producer_sync_kinds</c> in <c>options</c>.</description></item>
    /// </list>
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
        var nativeStatus = NativeMethods.mln_map_attach_open_gl_borrowed_texture(
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
    /// Creates a session that renders the map to an OpenGL surface.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_attach_open_gl_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.OpenGlSupportedContextProviderMask()</c> lacks the
    /// context's platform.</description></item>
    /// </list>
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
        var nativeStatus = NativeMethods.mln_map_attach_open_gl_surface(
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
    /// Creates a session that renders the map into a ring of your Vulkan
    /// images.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_attach_vulkan_borrowed_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.SupportedRenderBackendMask()</c> lacks
    /// <c>RenderBackendFlag.Vulkan</c>, or when the target supports none of the
    /// <c>producer_sync_kinds</c> in <c>options</c>.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
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
    /// Creates a session that renders the map to a Vulkan surface.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_attach_vulkan_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>MaplibreNative.SupportedRenderBackendMask()</c> lacks
    /// <c>RenderBackendFlag.Vulkan</c>.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
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
    /// Holds back the map's changes from rendering until
    /// <c>MapHandle.EndCommandGroupAsync()</c>, so they show in the same frame.
    /// </summary>
    /// <remarks>
    /// <para>Groups nest, and the changes render when the outermost group
    /// ends.</para>
    /// <para>See <c>mln_map_begin_command_group</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
    /// Computes a camera that fits a GeoJSON geometry in the map's view.
    /// </summary>
    /// <remarks>
    /// <para><c>geometry</c> is a GeoJSON geometry object with at least one
    /// coordinate, not a feature. The camera keeps the map's bearing and pitch
    /// unless <c>fitOptions</c> sets them, and its zoom stays within the map's
    /// zoom limits.</para>
    /// <para>See <c>mln_map_camera_for_geometry</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when the padding in
    /// <c>fitOptions</c> leaves no room for the area: its left and right add up
    /// to at least the map's width, or its top and bottom to at least the map's
    /// height.</description></item>
    /// </list>
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
    /// Computes a camera that fits <c>bounds</c> in the map's view.
    /// </summary>
    /// <remarks>
    /// <para>The camera keeps the map's bearing and pitch unless
    /// <c>fitOptions</c> sets them, and its zoom stays within the map's zoom
    /// limits.</para>
    /// <para>See <c>mln_map_camera_for_lat_lng_bounds</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when the padding in
    /// <c>fitOptions</c> leaves no room for the area: its left and right add up
    /// to at least the map's width, or its top and bottom to at least the map's
    /// height.</description></item>
    /// </list>
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
    /// Computes a camera that fits <c>coordinates</c> in the map's view.
    /// </summary>
    /// <remarks>
    /// <para><c>coordinates</c> must not be empty. The camera keeps the map's
    /// bearing and pitch unless <c>fitOptions</c> sets them, and its zoom stays
    /// within the map's zoom limits.</para>
    /// <para>See <c>mln_map_camera_for_lat_lngs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when the padding in
    /// <c>fitOptions</c> leaves no room for the area: its left and right add up
    /// to at least the map's width, or its top and bottom to at least the map's
    /// height.</description></item>
    /// </list>
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
    /// Stops every running camera animation where it is.
    /// </summary>
    /// <remarks>
    /// <para>The end handler of each stopped animation reports
    /// <c>CameraTransitionOutcome.Cancelled</c>.</para>
    /// <para>See <c>mln_map_cancel_camera_transitions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<CommandCompletion> CancelCameraTransitionsAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_cancel_camera_transitions(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Stops the running camera animations whose animation options set
    /// <c>transitionId</c>.
    /// </summary>
    /// <remarks>
    /// <para>Other animations keep running. The end handler of each stopped
    /// animation reports <c>CameraTransitionOutcome.Cancelled</c>.</para>
    /// <para>See <c>mln_map_cancel_camera_transitions_with_id</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<CommandCompletion> CancelCameraTransitionsWithIdAsync(ulong transitionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_cancel_camera_transitions_with_id(
            Handle,
            transitionId,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Logs debugging information about the map's style and sources.
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
    /// Ends the group that <c>MapHandle.BeginCommandGroupAsync()</c> began.
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
    /// Moves the camera to fit a GeoJSON geometry in the map's view, at once or
    /// with an animation.
    /// </summary>
    /// <remarks>
    /// <para><c>geometry</c> is a GeoJSON geometry object with at least one
    /// coordinate, not a feature. The map computes the camera when it runs the
    /// command, from the camera and size that earlier commands leave, as
    /// <c>MapHandle.CameraForGeometryAsync()</c> would then. The function
    /// finishes when the map applies the change, before an animation
    /// ends.</para>
    /// <para>See <c>mln_map_fit_camera_to_geometry</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when the padding in
    /// <c>update</c> leaves no room for the area: its left and right add up to
    /// at least the map's width, or its top and bottom to at least the map's
    /// height. The camera doesn't move.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> FitCameraToGeometryAsync(string geometry, CameraFitUpdate update)
    {
        using var scope = new NativeCallScope(this);
        var nativeUpdate = NativeCameraFitUpdate(update, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_fit_camera_to_geometry(
            Handle,
            scope.Utf8(geometry),
            &nativeUpdate,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Moves the camera to fit <c>bounds</c> in the map's view, at once or with
    /// an animation.
    /// </summary>
    /// <remarks>
    /// <para>The map computes the camera when it runs the command, from the
    /// camera and size that earlier commands leave, as
    /// <c>MapHandle.CameraForLatLngBoundsAsync()</c> would then. The function
    /// finishes when the map applies the change, before an animation
    /// ends.</para>
    /// <para>See <c>mln_map_fit_camera_to_lat_lng_bounds</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when the padding in
    /// <c>update</c> leaves no room for the area: its left and right add up to
    /// at least the map's width, or its top and bottom to at least the map's
    /// height. The camera doesn't move.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> FitCameraToLatLngBoundsAsync(
        LatLngBounds bounds,
        CameraFitUpdate update
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeUpdate = NativeCameraFitUpdate(update, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_fit_camera_to_lat_lng_bounds(
            Handle,
            NativeLatLngBounds(bounds),
            &nativeUpdate,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Moves the camera to fit <c>coordinates</c> in the map's view, at once or
    /// with an animation.
    /// </summary>
    /// <remarks>
    /// <para><c>coordinates</c> must not be empty. The map computes the camera
    /// when it runs the command, from the camera and size that earlier commands
    /// leave, as <c>MapHandle.CameraForLatLngsAsync()</c> would then. The
    /// function finishes when the map applies the change, before an animation
    /// ends.</para>
    /// <para>See <c>mln_map_fit_camera_to_lat_lngs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when the padding in
    /// <c>update</c> leaves no room for the area: its left and right add up to
    /// at least the map's width, or its top and bottom to at least the map's
    /// height. The camera doesn't move.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> FitCameraToLatLngsAsync(
        LatLng[] coordinates,
        CameraFitUpdate update
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeUpdate = NativeCameraFitUpdate(update, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_fit_camera_to_lat_lngs(
            Handle,
            scope.Array<mln_lat_lng, LatLng>(
                coordinates,
                static (item, scope) => NativeLatLng(item)
            ),
            checked((nuint)coordinates.Length),
            &nativeUpdate,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Reads the map's camera.
    /// </summary>
    /// <remarks>
    /// <para>The query reads the camera after the map applies the commands sent
    /// before it.</para>
    /// <para>See <c>mln_map_get_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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
    /// Reads the state of a feature, as a JSON object.
    /// </summary>
    /// <remarks>
    /// <para>The object is empty when the feature has no state.</para>
    /// <para>See <c>mln_map_get_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
    /// Reads the global state of the map's style as a JSON object.
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
    /// Reads the corners of an image source's image, clockwise from the top
    /// left.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_get_image_source_coordinates</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't an image source.</description></item>
    /// </list>
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
    /// Reads the JSON of the style that the map loaded.
    /// </summary>
    /// <remarks>
    /// <para>The JSON doesn't include the changes made to the style since it
    /// loaded.</para>
    /// <para>See <c>mln_map_get_loaded_style_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
    /// Copies the map's state.
    /// </summary>
    /// <remarks>
    /// <para>The copy doesn't wait for pending commands. To read the camera
    /// after they apply, use <c>MapHandle.GetCameraAsync()</c>.</para>
    /// <para>See <c>mln_map_get_snapshot</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// </remarks>
    public MapSnapshot GetSnapshot()
    {
        using var call = Enter(this);
        var outSnapshot = new mln_map_snapshot { size = (uint)sizeof(mln_map_snapshot) };
        Check(NativeMethods.mln_map_get_snapshot(Handle, &outSnapshot, Diagnostic));
        return CopyMapSnapshot(outSnapshot);
    }

    /// <summary>
    /// Reads an image of the map's style.
    /// </summary>
    /// <remarks>
    /// <para>Completes with no value when the style has no image named
    /// <c>imageId</c>.</para>
    /// <para>See <c>mln_map_get_style_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
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
    /// Reads a layer of the map's style.
    /// </summary>
    /// <remarks>
    /// <para>Completes with no value when the style has no layer named
    /// <c>layerId</c>.</para>
    /// <para>See <c>mln_map_get_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
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
    /// Reads a layer of the map's style as a
    /// [layer](https://maplibre.org/maplibre-style-spec/layers/) object.
    /// </summary>
    /// <remarks>
    /// <para>Completes with no value when the style has no layer named
    /// <c>layerId</c>.</para>
    /// <para>See <c>mln_map_get_style_layer_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
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
    /// Reads a property of a layer in the map's style.
    /// </summary>
    /// <remarks>
    /// <para>Completes with no value when the layer doesn't set the
    /// property.</para>
    /// <para>See <c>mln_map_get_style_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no layer
    /// named <c>layerId</c>.</description></item>
    /// </list>
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
    /// Reads a property of the light of the map's style.
    /// </summary>
    /// <remarks>
    /// <para>Completes with no value when the light doesn't set the
    /// property.</para>
    /// <para>See <c>mln_map_get_style_light_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
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
    /// Reads a source of the map's style.
    /// </summary>
    /// <remarks>
    /// <para>Completes with no value when the style has no source named
    /// <c>sourceId</c>.</para>
    /// <para>See <c>mln_map_get_style_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
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
    /// Reads the default transition of the map's style.
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
    /// Reads the URL of the map's style.
    /// </summary>
    /// <remarks>
    /// <para>The URL is empty when the style came from
    /// <c>MapHandle.SetStyleJsonAsync()</c>.</para>
    /// <para>See <c>mln_map_get_style_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
    /// Makes a custom geometry source ask again for the tiles in <c>bounds</c>
    /// that the map still needs.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_invalidate_custom_geometry_source_region</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a custom geometry source.</description></item>
    /// </list>
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
    /// Makes a custom geometry source ask for a tile again when the map needs
    /// it.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_invalidate_custom_geometry_source_tile</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a custom geometry source.</description></item>
    /// </list>
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
    /// Makes a custom vector source ask for a tile again when the map needs it.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_invalidate_custom_vector_source_tile</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a custom vector source.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> InvalidateCustomVectorSourceTileAsync(
        string sourceId,
        CanonicalTileId tileId
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_invalidate_custom_vector_source_tile(
            Handle,
            scope.Utf8(sourceId),
            NativeCanonicalTileId(tileId),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Computes the area that the map would show with <c>camera</c>.
    /// </summary>
    /// <remarks>
    /// <para>Camera fields that <c>camera</c> doesn't set come from the map's
    /// camera. The longitudes can fall outside -180 to 180, so a view across
    /// the antimeridian gives one continuous range.</para>
    /// <para>See <c>mln_map_lat_lng_bounds_for_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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
    /// Lists the layers of the map's style, from bottom to top.
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
    /// Lists the sources of the map's style.
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
    /// Moves a layer of the map's style below the layer named
    /// <c>beforeLayerId</c>, or above every layer when <c>beforeLayerId</c> is
    /// empty.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_move_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no layer
    /// named <c>layerId</c> or <c>beforeLayerId</c>.</description></item>
    /// </list>
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
    /// Releases the map.
    /// </summary>
    /// <remarks>
    /// <para>Detach the map's render session first.</para>
    /// <para>See <c>mln_map_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
    /// Removes the feature state that <c>selector</c> selects.
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
    /// Removes an image from the map's style.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_remove_style_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no image
    /// named <c>imageId</c>.</description></item>
    /// </list>
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
    /// Removes a layer from the map's style.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_remove_style_layer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no layer
    /// named <c>layerId</c>.</description></item>
    /// </list>
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
    /// Removes a source from the map's style.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_remove_style_source</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidState</c> when a layer still uses
    /// the source.</description></item>
    /// </list>
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
    /// Renders a new frame of a continuous map, even when nothing changed.
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
    /// Renders a still image of a static or tile map.
    /// </summary>
    /// <remarks>
    /// <para>The map's render session draws the image once every resource that
    /// the image needs has loaded.</para>
    /// <para>See <c>mln_map_request_still_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Cancelled</c> when the map is released
    /// before the image is done.</description></item>
    /// <item><description><c>Status.NativeError</c> when the style or a
    /// resource that the image needs fails to load.</description></item>
    /// </list>
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
    /// Sets the map's size.
    /// </summary>
    /// <remarks>
    /// <para>A map with a render session takes its size from the session, so
    /// resize the session with <c>RenderSessionHandle.ResizeAsync()</c>
    /// instead.</para>
    /// <para>See <c>mln_map_resize</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> when the map has a render
    /// session.</description></item>
    /// <item><description><c>Status.InvalidState</c> when a render session
    /// attached to the map before the resize applied.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> ResizeAsync(LogicalSize logicalSize)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_resize(
            Handle,
            NativeLogicalSize(logicalSize),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Limits the camera's center to an area, and its zoom and pitch to ranges.
    /// </summary>
    /// <remarks>
    /// <para>Limits that <c>options</c> doesn't set keep their current values.
    /// When the camera is outside the new limits, it moves inside them.</para>
    /// <para>See <c>mln_map_set_bounds</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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
    /// Sets the data of a tile in a custom geometry source.
    /// </summary>
    /// <remarks>
    /// <para><c>data</c> is a GeoJSON feature collection. The source ignores
    /// data for a tile that it hasn't asked for, or has since cancelled.</para>
    /// <para>See <c>mln_map_set_custom_geometry_source_tile_data</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a custom geometry source.</description></item>
    /// </list>
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
    /// Reports that a tile of a custom geometry source failed to load.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_set_custom_geometry_source_tile_error</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a custom geometry source.</description></item>
    /// </list>
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
    /// Sets the data of a tile in a custom vector source.
    /// </summary>
    /// <remarks>
    /// <para><c>data</c> is the tile, encoded as <c>encoding</c>. The source
    /// ignores data for a tile that it hasn't asked for, or has since
    /// cancelled.</para>
    /// <para>See <c>mln_map_set_custom_vector_source_tile_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a custom vector source.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> SetCustomVectorSourceTileDataAsync(
        string sourceId,
        CanonicalTileId tileId,
        StyleVectorTileEncoding encoding,
        ReadOnlySpan<byte> data
    )
    {
        fixed (byte* pinnedData = data)
        {
            using var scope = new NativeCallScope(this);
            mln_completion nativeCompletion;
            var nativeStatus = NativeMethods.mln_map_set_custom_vector_source_tile_data(
                Handle,
                scope.Utf8(sourceId),
                NativeCanonicalTileId(tileId),
                (mln_style_vector_tile_encoding)encoding,
                new mln_buffer_view { data = pinnedData, size = (nuint)data.Length },
                NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
                Diagnostic
            );
            return scope.Accepted(nativePending.Submitted(nativeStatus));
        }
    }

    /// <summary>
    /// Reports that a tile of a custom vector source failed to load.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_set_custom_vector_source_tile_error</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a custom vector source.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> SetCustomVectorSourceTileErrorAsync(
        string sourceId,
        CanonicalTileId tileId,
        string message
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_custom_vector_source_tile_error(
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
    /// Draws debugging overlays, such as tile borders, on the map.
    /// </summary>
    /// <remarks>
    /// <para><c>flags</c> replaces the overlays set before.</para>
    /// <para>See <c>mln_map_set_debug_flags</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<CommandCompletion> SetDebugFlagsAsync(MapDebugFlag flags)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_debug_flags(
            Handle,
            (uint)flags,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Selects the events that the map reports.
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
    /// Merges <c>state</c> into the state of a feature.
    /// </summary>
    /// <remarks>
    /// <para><c>state</c> is a JSON object. Its keys replace the same keys in
    /// the feature's state, and the other keys stay.</para>
    /// <para>See <c>mln_map_set_feature_state</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
    /// Places the camera at a position and orientation in space.
    /// </summary>
    /// <remarks>
    /// <para>The map adjusts them to a camera that it can show. It stops
    /// running camera animations first, and their end handlers report
    /// <c>CameraTransitionOutcome.Cancelled</c>.</para>
    /// <para>See <c>mln_map_set_free_camera_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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
    /// Replaces the data of a GeoJSON source.
    /// </summary>
    /// <remarks>
    /// <para><c>data</c> must have been prepared with the same options as the
    /// source. It can be released once this function returns.</para>
    /// <para>See <c>mln_map_set_geo_json_source_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a GeoJSON source, or <c>data</c> was prepared with
    /// other options.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceDataAsync(
        string sourceId,
        GeoJsonSourceDataHandle data
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_geo_json_source_data(
            Handle,
            scope.Utf8(sourceId),
            scope.Use(data),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets whether a GeoJSON source builds its tiles synchronously.
    /// </summary>
    /// <remarks>
    /// <para>This works like the synchronous tiling option of
    /// <c>GeoJsonSourceOptions</c>, but <c>enabled</c> can't turn off
    /// synchronous tiling that the source's options turned on.</para>
    /// <para>See <c>mln_map_set_geo_json_source_synchronous_tiling</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a GeoJSON source.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceSynchronousTilingAsync(
        string sourceId,
        bool enabled
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_geo_json_source_synchronous_tiling(
            Handle,
            scope.Utf8(sourceId),
            (byte)(enabled ? 1 : 0),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Makes a GeoJSON source load its data from <c>url</c>.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_set_geo_json_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't a GeoJSON source.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> SetGeoJsonSourceUrlAsync(string sourceId, string url)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_geo_json_source_url(
            Handle,
            scope.Utf8(sourceId),
            scope.Utf8(url),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets a value that the style's <c>global-state</c>
    /// [expressions](https://maplibre.org/maplibre-style-spec/expressions/)
    /// read.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_set_global_state_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> until the map has parsed
    /// its style, which <c>RuntimeEventType.MapStyleLoaded</c> reports, and
    /// again while it loads a new style URL.</description></item>
    /// </list>
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
    /// Moves the image of an image source.
    /// </summary>
    /// <remarks>
    /// <para><c>coordinates</c> are the image's four corners, clockwise from
    /// the top left.</para>
    /// <para>See <c>mln_map_set_image_source_coordinates</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't an image source.</description></item>
    /// </list>
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
    /// Replaces the image of an image source.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_set_image_source_image</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't an image source.</description></item>
    /// </list>
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
    /// Makes an image source load its image from <c>url</c>.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_map_set_image_source_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source named
    /// <c>sourceId</c> isn't an image source.</description></item>
    /// </list>
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
    /// Sets whether the map draws in perspective or axonometric projection, and
    /// how much it skews the view.
    /// </summary>
    /// <remarks>
    /// <para>Settings that <c>options</c> doesn't set keep their current
    /// values.</para>
    /// <para>See <c>mln_map_set_projection_mode</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<CommandCompletion> SetProjectionModeAsync(ProjectionModeOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeProjectionModeOptions(options);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_map_set_projection_mode(
            Handle,
            &nativeOptions,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Shows or hides an overlay of rendering statistics on the map.
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
    /// Adds an image to the map's style, or replaces the image named
    /// <c>imageId</c>.
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
    /// Loads the map's style from <c>json</c>.
    /// </summary>
    /// <remarks>
    /// <para>The style's sources, sprites, and glyphs load in the
    /// background.</para>
    /// <para>See <c>mln_map_set_style_json</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NativeError</c> when <c>json</c> isn't a
    /// valid style.</description></item>
    /// </list>
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
    /// Sets a property of a layer in the map's style.
    /// </summary>
    /// <remarks>
    /// <para><c>propertyName</c> is one of the layer's layout or paint
    /// [properties](https://maplibre.org/maplibre-style-spec/layers/), or
    /// <c>filter</c>, <c>minzoom</c>, <c>maxzoom</c>, <c>source</c>, or
    /// <c>source-layer</c>. A null <c>value</c> resets any of these but
    /// <c>source</c> and <c>source-layer</c> to its default.</para>
    /// <para>See <c>mln_map_set_style_layer_property</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no layer
    /// named <c>layerId</c>.</description></item>
    /// </list>
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
    /// Replaces the [light](https://maplibre.org/maplibre-style-spec/light/) of
    /// the map's style.
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
    /// Sets a property of the light of the map's style.
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
    /// Sets whether a source is volatile.
    /// </summary>
    /// <remarks>
    /// <para>The map doesn't store the tiles of a volatile source in the
    /// persistent cache.</para>
    /// <para>See <c>mln_map_set_style_source_volatile</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the style has no source
    /// named <c>sourceId</c>.</description></item>
    /// </list>
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
    /// Sets the default transition of the map's style.
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
    /// Loads the map's style from <c>url</c>.
    /// </summary>
    /// <remarks>
    /// <para>The style loads in the background, and
    /// <c>RuntimeEventType.MapStyleLoaded</c> or
    /// <c>RuntimeEventType.MapLoadingFailed</c> reports how the load ends.
    /// Sources, layers, and images added before the style loads are lost when
    /// it loads.</para>
    /// <para>See <c>mln_map_set_style_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
    /// Sets how the map chooses which tiles to load, and at what detail.
    /// </summary>
    /// <remarks>
    /// <para>Settings that <c>options</c> doesn't set keep their current
    /// values.</para>
    /// <para>See <c>mln_map_set_tile_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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
    /// Sets how the map orients and constrains its view.
    /// </summary>
    /// <remarks>
    /// <para>Settings that <c>options</c> doesn't set keep their current
    /// values.</para>
    /// <para>See <c>mln_map_set_viewport_options</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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
    /// Moves the camera, at once or with an animation.
    /// </summary>
    /// <remarks>
    /// <para>Camera fields that <c>update</c> doesn't set keep their values,
    /// and the update stops any running animation of the fields it sets. The
    /// function finishes when the map applies the update, before an animation
    /// ends. To learn when an animation ends, set an
    /// <c>CameraTransitionHandler</c> in its animation options.</para>
    /// <para>See <c>mln_map_update_camera</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html">C API reference</see>.</para>
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

        public void AddCustomVectorSource(string sourceId, CustomVectorSourceOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeCustomVectorSourceOptions(options, scope);
            Check(
                NativeMethods.mln_map_add_custom_vector_source(
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
                NativeMethods.mln_map_add_geo_json_source_data(
                    receiverHandle,
                    scope.Utf8(sourceId),
                    scope.Use(data),
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

        public void CancelCameraTransitions()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_cancel_camera_transitions(receiverHandle, null, Diagnostic)
            );
            scope.Accept();
        }

        public void CancelCameraTransitionsWithId(ulong transitionId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_cancel_camera_transitions_with_id(
                    receiverHandle,
                    transitionId,
                    null,
                    Diagnostic
                )
            );
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

        public void FitCameraToGeometry(string geometry, CameraFitUpdate update)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeUpdate = NativeCameraFitUpdate(update, scope);
            Check(
                NativeMethods.mln_map_fit_camera_to_geometry(
                    receiverHandle,
                    scope.Utf8(geometry),
                    &nativeUpdate,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void FitCameraToLatLngBounds(LatLngBounds bounds, CameraFitUpdate update)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeUpdate = NativeCameraFitUpdate(update, scope);
            Check(
                NativeMethods.mln_map_fit_camera_to_lat_lng_bounds(
                    receiverHandle,
                    NativeLatLngBounds(bounds),
                    &nativeUpdate,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void FitCameraToLatLngs(LatLng[] coordinates, CameraFitUpdate update)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeUpdate = NativeCameraFitUpdate(update, scope);
            Check(
                NativeMethods.mln_map_fit_camera_to_lat_lngs(
                    receiverHandle,
                    scope.Array<mln_lat_lng, LatLng>(
                        coordinates,
                        static (item, scope) => NativeLatLng(item)
                    ),
                    checked((nuint)coordinates.Length),
                    &nativeUpdate,
                    null,
                    Diagnostic
                )
            );
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

        public void InvalidateCustomVectorSourceTile(string sourceId, CanonicalTileId tileId)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_invalidate_custom_vector_source_tile(
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

        public void Resize(LogicalSize logicalSize)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_resize(
                    receiverHandle,
                    NativeLogicalSize(logicalSize),
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

        public void SetCustomVectorSourceTileData(
            string sourceId,
            CanonicalTileId tileId,
            StyleVectorTileEncoding encoding,
            ReadOnlySpan<byte> data
        )
        {
            fixed (byte* pinnedData = data)
            {
                var receiverHandle = owner.Handle;
                using var scope = new NativeCallScope(owner);
                Check(
                    NativeMethods.mln_map_set_custom_vector_source_tile_data(
                        receiverHandle,
                        scope.Utf8(sourceId),
                        NativeCanonicalTileId(tileId),
                        (mln_style_vector_tile_encoding)encoding,
                        new mln_buffer_view { data = pinnedData, size = (nuint)data.Length },
                        null,
                        Diagnostic
                    )
                );
                scope.Accept();
            }
        }

        public void SetCustomVectorSourceTileError(
            string sourceId,
            CanonicalTileId tileId,
            string message
        )
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_custom_vector_source_tile_error(
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

        public void SetDebugFlags(MapDebugFlag flags)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_map_set_debug_flags(receiverHandle, (uint)flags, null, Diagnostic)
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
                NativeMethods.mln_map_set_geo_json_source_data(
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
                NativeMethods.mln_map_set_geo_json_source_synchronous_tiling(
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
                NativeMethods.mln_map_set_geo_json_source_url(
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

        public void SetProjectionMode(ProjectionModeOptions options)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeOptions = NativeProjectionModeOptions(options);
            Check(
                NativeMethods.mln_map_set_projection_mode(
                    receiverHandle,
                    &nativeOptions,
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
