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

/// <summary>Owner-thread render session handle bound to a map.</summary>
public sealed unsafe class RenderSessionHandle : IDisposable
{
    private readonly MapHandle map;
    private readonly NativeHandleState<mln_render_session> state;

    private RenderSessionHandle(MapHandle map, mln_render_session* handle)
    {
        this.map = map ?? throw new ArgumentNullException(nameof(map));
        state = new NativeHandleState<mln_render_session>(
            handle,
            static handle => NativeMethods.mln_render_session_destroy(handle),
            nameof(RenderSessionHandle)
        );
    }

    internal static RenderSessionHandle AttachMetalSurface(
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

    internal static RenderSessionHandle AttachVulkanSurface(
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

    internal static RenderSessionHandle AttachOpenGLSurface(
        MapHandle map,
        OpenGLSurfaceDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(NativeMethods.mln_opengl_surface_attach(map.Pointer, &native, &session));
        return new RenderSessionHandle(map, session);
    }

    internal static RenderSessionHandle AttachMetalOwnedTexture(
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

    internal static RenderSessionHandle AttachMetalBorrowedTexture(
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

    internal static RenderSessionHandle AttachVulkanOwnedTexture(
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

    internal static RenderSessionHandle AttachVulkanBorrowedTexture(
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

    internal static RenderSessionHandle AttachOpenGLOwnedTexture(
        MapHandle map,
        OpenGLOwnedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(
            NativeMethods.mln_opengl_owned_texture_attach(map.Pointer, &native, &session)
        );
        return new RenderSessionHandle(map, session);
    }

    internal static RenderSessionHandle AttachOpenGLBorrowedTexture(
        MapHandle map,
        OpenGLBorrowedTextureDescriptor descriptor
    )
    {
        ArgumentNullException.ThrowIfNull(map);
        var native = RenderStructs.ToNative(descriptor);
        mln_render_session* session = null;
        NativeStatus.Check(
            NativeMethods.mln_opengl_borrowed_texture_attach(map.Pointer, &native, &session)
        );
        return new RenderSessionHandle(map, session);
    }

    internal mln_render_session* Pointer => state.Pointer;

    public bool IsClosed => state.IsClosed;

    public void Resize(uint width, uint height, double scaleFactor)
    {
        if (!double.IsFinite(scaleFactor) || scaleFactor <= 0)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                "Render target scale factor must be positive and finite."
            );
        }

        NativeStatus.Check(
            NativeMethods.mln_render_session_resize(Pointer, width, height, scaleFactor)
        );
    }

    public void RenderUpdate()
    {
        NativeStatus.Check(NativeMethods.mln_render_session_render_update(Pointer));
    }

    public void Detach()
    {
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

    public IReadOnlyList<QueriedFeature> QueryRenderedFeatures(RenderedQueryGeometry geometry) =>
        QueryRenderedFeaturesCore(geometry, null);

    public IReadOnlyList<QueriedFeature> QueryRenderedFeatures(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions options
    ) =>
        QueryRenderedFeaturesCore(
            geometry,
            options ?? throw new ArgumentNullException(nameof(options))
        );

    public IReadOnlyList<QueriedFeature> QuerySourceFeatures(string sourceId) =>
        QuerySourceFeaturesCore(sourceId, null);

    public IReadOnlyList<QueriedFeature> QuerySourceFeatures(
        string sourceId,
        SourceFeatureQueryOptions options
    ) =>
        QuerySourceFeaturesCore(
            sourceId,
            options ?? throw new ArgumentNullException(nameof(options))
        );

    public FeatureExtensionResult QueryFeatureExtension(
        string sourceId,
        Feature feature,
        string extension,
        string extensionField,
        JsonValue? arguments = null
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
        var info = new mln_texture_image_info { size = (uint)sizeof(mln_texture_image_info) };
        var status = NativeMethods.mln_texture_read_premultiplied_rgba8(Pointer, null, 0, &info);
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
        var info = new mln_texture_image_info { size = (uint)sizeof(mln_texture_image_info) };
        fixed (byte* data = buffer.Span)
        {
            NativeStatus.Check(
                NativeMethods.mln_texture_read_premultiplied_rgba8(
                    Pointer,
                    buffer.ByteLength == 0 ? null : data,
                    buffer.ByteLength,
                    &info
                )
            );
        }
        return RenderStructs.FromNative(info);
    }

    public PremultipliedRgba8Image ReadPremultipliedRgba8()
    {
        var info = TextureImageInfo();
        using var buffer = new NativeBuffer((nuint)info.ByteLength);
        var readInfo = ReadPremultipliedRgba8(buffer);
        return new PremultipliedRgba8Image(buffer.Span.ToArray(), readInfo);
    }

    public MetalOwnedTextureFrameHandle AcquireMetalOwnedTextureFrame()
    {
        var pointer = (mln_metal_owned_texture_frame*)
            NativeMemory.AllocZeroed((nuint)sizeof(mln_metal_owned_texture_frame));
        pointer->size = (uint)sizeof(mln_metal_owned_texture_frame);
        var acquired = false;
        FrameScope? scope = null;
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_metal_owned_texture_acquire_frame(Pointer, pointer)
            );
            acquired = true;
            scope = new FrameScope(nameof(MetalOwnedTextureFrame));
            var frame = RenderStructs.FromNative(*pointer, scope);
            return new MetalOwnedTextureFrameHandle(this, pointer, scope, frame);
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
            NativeMemory.Free(pointer);
            throw;
        }
    }

    public VulkanOwnedTextureFrameHandle AcquireVulkanOwnedTextureFrame()
    {
        var pointer = (mln_vulkan_owned_texture_frame*)
            NativeMemory.AllocZeroed((nuint)sizeof(mln_vulkan_owned_texture_frame));
        pointer->size = (uint)sizeof(mln_vulkan_owned_texture_frame);
        var acquired = false;
        FrameScope? scope = null;
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_vulkan_owned_texture_acquire_frame(Pointer, pointer)
            );
            acquired = true;
            scope = new FrameScope(nameof(VulkanOwnedTextureFrame));
            var frame = RenderStructs.FromNative(*pointer, scope);
            return new VulkanOwnedTextureFrameHandle(this, pointer, scope, frame);
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
            NativeMemory.Free(pointer);
            throw;
        }
    }

    public OpenGLOwnedTextureFrameHandle AcquireOpenGLOwnedTextureFrame()
    {
        var pointer = (mln_opengl_owned_texture_frame*)
            NativeMemory.AllocZeroed((nuint)sizeof(mln_opengl_owned_texture_frame));
        pointer->size = (uint)sizeof(mln_opengl_owned_texture_frame);
        var acquired = false;
        FrameScope? scope = null;
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_opengl_owned_texture_acquire_frame(Pointer, pointer)
            );
            acquired = true;
            scope = new FrameScope(nameof(OpenGLOwnedTextureFrame));
            var frame = RenderStructs.FromNative(*pointer, scope);
            return new OpenGLOwnedTextureFrameHandle(this, pointer, scope, frame);
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
            NativeMemory.Free(pointer);
            throw;
        }
    }

    internal mln_status ReleaseMetalFrame(mln_metal_owned_texture_frame* frame) =>
        NativeMethods.mln_metal_owned_texture_release_frame(Pointer, frame);

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
        state.Close();
    }

    /// <inheritdoc />
    public void Dispose()
    {
        state.TryClose();
        GC.KeepAlive(map);
    }
}

