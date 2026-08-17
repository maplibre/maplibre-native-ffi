using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Render;

internal unsafe delegate mln_status AttachNative<T>(
    MlnMap map,
    T* descriptor,
    mln_render_session_attach_options* options,
    MlnRenderSession* outSession,
    MlnOperation* outOperation
)
    where T : unmanaged;

internal unsafe delegate mln_status StartRenderOperation(MlnOperation* outOperation);

/// <summary>A render session and its asynchronous attachment completion.</summary>
public sealed unsafe class RenderSessionHandle : IDisposable
{
    private readonly MapHandle map;
    private readonly NativeHandleState<MlnRenderSession> state;
    private readonly object frameGate = new();
    private readonly HashSet<AcquiredFrameHandle> acquiredFrames = [];

    private RenderSessionHandle(MapHandle map, MlnRenderSession handle, MlnOperation attachment)
    {
        this.map = map;
        state = new NativeHandleState<MlnRenderSession>(
            handle,
            static value => NativeMethods.mln_render_session_destroy(value),
            nameof(RenderSessionHandle)
        );
        Attachment = CompleteOperationAsync(attachment, default);
    }

    /// <summary>
    /// Completes after the selected driver initializes the target. A caller-graphics-thread host
    /// services driver work on the graphics thread while this task is pending.
    /// </summary>
    public Task Attachment { get; }

    public RenderSessionCapabilityInfo Capabilities
    {
        get
        {
            var value = new mln_render_session_capabilities
            {
                size = (uint)sizeof(mln_render_session_capabilities),
            };
            NativeStatus.Check(NativeMethods.mln_render_session_get_capabilities(Handle, &value));
            return new(
                (RenderDriverKind)value.driver,
                value.texture_ring_depth,
                (RenderSessionCapabilities)value.flags
            );
        }
    }

    public RenderSessionSnapshot Snapshot
    {
        get
        {
            var value = new mln_render_session_snapshot
            {
                size = (uint)sizeof(mln_render_session_snapshot),
            };
            NativeStatus.Check(NativeMethods.mln_render_session_get_snapshot(Handle, &value));
            return new(
                (RenderSessionState)value.state,
                (RenderDriverKind)value.driver,
                (RenderResult)value.latest_result,
                new(value.extent.width, value.extent.height, value.extent.scale_factor),
                value.generation,
                value.map_update_generation,
                value.rendered_update_generation,
                value.extent_generation,
                value.frame_generation,
                value.latest_demand_token,
                value.pending_demand_count,
                value.acquired_frame_count,
                value.target_ready != 0,
                value.pending_changes != 0
            );
        }
    }

    internal MlnRenderSession Handle => state.Handle;

