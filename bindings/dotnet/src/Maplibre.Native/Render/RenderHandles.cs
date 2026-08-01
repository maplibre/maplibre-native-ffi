using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Pointer;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Map;
using Maplibre.Native.Query;

namespace Maplibre.Native.Render;

internal unsafe delegate mln_status OpenGLSurfaceAttach(
    MlnMap map,
    mln_opengl_surface_descriptor* descriptor,
    MlnRenderSession* outSession
);

internal unsafe delegate mln_status OpenGLOwnedTextureAttach(
    MlnMap map,
    mln_opengl_owned_texture_descriptor* descriptor,
    MlnRenderSession* outSession
);

internal unsafe delegate mln_status OpenGLBorrowedTextureAttach(
    MlnMap map,
    mln_opengl_borrowed_texture_descriptor* descriptor,
    MlnRenderSession* outSession
);

internal unsafe delegate mln_status RenderSessionResize(
    MlnRenderSession session,
    uint width,
    uint height,
    double scaleFactor
);

internal unsafe delegate mln_status RenderSessionRenderUpdate(
    MlnRenderSession session,
    bool* out_rendered
);

internal unsafe delegate mln_status MetalOwnedTextureAcquireFrame(
    MlnRenderSession session,
    mln_metal_owned_texture_frame* frame
);

internal unsafe delegate mln_status MetalOwnedTextureReleaseFrame(
    MlnRenderSession session,
    mln_metal_owned_texture_frame* frame
);

/// <summary>Owner-thread render session handle bound to a map.</summary>
public sealed unsafe class RenderSessionHandle : IDisposable
{
    private static readonly OpenGLSurfaceAttach DefaultOpenGLSurfaceAttach = static (
        map,
        descriptor,
        outSession
    ) => NativeMethods.mln_opengl_surface_attach(map, descriptor, outSession);
    private static readonly OpenGLOwnedTextureAttach DefaultOpenGLOwnedTextureAttach = static (
        map,
        descriptor,
        outSession
    ) => NativeMethods.mln_opengl_owned_texture_attach(map, descriptor, outSession);
    private static readonly OpenGLBorrowedTextureAttach DefaultOpenGLBorrowedTextureAttach =
        static (map, descriptor, outSession) =>
            NativeMethods.mln_opengl_borrowed_texture_attach(map, descriptor, outSession);
    private static readonly RenderSessionResize DefaultResize = static (
        session,
        width,
        height,
        scaleFactor
    ) => NativeMethods.mln_render_session_resize(session, width, height, scaleFactor);
    private static readonly RenderSessionRenderUpdate DefaultRenderUpdate = static (
        session,
        outRendered
    ) => NativeMethods.mln_render_session_render_update(session, outRendered);
    private static readonly TextureRead DefaultTextureRead = static (session, data, length, info) =>
        NativeMethods.mln_texture_read_premultiplied_rgba8(session, data, length, info);
    private static readonly StatusDestroy<MlnRenderSession> DefaultDestroy = static session =>
        NativeMethods.mln_render_session_destroy(session);
    private static readonly MetalOwnedTextureAcquireFrame DefaultAcquireMetalFrame = static (
        session,
        frame
    ) => NativeMethods.mln_metal_owned_texture_acquire_frame(session, frame);
    private static readonly MetalOwnedTextureReleaseFrame DefaultReleaseMetalFrame = static (
        session,
        frame
    ) => NativeMethods.mln_metal_owned_texture_release_frame(session, frame);
    private static readonly VulkanOwnedTextureAcquireFrame DefaultAcquireVulkanFrame = static (
        session,
        frame
    ) => NativeMethods.mln_vulkan_owned_texture_acquire_frame(session, frame);
    private static readonly OpenGLOwnedTextureAcquireFrame DefaultAcquireOpenGLFrame = static (
        session,
        frame
    ) => NativeMethods.mln_opengl_owned_texture_acquire_frame(session, frame);

    [ThreadStatic]
    private static OpenGLSurfaceAttach? openGLSurfaceAttachForTest;

    [ThreadStatic]
    private static OpenGLOwnedTextureAttach? openGLOwnedTextureAttachForTest;

    [ThreadStatic]
    private static OpenGLBorrowedTextureAttach? openGLBorrowedTextureAttachForTest;

    [ThreadStatic]
    private static RenderSessionResize? resizeForTest;

    [ThreadStatic]
    private static RenderSessionRenderUpdate? renderUpdateForTest;

    [ThreadStatic]
    private static TextureRead? textureReadForTest;

    [ThreadStatic]
    private static StatusDestroy<MlnRenderSession>? destroyForTest;

    [ThreadStatic]
    private static MetalOwnedTextureAcquireFrame? acquireMetalFrameForTest;

    [ThreadStatic]
    private static MetalOwnedTextureReleaseFrame? releaseMetalFrameForTest;

    [ThreadStatic]
    private static VulkanOwnedTextureAcquireFrame? acquireVulkanFrameForTest;

    [ThreadStatic]
    private static OpenGLOwnedTextureAcquireFrame? acquireOpenGLFrameForTest;

    [ThreadStatic]
    private static Func<
        mln_metal_owned_texture_frame,
        FrameScope,
        MetalOwnedTextureFrame
    >? readMetalFrameForTest;

