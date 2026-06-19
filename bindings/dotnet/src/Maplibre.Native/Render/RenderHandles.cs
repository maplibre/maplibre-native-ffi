using System.Runtime.InteropServices;
using Maplibre.Native.Error;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Map;
using Maplibre.Native.Query;

namespace Maplibre.Native.Render;

internal unsafe delegate mln_status OpenGLSurfaceAttach(
    mln_map* map,
    mln_opengl_surface_descriptor* descriptor,
    mln_render_session** outSession
);

internal unsafe delegate mln_status OpenGLOwnedTextureAttach(
    mln_map* map,
    mln_opengl_owned_texture_descriptor* descriptor,
    mln_render_session** outSession
);

internal unsafe delegate mln_status OpenGLBorrowedTextureAttach(
    mln_map* map,
    mln_opengl_borrowed_texture_descriptor* descriptor,
    mln_render_session** outSession
);

internal unsafe delegate mln_status RenderSessionResize(
    mln_render_session* session,
    uint width,
    uint height,
    double scaleFactor
);

internal unsafe delegate mln_status RenderSessionRenderUpdate(mln_render_session* session);

internal unsafe delegate mln_status MetalOwnedTextureAcquireFrame(
    mln_render_session* session,
    mln_metal_owned_texture_frame* frame
);

