using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;

namespace Maplibre.NativeFfi.Render;

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
    mln_render_result* out_result,
    bool* out_needs_repaint
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
        outResult,
        outNeedsRepaint
    ) => NativeMethods.mln_render_session_render_update(session, outResult, outNeedsRepaint);
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
    /// Resizes this attached render session. Surface and owned-texture sessions resize in place;
    /// a borrowed texture target throws an unsupported-feature error, since its owner sizes it —
    /// hand over a new texture with the backend's set-target method instead. A resize keeps the
    /// session's renderer along with the tile pyramid, glyph and image atlases, and symbol
    /// placement. A scale factor change retires the renderer instead, because shaders are compiled
    /// for one pixel ratio. Map-owned feature state survives either way.
    /// </summary>
    public void Resize(uint width, uint height, double scaleFactor)
    {
        ThrowIfTextureFrameActive(nameof(Resize));
        NativeStatus.Check(ResizeNative(Handle, width, height, scaleFactor));
    }

    /// <summary>
    /// Presents this attached surface session through a new surface, keeping the session's renderer
    /// and its cached state. The descriptor must name the same graphics context this session
    /// attached with; one whose context device is neither null nor this session's device throws an
    /// invalid-argument error and leaves this session rendering into the surface it has. The
    /// descriptor's extent applies as <see cref="Resize"/> applies one.
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
    /// VkSurfaceKHR must still be valid, since this session holds a swapchain built from it. The
    /// replacement must report the color format and surface-transform support this session
    /// compiled a render pass and shaders for, or it throws an unsupported-feature error.
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
    /// destroyed, and an unusable surface is reported by the next <see cref="RenderUpdate"/>.
    /// </summary>
    public void SetOpenGLSurfaceTarget(OpenGLSurfaceDescriptor descriptor)
    {
        ThrowIfTextureFrameActive(nameof(SetOpenGLSurfaceTarget));
        var native = RenderStructs.ToNative(descriptor);
        NativeStatus.Check(NativeMethods.mln_opengl_surface_set_target(Handle, &native));
    }

    /// <summary>
    /// Renders this attached texture session into a new caller-owned texture, keeping the session's
    /// renderer unless the scale factor changes, which starts a new renderer. The replacement must
    /// belong to the device this session attached with, which throws an invalid-argument error
    /// otherwise, and carry the pixel format it attached with, which throws an unsupported-feature
    /// error otherwise; both leave this session rendering into the texture it has. The caller owns
    /// the replacement and keeps it valid until the next replacement, detach, or dispose. The
    /// outgoing texture is neither read nor released here.
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
    /// replacement must carry the format and both layouts this session attached with, since its
    /// render pass was built around them.
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
    /// Renders the latest available map render update into this session's render target. The map
    /// retains its latest update, so repeated calls re-render it and report
    /// <see cref="RenderResult.Rendered"/> again. Every other result names the wake to wait for:
    /// <see cref="RenderResult.NoUpdate"/> and <see cref="RenderResult.SizePending"/> resolve on a
    /// render-update-available event, and <see cref="RenderResult.TargetNotReady"/> resolves when
    /// the host changes the render target. The returned <see cref="RenderUpdate.NeedsRepaint"/>
    /// flag tells whether the map asked for another frame while it rendered this one.
    /// </summary>
    public RenderUpdate RenderUpdate()
    {
        ThrowIfTextureFrameActive(nameof(RenderUpdate));
        var result = mln_render_result.MLN_RENDER_RESULT_NO_UPDATE;
        var needsRepaint = false;
        NativeStatus.Check(RenderUpdateNative(Handle, &result, &needsRepaint));
        return new RenderUpdate((RenderResult)result, needsRepaint);
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

    public QueriedFeature[] QueryRenderedFeatures(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options
    ) => QueryRenderedFeaturesCore(geometry, options);

    public QueriedFeature[] QuerySourceFeatures(
        string sourceId,
        SourceFeatureQueryOptions? options
    ) => QuerySourceFeaturesCore(sourceId, options);

    /// <summary>
    /// Queries a feature extension from the latest render session state.
    /// </summary>
    public byte[] QueryFeatureExtension(
        string sourceId,
        byte[] feature,
        string extension,
        string extensionField,
        byte[]? arguments
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeFeature = NativeStringView.From(feature, nameof(feature));
        using var nativeExtension = NativeStringView.From(extension, nameof(extension));
        using var nativeExtensionField = NativeStringView.From(
            extensionField,
            nameof(extensionField)
        );
        using var nativeArguments = arguments is null
            ? null
            : NativeStringView.From(arguments, nameof(arguments));
        MlnBuffer result = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_query_feature_extensions(
                Handle,
                nativeSourceId.Value,
                nativeFeature.Value,
                nativeExtension.Value,
                nativeExtensionField.Value,
                nativeArguments?.Pointer,
                &result
            )
        );
        return ValueStructs.ReadBuffer(result);
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
        // An empty destination reaches native code as a size probe, which succeeds without
        // copying, so report the buffer as too small unless the frame carries no bytes.
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

    private QueriedFeature[] QueryRenderedFeaturesCore(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options
    )
    {
        using var nativeGeometry = NativeRenderedQueryGeometry.From(geometry);
        using var nativeOptions = options is null
            ? null
            : NativeRenderedFeatureQueryOptions.From(options);
        var geometryValue = nativeGeometry.Value;
        MlnQueriedFeatureList result = default;
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
        return CopyQueriedFeatureList(result);
    }

    private QueriedFeature[] QuerySourceFeaturesCore(
        string sourceId,
        SourceFeatureQueryOptions? options
    )
    {
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeOptions = options is null
            ? null
            : NativeSourceFeatureQueryOptions.From(options);
        MlnQueriedFeatureList result = default;
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
        return CopyQueriedFeatureList(result);
    }

    private static QueriedFeature[] CopyQueriedFeatureList(MlnQueriedFeatureList list)
    {
        if (list.IsNull)
        {
            return [];
        }

        try
        {
            nuint count = 0;
            NativeStatus.Check(NativeMethods.mln_queried_feature_list_count(list, &count));
            var features = new QueriedFeature[checked((int)count)];
            for (var index = 0; index < features.Length; index++)
            {
                var native = NativeMethods.mln_queried_feature_default();
                NativeStatus.Check(
                    NativeMethods.mln_queried_feature_list_get(list, (nuint)index, &native)
                );
                var fields = (mln_queried_feature_field)native.fields;
                features[index] = new QueriedFeature(
                    ValueStructs.CopyBufferView(native.feature),
                    fields.HasFlag(mln_queried_feature_field.MLN_QUERIED_FEATURE_SOURCE_ID)
                        ? RuntimeStructs.CopyUtf8(native.source_id.data, native.source_id.size)
                        : null,
                    fields.HasFlag(mln_queried_feature_field.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID)
                        ? RuntimeStructs.CopyUtf8(
                            native.source_layer_id.data,
                            native.source_layer_id.size
                        )
                        : null,
                    fields.HasFlag(mln_queried_feature_field.MLN_QUERIED_FEATURE_STATE)
                        ? ValueStructs.CopyBufferView(native.state)
                        : null
                );
            }

            return features;
        }
        finally
        {
            NativeMethods.mln_queried_feature_list_destroy(list);
        }
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