internal unsafe delegate mln_status FrameRelease<T>(RenderSessionHandle session, T* frame)
    where T : unmanaged;

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
        string typeName
    )
    {
        this.session = session;
        this.pointer = pointer;
        this.scope = scope;
        this.release = release;
        this.typeName = typeName;
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
    }
}

public sealed unsafe class MetalOwnedTextureFrameHandle : IDisposable
{
    private readonly TextureFrameState<mln_metal_owned_texture_frame> state;

    internal MetalOwnedTextureFrameHandle(
        RenderSessionHandle session,
        mln_metal_owned_texture_frame* pointer,
        FrameScope scope,
        MetalOwnedTextureFrame frame
    )
    {
        state = new TextureFrameState<mln_metal_owned_texture_frame>(
            session,
            pointer,
            scope,
            static (session, frame) => session.ReleaseMetalFrame(frame),
            nameof(MetalOwnedTextureFrameHandle)
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
        VulkanOwnedTextureFrame frame
    )
    {
        state = new TextureFrameState<mln_vulkan_owned_texture_frame>(
            session,
            pointer,
            scope,
            static (session, frame) => session.ReleaseVulkanFrame(frame),
            nameof(VulkanOwnedTextureFrameHandle)
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
        OpenGLOwnedTextureFrame frame
    )
    {
        state = new TextureFrameState<mln_opengl_owned_texture_frame>(
            session,
            pointer,
            scope,
            static (session, frame) => session.ReleaseOpenGLFrame(frame),
            nameof(OpenGLOwnedTextureFrameHandle)
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