internal unsafe delegate mln_status MetalOwnedTextureReleaseFrame(
    mln_render_session* session,
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
    private static readonly RenderSessionRenderUpdate DefaultRenderUpdate = static session =>
        NativeMethods.mln_render_session_render_update(session);
    private static readonly TextureRead DefaultTextureRead = static (session, data, length, info) =>
        NativeMethods.mln_texture_read_premultiplied_rgba8(session, data, length, info);
    private static readonly StatusDestroy<mln_render_session> DefaultDestroy = static session =>
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
    private static StatusDestroy<mln_render_session>? destroyForTest;

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
    private readonly NativeHandleState<mln_render_session> state;
    private bool hasActiveTextureFrame;

    private RenderSessionHandle(MapHandle? map, mln_render_session* handle)
        : this(map, handle, DestroyNative) { }

    private RenderSessionHandle(
        MapHandle? map,
        mln_render_session* handle,
        StatusDestroy<mln_render_session> destroy
    )
    {
        this.map = map;
        state = new NativeHandleState<mln_render_session>(
            handle,
            destroy,
            nameof(RenderSessionHandle)
        );
    }

    internal static RenderSessionHandle CreateForTest(mln_render_session* handle) =>
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
        StatusDestroy<mln_render_session> destroy
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
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_metal_surface_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachVulkanSurface(
        MapHandle map,
        VulkanSurfaceDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_vulkan_surface_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachOpenGLSurface(
        MapHandle map,
        OpenGLSurfaceDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(OpenGLSurfaceAttachNative(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachMetalOwnedTexture(
        MapHandle map,
        MetalOwnedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(
            NativeMethods.mln_metal_owned_texture_attach(map.Pointer, &native, &session)
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
        mln_render_session* session = null;
        NativeStatus.Check(
            NativeMethods.mln_metal_borrowed_texture_attach(map.Pointer, &native, &session)
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
        mln_render_session* session = null;
        NativeStatus.Check(
            NativeMethods.mln_vulkan_owned_texture_attach(map.Pointer, &native, &session)
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
        mln_render_session* session = null;
        NativeStatus.Check(
            NativeMethods.mln_vulkan_borrowed_texture_attach(map.Pointer, &native, &session)
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
        mln_render_session* session = null;
        NativeStatus.Check(OpenGLOwnedTextureAttachNative(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    public static RenderSessionHandle AttachOpenGLBorrowedTexture(
        MapHandle map,
        OpenGLBorrowedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(OpenGLBorrowedTextureAttachNative(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    internal mln_render_session* Pointer => state.Pointer;

    public bool IsClosed => state.IsClosed;

    public void Resize(uint width, uint height, double scaleFactor)
    {
        ThrowIfTextureFrameActive(nameof(Resize));
        if (!double.IsFinite(scaleFactor) || scaleFactor <= 0)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                "Render target scale factor must be positive and finite.",
                null
            );
        }

        NativeStatus.Check(ResizeNative(Pointer, width, height, scaleFactor));
    }

    public void RenderUpdate()
    {
        ThrowIfTextureFrameActive(nameof(RenderUpdate));
        NativeStatus.Check(RenderUpdateNative(Pointer));
    }

    public void Detach()
    {
        ThrowIfTextureFrameActive(nameof(Detach));
        NativeStatus.Check(NativeMethods.mln_render_session_detach(Pointer));
    }

    public void ReduceMemoryUse()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_reduce_memory_use(Pointer));
    }

    public void ClearData()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_clear_data(Pointer));
    }

    public void DumpDebugLogs()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_dump_debug_logs(Pointer));
    }

    public void SetFeatureState(FeatureStateSelector selector, JsonValue value)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        using var nativeValue = NativeJsonValue.From(value);
        var selectorValue = nativeSelector.Value;
        NativeStatus.Check(
            NativeMethods.mln_render_session_set_feature_state(
                Pointer,
                &selectorValue,
                nativeValue.Pointer
            )
        );
    }

    public JsonValue GetFeatureState(FeatureStateSelector selector)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        var selectorValue = nativeSelector.Value;
        mln_json_snapshot* snapshot = null;
        NativeStatus.Check(
            NativeMethods.mln_render_session_get_feature_state(Pointer, &selectorValue, &snapshot)
        );
        return ValueStructs.ReadJsonSnapshot(snapshot) ?? new JsonValue.Object([]);
    }

    public void RemoveFeatureState(FeatureStateSelector selector)
    {
        using var nativeSelector = NativeFeatureStateSelector.From(selector);
        var selectorValue = nativeSelector.Value;
        NativeStatus.Check(
            NativeMethods.mln_render_session_remove_feature_state(Pointer, &selectorValue)
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
        mln_feature_extension_result* result = null;
        NativeStatus.Check(
            NativeMethods.mln_render_session_query_feature_extensions(
                Pointer,
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
        var status = TextureReadNative(Pointer, null, 0, &info);
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
                    Pointer,
                    buffer.ByteLength == 0 ? null : data,
                    (nuint)buffer.ByteLength,
                    &info
                )
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
            NativeStatus.Check(AcquireMetalFrameNative(Pointer, pointer));
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
            NativeStatus.Check(AcquireVulkanFrameNative(Pointer, pointer));
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
            NativeStatus.Check(AcquireOpenGLFrameNative(Pointer, pointer));
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
        ReleaseMetalFrameNative(Pointer, frame);

    internal mln_status ReleaseVulkanFrame(mln_vulkan_owned_texture_frame* frame) =>
        NativeMethods.mln_vulkan_owned_texture_release_frame(Pointer, frame);

    internal mln_status ReleaseOpenGLFrame(mln_opengl_owned_texture_frame* frame) =>
        NativeMethods.mln_opengl_owned_texture_release_frame(Pointer, frame);

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
                    (nint)pointer,
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
                    (nint)pointer,
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

    private static mln_status DestroyNative(mln_render_session* session) =>
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
        StatusDestroy<mln_render_session>? previousDestroy
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
        mln_feature_query_result* result = null;
        if (nativeOptions is null)
        {
            NativeStatus.Check(
                NativeMethods.mln_render_session_query_rendered_features(
                    Pointer,
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
                    Pointer,
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
        mln_feature_query_result* result = null;
        if (nativeOptions is null)
        {
            NativeStatus.Check(
                NativeMethods.mln_render_session_query_source_features(
                    Pointer,
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
                    Pointer,
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
                (nint)Pointer,
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
    mln_render_session* session,
    byte* data,
    nuint length,
    mln_texture_image_info* info
);
internal unsafe delegate mln_status VulkanOwnedTextureAcquireFrame(
    mln_render_session* session,
    mln_vulkan_owned_texture_frame* frame
);
internal unsafe delegate mln_status OpenGLOwnedTextureAcquireFrame(
    mln_render_session* session,
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
                    (nint)pointer,
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
                (nint)pointer,
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
            throw new ObjectDisposedException(owner);
        }
    }

    public void Dispose()
    {
        IsClosed = true;
    }
}