    public static RenderSessionHandle AttachMetalSurface(
        MapHandle map,
        MetalSurfaceDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options,
            static (m, d, o, s, p) => NativeMethods.mln_metal_surface_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachVulkanSurface(
        MapHandle map,
        VulkanSurfaceDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options,
            static (m, d, o, s, p) => NativeMethods.mln_vulkan_surface_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachOpenGLSurface(
        MapHandle map,
        OpenGLSurfaceDescriptor descriptor,
        RenderSessionAttachOptions? options
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        using var selector = ApplyCanvasSelector(descriptor.Context, ref native.context);
        return Attach(
            map,
            native,
            options ?? DefaultOpenGLHostOptions(descriptor.Context),
            static (m, d, o, s, p) => NativeMethods.mln_opengl_surface_attach_start(m, d, o, s, p)
        );
    }

    public static RenderSessionHandle AttachWebGpuSurface(
        MapHandle map,
        WebGpuSurfaceDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options ?? CallerGraphicsOptions(),
            static (m, d, o, s, p) => NativeMethods.mln_webgpu_surface_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachMetalOwnedTexture(
        MapHandle map,
        MetalOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options,
            static (m, d, o, s, p) =>
                NativeMethods.mln_metal_owned_texture_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachMetalBorrowedTexture(
        MapHandle map,
        MetalBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options,
            static (m, d, o, s, p) =>
                NativeMethods.mln_metal_borrowed_texture_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachVulkanOwnedTexture(
        MapHandle map,
        VulkanOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options,
            static (m, d, o, s, p) =>
                NativeMethods.mln_vulkan_owned_texture_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachVulkanBorrowedTexture(
        MapHandle map,
        VulkanBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options,
            static (m, d, o, s, p) =>
                NativeMethods.mln_vulkan_borrowed_texture_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachOpenGLOwnedTexture(
        MapHandle map,
        OpenGLOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        using var selector = ApplyCanvasSelector(descriptor.Context, ref native.context);
        return Attach(
            map,
            native,
            options ?? DefaultOpenGLOwnedTextureOptions(descriptor.Context),
            static (m, d, o, s, p) =>
                NativeMethods.mln_opengl_owned_texture_attach_start(m, d, o, s, p)
        );
    }

    public static RenderSessionHandle AttachOpenGLBorrowedTexture(
        MapHandle map,
        OpenGLBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        using var selector = ApplyCanvasSelector(descriptor.Context, ref native.context);
        return Attach(
            map,
            native,
            options ?? DefaultOpenGLHostOptions(descriptor.Context),
            static (m, d, o, s, p) =>
                NativeMethods.mln_opengl_borrowed_texture_attach_start(m, d, o, s, p)
        );
    }

    public static RenderSessionHandle AttachWebGpuOwnedTexture(
        MapHandle map,
        WebGpuOwnedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options ?? CallerGraphicsOptions(),
            static (m, d, o, s, p) =>
                NativeMethods.mln_webgpu_owned_texture_attach_start(m, d, o, s, p)
        );

    public static RenderSessionHandle AttachWebGpuBorrowedTexture(
        MapHandle map,
        WebGpuBorrowedTextureDescriptor descriptor,
        RenderSessionAttachOptions? options
    ) =>
        Attach(
            map,
            RenderStructs.ToNative(descriptor),
            options ?? CallerGraphicsOptions(),
            static (m, d, o, s, p) =>
                NativeMethods.mln_webgpu_borrowed_texture_attach_start(m, d, o, s, p)
        );

    public void RequestFrame(FrameDemand demand)
    {
        var native = NativeMethods.mln_frame_demand_default();
        native.flags = (uint)demand.Flags;
        native.token = demand.Token;
        native.coalescing_boundary = demand.CoalescingBoundary;
        native.deadline_ns = demand.DeadlineNanoseconds;
        NativeStatus.Check(NativeMethods.mln_render_session_request_frame(Handle, &native));
    }

    public RenderFrameBatch DrainFrameResults()
    {
        MlnRenderFrameBatch batch = default;
        NativeStatus.Check(NativeMethods.mln_render_session_drain_frame_results(Handle, &batch));
        try
        {
            return new RenderFrameBatch(batch);
        }
        catch
        {
            NativeMethods.mln_render_frame_batch_release(batch);
            throw;
        }
    }

    public bool TryAcquireFrame(
        [System.Diagnostics.CodeAnalysis.NotNullWhen(true)] out AcquiredFrameHandle? frame
    )
    {
        MlnAcquiredFrame value = default;
        var status = NativeMethods.mln_render_session_acquire_frame(Handle, &value);
        if (status == mln_status.MLN_STATUS_NOT_READY)
        {
            frame = null;
            return false;
        }
        NativeStatus.Check(status);
        frame = new AcquiredFrameHandle(this, value);
        lock (frameGate)
            acquiredFrames.Add(frame);
        return true;
    }

    /// <summary>
    /// Services typed native work on the caller's current graphics thread. The first successful
    /// call fixes that native thread as the session's graphics thread.
    /// </summary>
    public int ServiceDriverWork(int maxWork)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(maxWork);
        nuint serviced = 0;
        NativeStatus.Check(
            NativeMethods.mln_render_session_service_driver_work(
                Handle,
                checked((nuint)maxWork),
                &serviced
            )
        );
        return checked((int)serviced);
    }

    public Task SetMetalSurfaceAsync(
        MetalSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_metal_surface_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task SetVulkanSurfaceAsync(
        VulkanSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_vulkan_surface_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task SetOpenGLSurfaceAsync(
        OpenGLSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        using var selector = ApplyCanvasSelector(descriptor.Context, ref native.context);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_opengl_surface_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task SetWebGpuSurfaceAsync(
        WebGpuSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_webgpu_surface_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task SetMetalBorrowedTextureAsync(
        MetalBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_metal_borrowed_texture_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task SetVulkanBorrowedTextureAsync(
        VulkanBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_vulkan_borrowed_texture_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task SetOpenGLBorrowedTextureAsync(
        OpenGLBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        using var selector = ApplyCanvasSelector(descriptor.Context, ref native.context);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_opengl_borrowed_texture_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task SetWebGpuBorrowedTextureAsync(
        WebGpuBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(descriptor);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_webgpu_borrowed_texture_set_target_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task ResizeAsync(
        RenderTargetExtent extent,
        CancellationToken cancellationToken = default
    )
    {
        var native = RenderStructs.ToNative(extent);
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_resize_start(Handle, &native, &operation)
        );
        return CompleteOperationAsync(operation, cancellationToken);
    }

    public Task BarrierAsync(
        ulong minUpdateGeneration,
        CancellationToken cancellationToken = default
    ) =>
        StartOperationAsync(
            operation =>
                NativeMethods.mln_render_session_barrier_start(
                    Handle,
                    minUpdateGeneration,
                    operation
                ),
            cancellationToken
        );

    public Task ReduceMemoryUseAsync(CancellationToken cancellationToken = default) =>
        StartOperationAsync(
            operation =>
                NativeMethods.mln_render_session_reduce_memory_use_start(Handle, operation),
            cancellationToken
        );

    public Task ClearDataAsync(CancellationToken cancellationToken = default) =>
        StartOperationAsync(
            operation => NativeMethods.mln_render_session_clear_data_start(Handle, operation),
            cancellationToken
        );

    public Task DumpDebugLogsAsync(CancellationToken cancellationToken = default) =>
        StartOperationAsync(
            operation => NativeMethods.mln_render_session_dump_debug_logs_start(Handle, operation),
            cancellationToken
        );

    public Task<byte[]> ReadPremultipliedRgba8Async(
        CancellationToken cancellationToken = default
    ) =>
        StartBufferOperationAsync(
            operation =>
                NativeMethods.mln_texture_read_premultiplied_rgba8_start(Handle, operation),
            static operation =>
            {
                MlnBuffer data = default;
                var info = new mln_texture_image_info
                {
                    size = (uint)sizeof(mln_texture_image_info),
                };
                NativeStatus.Check(
                    NativeMethods.mln_texture_read_premultiplied_rgba8_take_result(
                        operation,
                        &data,
                        &info
                    )
                );
                return ValueStructs.ReadBuffer(data);
            },
            cancellationToken
        );

    public Task<PremultipliedRgba8Image> ReadImageAsync(
        CancellationToken cancellationToken = default
    ) =>
        StartBufferOperationAsync(
            operation =>
                NativeMethods.mln_texture_read_premultiplied_rgba8_start(Handle, operation),
            static operation =>
            {
                MlnBuffer data = default;
                var info = new mln_texture_image_info
                {
                    size = (uint)sizeof(mln_texture_image_info),
                };
                NativeStatus.Check(
                    NativeMethods.mln_texture_read_premultiplied_rgba8_take_result(
                        operation,
                        &data,
                        &info
                    )
                );
                return new PremultipliedRgba8Image(
                    ValueStructs.ReadBuffer(data),
                    RenderStructs.FromNative(info)
                );
            },
            cancellationToken
        );

    public Task<IReadOnlyList<QueriedFeature>> QueryRenderedFeaturesAsync(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var nativeGeometry = NativeRenderedQueryGeometry.From(geometry);
        using var nativeOptions = options is null
            ? null
            : NativeRenderedFeatureQueryOptions.From(options);
        var geometryValue = nativeGeometry.Value;
        var optionsValue = nativeOptions?.Value ?? default;
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_query_rendered_features_start(
                Handle,
                &geometryValue,
                nativeOptions is null ? null : &optionsValue,
                &operation
            )
        );
        return AwaitBufferOperationAsync(operation, TakeQueriedFeaturesResult, cancellationToken);
    }

    public Task<IReadOnlyList<QueriedFeature>> QuerySourceFeaturesAsync(
        string sourceId,
        SourceFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var source = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeOptions = options is null
            ? null
            : NativeSourceFeatureQueryOptions.From(options);
        var optionsValue = nativeOptions?.Value ?? default;
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_query_source_features_start(
                Handle,
                source.Value,
                nativeOptions is null ? null : &optionsValue,
                &operation
            )
        );
        return AwaitBufferOperationAsync(operation, TakeQueriedFeaturesResult, cancellationToken);
    }

    public Task<byte[]> QueryFeatureExtensionAsync(
        string sourceId,
        byte[] feature,
        string extension,
        string extensionField,
        byte[]? arguments,
        CancellationToken cancellationToken = default
    )
    {
        using var source = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeFeature = NativeStringView.From(feature, nameof(feature));
        using var nativeExtension = NativeStringView.From(extension, nameof(extension));
        using var field = NativeStringView.From(extensionField, nameof(extensionField));
        using var nativeArguments = arguments is null
            ? null
            : NativeStringView.From(arguments, nameof(arguments));
        var argumentValue = nativeArguments?.Value ?? default;
        MlnOperation operation = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_query_feature_extensions_start(
                Handle,
                source.Value,
                nativeFeature.Value,
                nativeExtension.Value,
                field.Value,
                nativeArguments is null ? null : &argumentValue,
                &operation
            )
        );
        return AwaitBufferOperationAsync(operation, TakeRenderQueryResult, cancellationToken);
    }

    public Task SetFeatureStateAsync(
        FeatureStateSelector selector,
        byte[] stateJson,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(selector);
        using var source = NativeStringView.From(selector.SourceId, nameof(selector.SourceId));
        using var layer = NativeStringView.From(
            selector.SourceLayerId ?? string.Empty,
            nameof(selector.SourceLayerId)
        );
        using var feature = NativeStringView.From(
            selector.FeatureId ?? string.Empty,
            nameof(selector.FeatureId)
        );
        using var state = NativeStringView.From(stateJson, nameof(stateJson));
        return StartOperationAsync(
            operation =>
                NativeMethods.mln_render_session_set_feature_state_start(
                    Handle,
                    source.Value,
                    layer.Value,
                    feature.Value,
                    state.Value,
                    operation
                ),
            cancellationToken
        );
    }

    public Task<byte[]> GetFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(selector);
        using var source = NativeStringView.From(selector.SourceId, nameof(selector.SourceId));
        using var layer = NativeStringView.From(
            selector.SourceLayerId ?? string.Empty,
            nameof(selector.SourceLayerId)
        );
        using var feature = NativeStringView.From(
            selector.FeatureId ?? string.Empty,
            nameof(selector.FeatureId)
        );
        return StartBufferOperationAsync(
            operation =>
                NativeMethods.mln_render_session_get_feature_state_start(
                    Handle,
                    source.Value,
                    layer.Value,
                    feature.Value,
                    operation
                ),
            static operation =>
            {
                MlnBuffer value = default;
                NativeStatus.Check(
                    NativeMethods.mln_render_session_get_feature_state_take_result(
                        operation,
                        &value
                    )
                );
                return ValueStructs.ReadBuffer(value);
            },
            cancellationToken
        );
    }

    public Task RemoveFeatureStateAsync(
        FeatureStateSelector selector,
        CancellationToken cancellationToken = default
    )
    {
        ArgumentNullException.ThrowIfNull(selector);
        using var source = NativeStringView.From(selector.SourceId, nameof(selector.SourceId));
        using var layer = NativeStringView.From(
            selector.SourceLayerId ?? string.Empty,
            nameof(selector.SourceLayerId)
        );
        using var feature = NativeStringView.From(
            selector.FeatureId ?? string.Empty,
            nameof(selector.FeatureId)
        );
        using var key = NativeStringView.From(
            selector.StateKey ?? string.Empty,
            nameof(selector.StateKey)
        );
        return StartOperationAsync(
            operation =>
                NativeMethods.mln_render_session_remove_feature_state_start(
                    Handle,
                    source.Value,
                    layer.Value,
                    feature.Value,
                    key.Value,
                    operation
                ),
            cancellationToken
        );
    }

    public Task DetachAsync(CancellationToken cancellationToken = default) =>
        StartOperationAsync(
            operation => NativeMethods.mln_render_session_detach_start(Handle, operation),
            cancellationToken
        );

    public RenderAbandonResult Abandon()
    {
        var result = new mln_render_abandon_result
        {
            size = (uint)sizeof(mln_render_abandon_result),
        };
        NativeStatus.Check(NativeMethods.mln_render_session_abandon(Handle, &result));
        lock (frameGate)
        {
            foreach (var frame in acquiredFrames)
                frame.InvalidateAccessors();
        }
        return new((RenderAbandonDisposition)result.disposition, result.quarantined_resource_count);
    }

    public void Close() => state.Close();

    public void Dispose()
    {
        state.TryClose();
        GC.KeepAlive(map);
    }

    internal void FrameReleased(AcquiredFrameHandle frame)
    {
        lock (frameGate)
            acquiredFrames.Remove(frame);
    }

    private static RenderSessionAttachOptions CallerGraphicsOptions() =>
        new() { Driver = RenderDriverKind.CallerGraphicsThread };

    private static RenderSessionAttachOptions DefaultOpenGLHostOptions(
        OpenGLContextDescriptor? context
    ) =>
        new()
        {
            Driver = context is WebGLContextDescriptor { Kind: WebGLContextKind.TransferredCanvas }
                ? RenderDriverKind.CoreWorker
                : RenderDriverKind.CallerGraphicsThread,
        };

    private static RenderSessionAttachOptions DefaultOpenGLOwnedTextureOptions(
        OpenGLContextDescriptor? context
    ) =>
        new()
        {
            Driver = context switch
            {
                EglContextDescriptor { Ownership: OpenGLContextOwnership.Dedicated } =>
                    RenderDriverKind.CoreWorker,
                WebGLContextDescriptor { Kind: WebGLContextKind.TransferredCanvas } =>
                    RenderDriverKind.CoreWorker,
                _ => RenderDriverKind.CallerGraphicsThread,
            },
        };

    private static RenderSessionHandle Attach<T>(
        MapHandle map,
        T descriptor,
        RenderSessionAttachOptions? options,
        AttachNative<T> attach
    )
        where T : unmanaged
    {
        ArgumentNullException.ThrowIfNull(map);
        var nativeOptions = RenderStructs.ToNative(options);
        MlnRenderSession session = default;
        MlnOperation operation = default;
        NativeStatus.Check(attach(map.Handle, &descriptor, &nativeOptions, &session, &operation));
        return new RenderSessionHandle(map, session, operation);
    }

    private static NativeStringView? ApplyCanvasSelector(
        OpenGLContextDescriptor? context,
        ref mln_opengl_context_descriptor native
    )
    {
        if (
            context is not WebGLContextDescriptor { Kind: WebGLContextKind.TransferredCanvas } webgl
        )
            return null;
        var selector = NativeStringView.From(webgl.CanvasSelector, nameof(webgl.CanvasSelector));
        native.data.webgl.canvas_selector = selector.Value;
        return selector;
    }

    private Task StartOperationAsync(
        StartRenderOperation start,
        CancellationToken cancellationToken
    )
    {
        MlnOperation operation = default;
        NativeStatus.Check(start(&operation));
        return CompleteOperationAsync(operation, cancellationToken);
    }

    private Task CompleteOperationAsync(
        MlnOperation operation,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            map.Runtime.WaitForOperationAsync(operation, cancellationToken),
            () => RuntimeHandle.CheckOperationCompletion(operation),
            () => ReleaseOperation(operation)
        );

    private Task<T> StartBufferOperationAsync<T>(
        StartRenderOperation start,
        Func<MlnOperation, T> take,
        CancellationToken cancellationToken
    )
    {
        MlnOperation operation = default;
        NativeStatus.Check(start(&operation));
        var completedOperation = operation;
        return OperationAwaiter.WaitThen(
            map.Runtime.WaitForOperationAsync(completedOperation, cancellationToken),
            () =>
            {
                RuntimeHandle.CheckOperationCompletion(completedOperation);
                return take(completedOperation);
            },
            () => ReleaseOperation(completedOperation)
        );
    }

    private Task<T> AwaitBufferOperationAsync<T>(
        MlnOperation operation,
        Func<MlnOperation, T> take,
        CancellationToken cancellationToken
    ) =>
        OperationAwaiter.WaitThen(
            map.Runtime.WaitForOperationAsync(operation, cancellationToken),
            () =>
            {
                RuntimeHandle.CheckOperationCompletion(operation);
                return take(operation);
            },
            () => ReleaseOperation(operation)
        );

    internal Task WaitForOperationAsync(
        MlnOperation operation,
        CancellationToken cancellationToken
    ) => map.Runtime.WaitForOperationAsync(operation, cancellationToken);

    private void ReleaseOperation(MlnOperation operation)
    {
        NativeMethods.mln_operation_release(operation);
        GC.KeepAlive(this);
    }

    private static byte[] TakeRenderQueryResult(MlnOperation operation)
    {
        MlnBuffer value = default;
        NativeStatus.Check(NativeMethods.mln_render_query_take_result(operation, &value));
        return ValueStructs.ReadBuffer(value);
    }

    private static IReadOnlyList<QueriedFeature> TakeQueriedFeaturesResult(MlnOperation operation)
    {
        MlnQueriedFeatureList list = default;
        NativeStatus.Check(NativeMethods.mln_render_query_features_take_result(operation, &list));
        return CopyQueriedFeatureList(list);
    }

    private static IReadOnlyList<QueriedFeature> CopyQueriedFeatureList(MlnQueriedFeatureList list)
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

    internal static RenderFrameResult FromNative(mln_render_frame_result value) =>
        new(
            (RenderResult)value.disposition,
            value.token,
            value.map_update_generation,
            value.extent_generation,
            value.frame_generation,
            value.needs_repaint != 0
        );
}

/// <summary>Owned, stable frame-result records from one level-triggered drain.</summary>
public sealed unsafe class RenderFrameBatch : IReadOnlyList<RenderFrameResult>, IDisposable
{
    private readonly object gate = new();
    private MlnRenderFrameBatch handle;

    internal RenderFrameBatch(MlnRenderFrameBatch handle)
    {
        this.handle = handle;
        nuint count = 0;
        NativeStatus.Check(NativeMethods.mln_render_frame_batch_count(handle, &count));
        if (count > int.MaxValue)
        {
            throw new OverflowException("The frame-result batch is too large.");
        }
        Count = (int)count;
    }

    public int Count { get; }

    public RenderFrameResult this[int index]
    {
        get
        {
            ArgumentOutOfRangeException.ThrowIfNegative(index);
            if (index >= Count)
                throw new ArgumentOutOfRangeException(nameof(index));
            lock (gate)
            {
                if (handle.IsNull)
                    throw new ObjectDisposedException(nameof(RenderFrameBatch));
                var value = new mln_render_frame_result
                {
                    size = (uint)sizeof(mln_render_frame_result),
                };
                NativeStatus.Check(
                    NativeMethods.mln_render_frame_batch_get(handle, (nuint)index, &value)
                );
                return RenderSessionHandle.FromNative(value);
            }
        }
    }

    public IEnumerator<RenderFrameResult> GetEnumerator() => new Enumerator(this);

    System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() =>
        GetEnumerator();

    public void Dispose()
    {
        lock (gate)
        {
            if (handle.IsNull)
                return;
            NativeMethods.mln_render_frame_batch_release(handle);
            handle = default;
        }
    }

    private sealed class Enumerator(RenderFrameBatch batch) : IEnumerator<RenderFrameResult>
    {
        private int index = -1;

        public RenderFrameResult Current => batch[index];
        object System.Collections.IEnumerator.Current => Current;

        public bool MoveNext()
        {
            if (index + 1 >= batch.Count)
                return false;
            index++;
            return true;
        }

        public void Reset() => index = -1;

        public void Dispose() { }
    }
}

/// <summary>An acquired texture-ring slot whose native accessors remain valid until release.</summary>
public sealed unsafe class AcquiredFrameHandle : IDisposable
{
    private readonly RenderSessionHandle session;
    private readonly FrameScope scope = new();
    private readonly object gate = new();
    private MlnAcquiredFrame handle;

    internal AcquiredFrameHandle(RenderSessionHandle session, MlnAcquiredFrame handle)
    {
        this.session = session;
        this.handle = handle;
    }

    public RenderFrameResult Result
    {
        get
        {
            lock (gate)
            {
                var value = new mln_render_frame_result
                {
                    size = (uint)sizeof(mln_render_frame_result),
                };
                NativeStatus.Check(
                    NativeMethods.mln_acquired_frame_get_result(RequireHandleLocked(), &value)
                );
                return RenderSessionHandle.FromNative(value);
            }
        }
    }

    public GpuSync ProducerSync
    {
        get
        {
            lock (gate)
            {
                var value = NativeMethods.mln_gpu_sync_default();
                NativeStatus.Check(
                    NativeMethods.mln_acquired_frame_get_producer_sync(
                        RequireHandleLocked(),
                        &value
                    )
                );
                return new(
                    (GpuSyncKind)value.kind,
                    NativePointer.FromNativeAddress((nint)value.@object),
                    value.value
                );
            }
        }
    }

    public MetalOwnedTextureFrame GetMetalTexture()
    {
        lock (gate)
        {
            var value = new mln_metal_owned_texture_frame
            {
                size = (uint)sizeof(mln_metal_owned_texture_frame),
            };
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_metal_texture(RequireHandleLocked(), &value)
            );
            return RenderStructs.FromNative(value, scope);
        }
    }

    public VulkanOwnedTextureFrame GetVulkanTexture()
    {
        lock (gate)
        {
            var value = new mln_vulkan_owned_texture_frame
            {
                size = (uint)sizeof(mln_vulkan_owned_texture_frame),
            };
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_vulkan_texture(RequireHandleLocked(), &value)
            );
            return RenderStructs.FromNative(value, scope);
        }
    }

    public OpenGLOwnedTextureFrame GetOpenGLTexture()
    {
        lock (gate)
        {
            var value = new mln_opengl_owned_texture_frame
            {
                size = (uint)sizeof(mln_opengl_owned_texture_frame),
            };
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_opengl_texture(RequireHandleLocked(), &value)
            );
            return RenderStructs.FromNative(value, scope);
        }
    }

    public WebGpuOwnedTextureFrame GetWebGpuTexture()
    {
        lock (gate)
        {
            var value = new mln_webgpu_owned_texture_frame
            {
                size = (uint)sizeof(mln_webgpu_owned_texture_frame),
            };
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_webgpu_texture(RequireHandleLocked(), &value)
            );
            return RenderStructs.FromNative(value, scope);
        }
    }

    public Task ReleaseAsync(
        GpuSync? consumerCompletion,
        CancellationToken cancellationToken = default
    )
    {
        MlnOperation operation = default;
        lock (gate)
        {
            if (handle.IsNull)
                throw new ObjectDisposedException(nameof(AcquiredFrameHandle));
            var sync = NativeMethods.mln_gpu_sync_default();
            if (consumerCompletion is { } completion)
            {
                sync.kind = (uint)completion.Kind;
                sync.@object = (void*)completion.Object.Address;
                sync.value = completion.Value;
            }
            var frame = handle;
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_release_start(&frame, &sync, &operation)
            );
            handle = frame;
            scope.Dispose();
        }
        session.FrameReleased(this);
        var completedOperation = operation;
        return OperationAwaiter.WaitThen(
            session.WaitForOperationAsync(completedOperation, cancellationToken),
            () => RuntimeHandle.CheckOperationCompletion(completedOperation),
            () => NativeMethods.mln_operation_release(completedOperation)
        );
    }

    internal void InvalidateAccessors()
    {
        lock (gate)
            scope.Dispose();
    }

    public void Dispose()
    {
        MlnOperation operation = default;
        lock (gate)
        {
            if (handle.IsNull)
                return;
            var sync = NativeMethods.mln_gpu_sync_default();
            var frame = handle;
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_release_start(&frame, &sync, &operation)
            );
            handle = frame;
            scope.Dispose();
        }
        session.FrameReleased(this);
        NativeMethods.mln_operation_release(operation);
    }

    private MlnAcquiredFrame RequireHandleLocked()
    {
        if (handle.IsNull)
            throw new ObjectDisposedException(nameof(AcquiredFrameHandle));
        scope.EnsureActive();
        return handle;
    }
}

internal sealed class FrameScope : IDisposable
{
    private int active = 1;

    public void EnsureActive()
    {
        if (Volatile.Read(ref active) == 0)
            throw new ObjectDisposedException("acquired frame");
    }

    public void Dispose() => Interlocked.Exchange(ref active, 0);
}