    private readonly object frameGate = new();
    private readonly MapHandle? map;
    private readonly NativeHandleState<MlnRenderSession> state;
    private bool hasActiveTextureFrame;

    private RenderSessionHandle(MapHandle? map, MlnRenderSession handle)
        : this(map, handle, DestroyNative) { }

    private RenderSessionHandle(
        MapHandle? map,
        MlnRenderSession handle,
        StatusDestroy<MlnRenderSession> destroy
    )
    {
        this.map = map;
        state = new NativeHandleState<MlnRenderSession>(
            handle,
            destroy,
            nameof(RenderSessionHandle)
        );
    }

    internal static RenderSessionHandle CreateForTest(MlnRenderSession handle) =>
        new(null, handle, static _ => mln_status.MLN_STATUS_OK);

    internal static IDisposable UseOpenGLAttachMethodsForTest(
        OpenGLSurfaceAttach surfaceAttach,
        OpenGLOwnedTextureAttach ownedTextureAttach,
        OpenGLBorrowedTextureAttach borrowedTextureAttach
    )
    {
        var previousSurface = openGLSurfaceAttachForTest;
        var previousOwnedTexture = openGLOwnedTextureAttachForTest;
        var previousBorrowedTexture = openGLBorrowedTextureAttachForTest;
        openGLSurfaceAttachForTest = surfaceAttach;
        openGLOwnedTextureAttachForTest = ownedTextureAttach;
        openGLBorrowedTextureAttachForTest = borrowedTextureAttach;
        return new RestoreOpenGLAttachMethods(
            previousSurface,
            previousOwnedTexture,
            previousBorrowedTexture
        );
    }

    internal static IDisposable UseSessionMethodsForTest(
        RenderSessionResize resize,
        RenderSessionRenderUpdate renderUpdate,
        TextureRead textureRead,
        StatusDestroy<MlnRenderSession> destroy
    )
    {
        var previousResize = resizeForTest;
        var previousRenderUpdate = renderUpdateForTest;
        var previousTextureRead = textureReadForTest;
        var previousDestroy = destroyForTest;
        resizeForTest = resize;
        renderUpdateForTest = renderUpdate;
        textureReadForTest = textureRead;
        destroyForTest = destroy;
        return new RestoreSessionMethods(
            previousResize,
            previousRenderUpdate,
            previousTextureRead,
            previousDestroy
        );
    }

    internal static IDisposable UseMetalFrameMethodsForTest(
        MetalOwnedTextureAcquireFrame acquire,
        MetalOwnedTextureReleaseFrame release,
        Func<mln_metal_owned_texture_frame, FrameScope, MetalOwnedTextureFrame> readFrame
    )
    {
        var previousAcquire = acquireMetalFrameForTest;
        var previousRelease = releaseMetalFrameForTest;
        var previousRead = readMetalFrameForTest;
        acquireMetalFrameForTest = acquire;
        releaseMetalFrameForTest = release;
        readMetalFrameForTest = readFrame;
        return new RestoreMetalFrameMethods(previousAcquire, previousRelease, previousRead);
    }

    internal static IDisposable UseTextureFrameAcquireMethodsForTest(
        VulkanOwnedTextureAcquireFrame acquireVulkan,
        OpenGLOwnedTextureAcquireFrame acquireOpenGL
    )
    {
        var previousVulkan = acquireVulkanFrameForTest;
        var previousOpenGL = acquireOpenGLFrameForTest;
        acquireVulkanFrameForTest = acquireVulkan;
        acquireOpenGLFrameForTest = acquireOpenGL;
        return new RestoreTextureFrameAcquireMethods(previousVulkan, previousOpenGL);
    }

    public static RenderSessionHandle AttachMetalSurface(
        MapHandle map,
        MetalSurfaceDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(NativeMethods.mln_metal_surface_attach(map.Handle, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachVulkanSurface(
        MapHandle map,
        VulkanSurfaceDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(NativeMethods.mln_vulkan_surface_attach(map.Handle, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachOpenGLSurface(
        MapHandle map,
        OpenGLSurfaceDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(OpenGLSurfaceAttachNative(map.Handle, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachMetalOwnedTexture(
        MapHandle map,
        MetalOwnedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(
            NativeMethods.mln_metal_owned_texture_attach(map.Handle, &native, &session)
        );
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachMetalBorrowedTexture(
        MapHandle map,
        MetalBorrowedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(
            NativeMethods.mln_metal_borrowed_texture_attach(map.Handle, &native, &session)
        );
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachVulkanOwnedTexture(
        MapHandle map,
        VulkanOwnedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(
            NativeMethods.mln_vulkan_owned_texture_attach(map.Handle, &native, &session)
        );
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachVulkanBorrowedTexture(
        MapHandle map,
        VulkanBorrowedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(
            NativeMethods.mln_vulkan_borrowed_texture_attach(map.Handle, &native, &session)
        );
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachOpenGLOwnedTexture(
        MapHandle map,
        OpenGLOwnedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(OpenGLOwnedTextureAttachNative(map.Handle, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachOpenGLBorrowedTexture(
        MapHandle map,
        OpenGLBorrowedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        MlnRenderSession session = default;
        NativeStatus.Check(OpenGLBorrowedTextureAttachNative(map.Handle, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    internal MlnRenderSession Handle => state.Handle;

    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Resizes this attached render session. Surface and owned-texture sessions resize in place.
    /// Borrowed texture targets are sized by their owner and throw an unsupported-feature error:
    /// allocate a texture at the new size and hand it over with the set-target method for the
    /// backend, such as <see cref="SetOpenGLBorrowedTextureTarget"/>, which keeps this session. The
    /// session keeps its renderer across a resize, so renderer-held state such as feature state
    /// carries over; a scale factor change is the exception, starting a new renderer with that state
    /// empty. Map state such as camera, style, and sources survives either way.
    /// </summary>
    public void Resize(uint width, uint height, double scaleFactor)
    {
        ThrowIfTextureFrameActive(nameof(Resize));
        NativeStatus.Check(ResizeNative(Handle, width, height, scaleFactor));
    }

    /// <summary>
    /// Presents this attached surface session through a new surface. A host surface can be destroyed
    /// and recreated while the map goes on living, which is what Android rotation, a Flutter surface
    /// producer lifecycle change, and a window resize that reallocates all look like from here.
    /// Replacing the surface in place keeps this session's renderer, and with it the tile pyramid,
    /// glyph and image atlases, symbol placement, and feature state. The descriptor names the same
    /// graphics context this session attached with, and its extent applies as <see cref="Resize"/>
    /// applies one. A descriptor whose context device is neither null nor this session's device
    /// throws an invalid-argument error and leaves this session rendering into the surface it has.
    /// The session assigns the layer its own device and pixel format, so the layer itself carries
    /// nothing that has to match.
    /// </summary>
    public void SetMetalSurfaceTarget(MetalSurfaceDescriptor descriptor)
    {
        ThrowIfTextureFrameActive(nameof(SetMetalSurfaceTarget));
        var native = RenderStructs.ToNative(descriptor);
        NativeStatus.Check(NativeMethods.mln_metal_surface_set_target(Handle, &native));
    }

    /// <summary>
    /// Presents this attached surface session through a new surface. See
    /// <see cref="SetMetalSurfaceTarget"/> for what replacing a surface preserves. The outgoing
    /// VkSurfaceKHR must still be valid: this session holds a swapchain built from it, and Vulkan
    /// destroys every swapchain before its surface. The replacement reports the color format and
    /// surface-transform support this session compiled a render pass and shaders for; one that does
    /// not throws an unsupported-feature error.
    /// </summary>
    public void SetVulkanSurfaceTarget(VulkanSurfaceDescriptor descriptor)
    {
        ThrowIfTextureFrameActive(nameof(SetVulkanSurfaceTarget));
        var native = RenderStructs.ToNative(descriptor);
        NativeStatus.Check(NativeMethods.mln_vulkan_surface_set_target(Handle, &native));
    }

    /// <summary>
    /// Presents this attached surface session through a new surface. See
    /// <see cref="SetMetalSurfaceTarget"/> for what replacing a surface preserves. The new surface is
    /// made current on the next render, so a host may hand over a replacement for one it has already
    /// destroyed. A surface accepted here can still prove unusable, which the next
    /// <see cref="RenderUpdate"/> reports rather than this call.
    /// </summary>
    public void SetOpenGLSurfaceTarget(OpenGLSurfaceDescriptor descriptor)
    {
        ThrowIfTextureFrameActive(nameof(SetOpenGLSurfaceTarget));
        var native = RenderStructs.ToNative(descriptor);
        NativeStatus.Check(NativeMethods.mln_opengl_surface_set_target(Handle, &native));
    }

    /// <summary>
    /// Renders this attached texture session into a new caller-owned texture. A caller-owned texture
    /// is sized by its owner, so a host that follows a resize reallocates rather than resizing and
    /// <see cref="Resize"/> throws an unsupported-feature error. Handing the replacement over here
    /// keeps this session's renderer instead, so the map does not go cold on every resize, unless
    /// the scale factor changes, which starts a new renderer for the new pixel ratio. The
    /// replacement belongs to the device this session attached with, which throws an
    /// invalid-argument error otherwise, and carries the pixel format it attached with, which throws
    /// an unsupported-feature error otherwise. Both leave this session rendering into the texture it
    /// has. The caller owns the replacement and keeps it valid until
    /// the next replacement, detach, or dispose. This session never retained the outgoing texture and
    /// never releases it, but reads from it during this call, so keep that texture valid until the
    /// call returns.
    /// </summary>
    public void SetMetalBorrowedTextureTarget(MetalBorrowedTextureDescriptor descriptor)
    {
        ThrowIfTextureFrameActive(nameof(SetMetalBorrowedTextureTarget));
        var native = RenderStructs.ToNative(descriptor);
        NativeStatus.Check(NativeMethods.mln_metal_borrowed_texture_set_target(Handle, &native));
    }

    /// <summary>
    /// Renders this attached texture session into a new caller-owned image. See
    /// <see cref="SetMetalBorrowedTextureTarget"/> for what replacing a target preserves. The
    /// replacement carries the format and both layouts this session attached with, since its render
    /// pass was built around them.
    /// </summary>
    public void SetVulkanBorrowedTextureTarget(VulkanBorrowedTextureDescriptor descriptor)
    {
        ThrowIfTextureFrameActive(nameof(SetVulkanBorrowedTextureTarget));
        var native = RenderStructs.ToNative(descriptor);
        NativeStatus.Check(NativeMethods.mln_vulkan_borrowed_texture_set_target(Handle, &native));
    }

    /// <summary>
    /// Renders this attached texture session into a new caller-owned texture. See
    /// <see cref="SetMetalBorrowedTextureTarget"/> for what replacing a target preserves. The
    /// replacement belongs to the context this session attached with, or one in its share group, and
    /// the host context must be current on this thread.
    /// </summary>
    public void SetOpenGLBorrowedTextureTarget(OpenGLBorrowedTextureDescriptor descriptor)
    {
        ThrowIfTextureFrameActive(nameof(SetOpenGLBorrowedTextureTarget));
        var native = RenderStructs.ToNative(descriptor);
        NativeStatus.Check(NativeMethods.mln_opengl_borrowed_texture_set_target(Handle, &native));
    }

    /// <summary>
    /// Renders the latest available map render update. The map retains its
    /// latest update, so repeated calls re-render it and return true again;
    /// use this to redraw on demand after resize or surface expose, and gate
    /// frame loops on render-update-available events instead of the return
    /// value. Returns false when no frame was rendered,
    /// because the map has not published an update yet or the renderer
    /// skipped the frame; both are normal during startup, so keep pumping
    /// the runtime until an update is reported.
    /// </summary>
    public bool RenderUpdate()
    {
        ThrowIfTextureFrameActive(nameof(RenderUpdate));
        var rendered = false;
        NativeStatus.Check(RenderUpdateNative(Handle, &rendered));
        return rendered;
    }

    public void Detach()
    {
        ThrowIfTextureFrameActive(nameof(Detach));
        NativeStatus.Check(NativeMethods.mln_render_session_detach(Handle));
    }

    public void ReduceMemoryUse()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_reduce_memory_use(Handle));
    }

    public void ClearData()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_clear_data(Handle));
    }

    public void DumpDebugLogs()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_dump_debug_logs(Handle));
    }

    public void SetFeatureState(FeatureStateSelector selector, JsonValue value)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        using var nativeValue = NativeJsonValue.From(value);
        var selectorValue = nativeSelector.Value;
        NativeStatus.Check(
            NativeMethods.mln_render_session_set_feature_state(
                Handle,
                &selectorValue,
                nativeValue.Pointer
            )
        );
    }

    public JsonValue GetFeatureState(FeatureStateSelector selector)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        var selectorValue = nativeSelector.Value;
        MlnJsonSnapshot snapshot = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_get_feature_state(Handle, &selectorValue, &snapshot)
        );
        return ValueStructs.ReadJsonSnapshot(snapshot) ?? new JsonValue.Object([]);
    }

    public void RemoveFeatureState(FeatureStateSelector selector)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        var selectorValue = nativeSelector.Value;
        NativeStatus.Check(
            NativeMethods.mln_render_session_remove_feature_state(Handle, &selectorValue)
        );
    }

    public IReadOnlyList<QueriedFeature> QueryRenderedFeatures(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options
    ) => QueryRenderedFeaturesCore(geometry, options);

    public IReadOnlyList<QueriedFeature> QuerySourceFeatures(
        string sourceId,
        SourceFeatureQueryOptions? options
    ) => QuerySourceFeaturesCore(sourceId, options);

    /// <summary>
    /// Queries a feature extension from the latest render session state.
    /// </summary>
    /// <remarks>
    /// The <c>supercluster</c> extension reads the <c>cluster_id</c> feature
    /// property and the <c>limit</c> and <c>offset</c> arguments as
    /// <c>JsonValue.UInt</c>. Other numeric types are treated as absent: a
    /// <c>cluster_id</c> that is not <c>JsonValue.UInt</c> returns a
    /// <c>FeatureExtensionResult.Value</c> holding <c>JsonValue.Null</c>
    /// instead of a feature collection, and a <c>limit</c> or <c>offset</c>
    /// that is not <c>JsonValue.UInt</c> leaves <c>leaves</c> at the native
    /// defaults of ten leaves at offset zero. Queried feature properties keep
    /// their JSON value type, so a queried cluster feature can be passed back
    /// unmodified.
    /// </remarks>
    public FeatureExtensionResult QueryFeatureExtension(
        string sourceId,
        Feature feature,
        string extension,
        string extensionField,
        JsonValue? arguments
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeFeature = NativeFeature.From(feature);
        using var nativeExtension = NativeStringView.From(extension, nameof(extension));
        using var nativeExtensionField = NativeStringView.From(
            extensionField,
            nameof(extensionField)
        );
        using var nativeArguments = arguments is null ? null : NativeJsonValue.From(arguments);
        var featureValue = nativeFeature.Value;
        MlnFeatureExtensionResult result = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_query_feature_extensions(
                Handle,
                nativeSourceId.Value,
                &featureValue,
                nativeExtension.Value,
                nativeExtensionField.Value,
                nativeArguments?.Pointer,
                &result
            )
        );
        return QueryStructs.ReadFeatureExtensionResult(result);
    }

    public TextureImageInfo TextureImageInfo()
    {
        ThrowIfTextureFrameActive(nameof(TextureImageInfo));
        var info = new mln_texture_image_info { size = (uint)sizeof(mln_texture_image_info) };
        var status = TextureReadNative(Handle, null, 0, &info);
        var copied = RenderStructs.FromNative(info);
        if (
            status == mln_status.MLN_STATUS_OK
            || (status == mln_status.MLN_STATUS_INVALID_ARGUMENT && copied.ByteLength > 0)
        )
        {
            return copied;
        }

        NativeStatus.Check(status);
        throw new InvalidOperationException("Unreachable native texture status.");
    }

    public TextureImageInfo ReadPremultipliedRgba8(NativeBuffer buffer)
    {
        ArgumentNullException.ThrowIfNull(buffer);
        ThrowIfTextureFrameActive(nameof(ReadPremultipliedRgba8));
        var info = new mln_texture_image_info { size = (uint)sizeof(mln_texture_image_info) };
        fixed (byte* data = buffer.Span)
        {
            NativeStatus.Check(
                TextureReadNative(
                    Handle,
                    buffer.ByteLength == 0 ? null : data,
                    (nuint)buffer.ByteLength,
                    &info
                )
            );
        }
        // An empty destination reaches native code as the null pointer and zero capacity that
        // mean a size probe, which succeeds without copying. Report the buffer as too small
        // unless the frame really carries no bytes.
        if (buffer.ByteLength == 0 && info.byte_length > 0)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"Buffer length 0 is smaller than the required {info.byte_length} bytes.",
                null
            );
        }
        return RenderStructs.FromNative(info);
    }

    public MetalOwnedTextureFrameHandle AcquireMetalOwnedTextureFrame()
    {
        ReserveActiveTextureFrame();
        mln_metal_owned_texture_frame* pointer = null;
        var acquired = false;
        var reservationHeld = true;
        FrameScope? scope = null;
        try
        {
            pointer = (mln_metal_owned_texture_frame*)
                NativeMemory.AllocZeroed((nuint)sizeof(mln_metal_owned_texture_frame));
            pointer->size = (uint)sizeof(mln_metal_owned_texture_frame);
            NativeStatus.Check(AcquireMetalFrameNative(Handle, pointer));
            acquired = true;
            scope = new FrameScope(nameof(MetalOwnedTextureFrame));
            var frame = ReadMetalFrame(*pointer, scope);
            var handle = new MetalOwnedTextureFrameHandle(this, pointer, scope, frame, true);
            reservationHeld = false;
            return handle;
        }
        catch
        {
            if (acquired)
            {
                ReleaseAcquiredFrameAfterConstructionFailure(
                    pointer,
                    static (session, frame) => session.ReleaseMetalFrame(frame),
                    nameof(MetalOwnedTextureFrameHandle)
                );
            }
            scope?.Dispose();
            if (pointer is not null)
            {
                NativeMemory.Free(pointer);
            }
            if (reservationHeld)
            {
                UnregisterActiveTextureFrame();
            }
            throw;
        }
    }

    public VulkanOwnedTextureFrameHandle AcquireVulkanOwnedTextureFrame()
    {
        ReserveActiveTextureFrame();
        mln_vulkan_owned_texture_frame* pointer = null;
        var acquired = false;
        var reservationHeld = true;
        FrameScope? scope = null;
        try
        {
            pointer = (mln_vulkan_owned_texture_frame*)
                NativeMemory.AllocZeroed((nuint)sizeof(mln_vulkan_owned_texture_frame));
            pointer->size = (uint)sizeof(mln_vulkan_owned_texture_frame);
            NativeStatus.Check(AcquireVulkanFrameNative(Handle, pointer));
            acquired = true;
            scope = new FrameScope(nameof(VulkanOwnedTextureFrame));
            var frame = RenderStructs.FromNative(*pointer, scope);
            var handle = new VulkanOwnedTextureFrameHandle(this, pointer, scope, frame, true);
            reservationHeld = false;
            return handle;
        }
        catch
        {
            if (acquired)
            {
                ReleaseAcquiredFrameAfterConstructionFailure(
                    pointer,
                    static (session, frame) => session.ReleaseVulkanFrame(frame),
                    nameof(VulkanOwnedTextureFrameHandle)
                );
            }
            scope?.Dispose();
            if (pointer is not null)
            {
                NativeMemory.Free(pointer);
            }
            if (reservationHeld)
            {
                UnregisterActiveTextureFrame();
            }
            throw;
        }
    }

    public OpenGLOwnedTextureFrameHandle AcquireOpenGLOwnedTextureFrame()
    {
        ReserveActiveTextureFrame();
        mln_opengl_owned_texture_frame* pointer = null;
        var acquired = false;
        var reservationHeld = true;
        FrameScope? scope = null;
        try
        {
            pointer = (mln_opengl_owned_texture_frame*)
                NativeMemory.AllocZeroed((nuint)sizeof(mln_opengl_owned_texture_frame));
            pointer->size = (uint)sizeof(mln_opengl_owned_texture_frame);
            NativeStatus.Check(AcquireOpenGLFrameNative(Handle, pointer));
            acquired = true;
            scope = new FrameScope(nameof(OpenGLOwnedTextureFrame));
            var frame = RenderStructs.FromNative(*pointer, scope);
            var handle = new OpenGLOwnedTextureFrameHandle(this, pointer, scope, frame, true);
            reservationHeld = false;
            return handle;
        }
        catch
        {
            if (acquired)
            {
                ReleaseAcquiredFrameAfterConstructionFailure(
                    pointer,
                    static (session, frame) => session.ReleaseOpenGLFrame(frame),
                    nameof(OpenGLOwnedTextureFrameHandle)
                );
            }
            scope?.Dispose();
            if (pointer is not null)
            {
                NativeMemory.Free(pointer);
            }
            if (reservationHeld)
            {
                UnregisterActiveTextureFrame();
            }
            throw;
        }
    }

    internal mln_status ReleaseMetalFrame(mln_metal_owned_texture_frame* frame) =>
        ReleaseMetalFrameNative(Handle, frame);

    internal mln_status ReleaseVulkanFrame(mln_vulkan_owned_texture_frame* frame) =>
        NativeMethods.mln_vulkan_owned_texture_release_frame(Handle, frame);

    internal mln_status ReleaseOpenGLFrame(mln_opengl_owned_texture_frame* frame) =>
        NativeMethods.mln_opengl_owned_texture_release_frame(Handle, frame);

    private void ReleaseAcquiredFrameAfterConstructionFailure<T>(
        T* pointer,
        FrameRelease<T> release,
        string typeName
    )
        where T : unmanaged
    {
        try
        {
            var status = release(this, pointer);
            if (status == mln_status.MLN_STATUS_OK)
            {
                return;
            }

            NativeLeakReporter.Report(
                new NativeLeakReport(
                    NativeLeakReportKind.DisposeFailed,
                    typeName,
                    0,
                    status,
                    $"Construction failed after acquiring {typeName} frame 0x{(nint)pointer:x}; cleanup returned {status}."
                )
            );
        }
        catch (Exception error)
        {
            NativeLeakReporter.Report(
                new NativeLeakReport(
                    NativeLeakReportKind.DisposeFailed,
                    typeName,
                    0,
                    null,
                    $"Construction failed after acquiring {typeName} frame 0x{(nint)pointer:x}; cleanup threw {error.GetType().Name}: {error.Message}"
                )
            );
        }
    }

    private static OpenGLSurfaceAttach OpenGLSurfaceAttachNative =>
        openGLSurfaceAttachForTest ?? DefaultOpenGLSurfaceAttach;

    private static OpenGLOwnedTextureAttach OpenGLOwnedTextureAttachNative =>
        openGLOwnedTextureAttachForTest ?? DefaultOpenGLOwnedTextureAttach;

    private static OpenGLBorrowedTextureAttach OpenGLBorrowedTextureAttachNative =>
        openGLBorrowedTextureAttachForTest ?? DefaultOpenGLBorrowedTextureAttach;

    private static RenderSessionResize ResizeNative => resizeForTest ?? DefaultResize;

    private static RenderSessionRenderUpdate RenderUpdateNative =>
        renderUpdateForTest ?? DefaultRenderUpdate;

    private static TextureRead TextureReadNative => textureReadForTest ?? DefaultTextureRead;

    private static mln_status DestroyNative(MlnRenderSession session) =>
        (destroyForTest ?? DefaultDestroy)(session);

    private static MetalOwnedTextureAcquireFrame AcquireMetalFrameNative =>
        acquireMetalFrameForTest ?? DefaultAcquireMetalFrame;

    private static MetalOwnedTextureReleaseFrame ReleaseMetalFrameNative =>
        releaseMetalFrameForTest ?? DefaultReleaseMetalFrame;

    private static VulkanOwnedTextureAcquireFrame AcquireVulkanFrameNative =>
        acquireVulkanFrameForTest ?? DefaultAcquireVulkanFrame;

    private static OpenGLOwnedTextureAcquireFrame AcquireOpenGLFrameNative =>
        acquireOpenGLFrameForTest ?? DefaultAcquireOpenGLFrame;

    private static MetalOwnedTextureFrame ReadMetalFrame(
        mln_metal_owned_texture_frame frame,
        FrameScope scope
    ) =>
        readMetalFrameForTest is { } reader
            ? reader(frame, scope)
            : RenderStructs.FromNative(frame, scope);

    private sealed class RestoreOpenGLAttachMethods(
        OpenGLSurfaceAttach? previousSurface,
        OpenGLOwnedTextureAttach? previousOwnedTexture,
        OpenGLBorrowedTextureAttach? previousBorrowedTexture
    ) : IDisposable
    {
        public void Dispose()
        {
            openGLSurfaceAttachForTest = previousSurface;
            openGLOwnedTextureAttachForTest = previousOwnedTexture;
            openGLBorrowedTextureAttachForTest = previousBorrowedTexture;
        }
    }

    private sealed class RestoreSessionMethods(
        RenderSessionResize? previousResize,
        RenderSessionRenderUpdate? previousRenderUpdate,
        TextureRead? previousTextureRead,
        StatusDestroy<MlnRenderSession>? previousDestroy
    ) : IDisposable
    {
        public void Dispose()
        {
            resizeForTest = previousResize;
            renderUpdateForTest = previousRenderUpdate;
            textureReadForTest = previousTextureRead;
            destroyForTest = previousDestroy;
        }
    }

    private sealed class RestoreMetalFrameMethods(
        MetalOwnedTextureAcquireFrame? previousAcquire,
        MetalOwnedTextureReleaseFrame? previousRelease,
        Func<mln_metal_owned_texture_frame, FrameScope, MetalOwnedTextureFrame>? previousRead
    ) : IDisposable
    {
        public void Dispose()
        {
            acquireMetalFrameForTest = previousAcquire;
            releaseMetalFrameForTest = previousRelease;
            readMetalFrameForTest = previousRead;
        }
    }

    private sealed class RestoreTextureFrameAcquireMethods(
        VulkanOwnedTextureAcquireFrame? previousVulkan,
        OpenGLOwnedTextureAcquireFrame? previousOpenGL
    ) : IDisposable
    {
        public void Dispose()
        {
            acquireVulkanFrameForTest = previousVulkan;
            acquireOpenGLFrameForTest = previousOpenGL;
        }
    }

    private IReadOnlyList<QueriedFeature> QueryRenderedFeaturesCore(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options
    )
    {
        using var nativeGeometry = NativeRenderedQueryGeometry.From(geometry);
        using var nativeOptions = options is null
            ? null
            : NativeRenderedFeatureQueryOptions.From(options);
        var geometryValue = nativeGeometry.Value;
        MlnFeatureQueryResult result = default;
        if (nativeOptions is null)
        {
            NativeStatus.Check(
                NativeMethods.mln_render_session_query_rendered_features(
                    Handle,
                    &geometryValue,
                    null,
                    &result
                )
            );
        }
        else
        {
            var optionsValue = nativeOptions.Value;
            NativeStatus.Check(
                NativeMethods.mln_render_session_query_rendered_features(
                    Handle,
                    &geometryValue,
                    &optionsValue,
                    &result
                )
            );
        }
        return QueryStructs.ReadFeatureQueryResult(result);
    }

    private IReadOnlyList<QueriedFeature> QuerySourceFeaturesCore(
        string sourceId,
        SourceFeatureQueryOptions? options
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeOptions = options is null
            ? null
            : NativeSourceFeatureQueryOptions.From(options);
        MlnFeatureQueryResult result = default;
        if (nativeOptions is null)
        {
            NativeStatus.Check(
                NativeMethods.mln_render_session_query_source_features(
                    Handle,
                    nativeSourceId.Value,
                    null,
                    &result
                )
            );
        }
        else
        {
            var optionsValue = nativeOptions.Value;
            NativeStatus.Check(
                NativeMethods.mln_render_session_query_source_features(
                    Handle,
                    nativeSourceId.Value,
                    &optionsValue,
                    &result
                )
            );
        }
        return QueryStructs.ReadFeatureQueryResult(result);
    }

    /// <summary>Destroys the render session on the map owner thread.</summary>
    public void Close()
    {
        ThrowIfTextureFrameActive(nameof(Close));
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        if (IsTextureFrameActive())
        {
            ReportDisposeWithActiveTextureFrame();
            GC.KeepAlive(map);
            return;
        }

        state.TryClose();
        GC.KeepAlive(map);
    }

    private bool IsTextureFrameActive()
    {
        lock (frameGate)
        {
            return hasActiveTextureFrame;
        }
    }

    private void ThrowIfTextureFrameActive(string operation)
    {
        if (!IsTextureFrameActive())
        {
            return;
        }

        throw new InvalidStateException(
            MaplibreStatus.InvalidState,
            null,
            $"{operation} cannot run while a texture frame is active.",
            null
        );
    }

    private void ReportDisposeWithActiveTextureFrame()
    {
        NativeLeakReporter.Report(
            new NativeLeakReport(
                NativeLeakReportKind.DisposeFailed,
                nameof(RenderSessionHandle),
                Handle.Value,
                null,
                "Dispose could not close RenderSessionHandle while a texture frame is active. Release the frame on the owner thread, then call Close() to observe errors and retry."
            )
        );
    }

    internal void ReserveActiveTextureFrame()
    {
        lock (frameGate)
        {
            if (hasActiveTextureFrame)
            {
                throw new InvalidStateException(
                    MaplibreStatus.InvalidState,
                    null,
                    "A texture frame is active already.",
                    null
                );
            }

            hasActiveTextureFrame = true;
        }
    }

    internal void UnregisterActiveTextureFrame()
    {
        lock (frameGate)
        {
            hasActiveTextureFrame = false;
        }
    }
}

internal unsafe delegate mln_status FrameRelease<T>(RenderSessionHandle session, T* frame)
    where T : unmanaged;
internal unsafe delegate mln_status TextureRead(
    MlnRenderSession session,
    byte* data,
    nuint length,
    mln_texture_image_info* info
);
internal unsafe delegate mln_status VulkanOwnedTextureAcquireFrame(
    MlnRenderSession session,
    mln_vulkan_owned_texture_frame* frame
);
internal unsafe delegate mln_status OpenGLOwnedTextureAcquireFrame(
    MlnRenderSession session,
    mln_opengl_owned_texture_frame* frame
);

internal sealed unsafe class TextureFrameState<T>
    where T : unmanaged
{
    private readonly RenderSessionHandle session;
    private readonly FrameScope scope;
    private readonly FrameRelease<T> release;
    private readonly string typeName;
    private T* pointer;

    internal TextureFrameState(
        RenderSessionHandle session,
        T* pointer,
        FrameScope scope,
        FrameRelease<T> release,
        string typeName,
        bool activeFrameReserved = false
    )
    {
        this.session = session;
        this.pointer = pointer;
        this.scope = scope;
        this.release = release;
        this.typeName = typeName;
        if (!activeFrameReserved)
        {
            session.ReserveActiveTextureFrame();
        }
    }

    internal bool IsClosed => pointer is null;

    internal void Close()
    {
        if (pointer is null)
        {
            return;
        }

        NativeStatus.Check(release(session, pointer));
        MarkClosed();
    }

    internal void TryClose()
    {
        if (pointer is null)
        {
            return;
        }

        if (session.IsClosed)
        {
            ReportParentClosed();
            MarkClosed();
            return;
        }

        mln_status status;
        try
        {
            status = release(session, pointer);
        }
        catch (MaplibreException) when (session.IsClosed)
        {
            ReportParentClosed();
            MarkClosed();
            return;
        }

        if (status != mln_status.MLN_STATUS_OK)
        {
            NativeLeakReporter.Report(
                new NativeLeakReport(
                    NativeLeakReportKind.DisposeFailed,
                    typeName,
                    0,
                    status,
                    $"Dispose could not release {typeName} frame 0x{(nint)pointer:x}; native release returned {status}. Call Close() on the owner thread to observe the error and retry."
                )
            );
            return;
        }

        MarkClosed();
    }

    private void ReportParentClosed()
    {
        NativeLeakReporter.Report(
            new NativeLeakReport(
                NativeLeakReportKind.DisposeFailed,
                typeName,
                0,
                null,
                $"Dispose could not release {typeName} frame 0x{(nint)pointer:x}; the parent RenderSessionHandle is already closed."
            )
        );
    }

    private void MarkClosed()
    {
        var current = pointer;
        pointer = null;
        scope.Dispose();
        NativeMemory.Free(current);
        session.UnregisterActiveTextureFrame();
    }
}

public sealed unsafe class MetalOwnedTextureFrameHandle : IDisposable
{
    private readonly TextureFrameState<mln_metal_owned_texture_frame> state;

    internal MetalOwnedTextureFrameHandle(
        RenderSessionHandle session,
        mln_metal_owned_texture_frame* pointer,
        FrameScope scope,
        MetalOwnedTextureFrame frame,
        bool activeFrameReserved = false
    )
    {
        state = new TextureFrameState<mln_metal_owned_texture_frame>(
            session,
            pointer,
            scope,
            static (session, frame) => session.ReleaseMetalFrame(frame),
            nameof(MetalOwnedTextureFrameHandle),
            activeFrameReserved
        );
        Frame = frame;
    }

    public bool IsClosed => state.IsClosed;

    public MetalOwnedTextureFrame Frame { get; }

    public void Close() => state.Close();

    public void Dispose() => state.TryClose();
}

public sealed unsafe class VulkanOwnedTextureFrameHandle : IDisposable
{
    private readonly TextureFrameState<mln_vulkan_owned_texture_frame> state;

    internal VulkanOwnedTextureFrameHandle(
        RenderSessionHandle session,
        mln_vulkan_owned_texture_frame* pointer,
        FrameScope scope,
        VulkanOwnedTextureFrame frame,
        bool activeFrameReserved = false
    )
    {
        state = new TextureFrameState<mln_vulkan_owned_texture_frame>(
            session,
            pointer,
            scope,
            static (session, frame) => session.ReleaseVulkanFrame(frame),
            nameof(VulkanOwnedTextureFrameHandle),
            activeFrameReserved
        );
        Frame = frame;
    }

    public bool IsClosed => state.IsClosed;

    public VulkanOwnedTextureFrame Frame { get; }

    public void Close() => state.Close();

    public void Dispose() => state.TryClose();
}

public sealed unsafe class OpenGLOwnedTextureFrameHandle : IDisposable
{
    private readonly TextureFrameState<mln_opengl_owned_texture_frame> state;

    internal OpenGLOwnedTextureFrameHandle(
        RenderSessionHandle session,
        mln_opengl_owned_texture_frame* pointer,
        FrameScope scope,
        OpenGLOwnedTextureFrame frame,
        bool activeFrameReserved = false
    )
    {
        state = new TextureFrameState<mln_opengl_owned_texture_frame>(
            session,
            pointer,
            scope,
            static (session, frame) => session.ReleaseOpenGLFrame(frame),
            nameof(OpenGLOwnedTextureFrameHandle),
            activeFrameReserved
        );
        Frame = frame;
    }

    public bool IsClosed => state.IsClosed;

    public OpenGLOwnedTextureFrame Frame { get; }

    public void Close() => state.Close();

    public void Dispose() => state.TryClose();
}

internal sealed class FrameScope : IDisposable
{
    private readonly string owner;

    internal FrameScope(string owner)
    {
        this.owner = owner;
    }

    public bool IsClosed { get; private set; }

    internal void EnsureActive()
    {
        if (IsClosed)
        {
            throw new InvalidStateException(
                MaplibreStatus.InvalidState,
                null,
                $"{owner} is closed.",
                null
            );
        }
    }

    public void Dispose()
    {
        IsClosed = true;
    }
}
