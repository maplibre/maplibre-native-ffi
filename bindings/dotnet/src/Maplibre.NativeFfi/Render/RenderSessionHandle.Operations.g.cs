// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public sealed unsafe partial class RenderSessionHandle : IDisposable, INativeOwner<MlnRenderSession>
{
    private readonly NativeHandleState<MlnRenderSession> state;
    public Task Completion { get; }

    internal RenderSessionHandle(MapHandle parent, MlnRenderSession handle, Task completion)
    {
        Completion = completion
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
        state = new(
            handle,
            static (live, diagnostic) => NativeMethods.mln_render_session_destroy(live, diagnostic),
            nameof(RenderSessionHandle),
            Abandon,
            retainedParent: parent
        );
    }

    internal static RenderSessionHandle Adopt(
        MapHandle parent,
        MlnRenderSession handle,
        Task completion
    ) =>
        NativeHandleState<MlnRenderSession>.Adopt(
            handle,
            () => new RenderSessionHandle(parent, handle, completion),
            Abandon
        );

    private static mln_status Abandon(MlnRenderSession live, mln_diagnostic* diagnostic) =>
        NativeMethods.mln_render_session_dispose(live, diagnostic);

    NativeHandleState<MlnRenderSession> INativeOwner<MlnRenderSession>.State => state;
    internal MlnRenderSession Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_render_session_dispose");
        state.Retire();
    }

    public Task MetalBorrowedTextureSetTargetAsync(
        MetalBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_metal_borrowed_texture_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_metal_borrowed_texture_set_target(
                    Handle,
                    scope.Value(NativeMetalBorrowedTextureDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task MetalSurfaceSetTargetAsync(
        MetalSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_metal_surface_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_metal_surface_set_target(
                    Handle,
                    scope.Value(NativeMetalSurfaceDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task OpenglBorrowedTextureSetTargetAsync(
        OpenglBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_opengl_borrowed_texture_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_opengl_borrowed_texture_set_target(
                    Handle,
                    scope.Value(NativeOpenglBorrowedTextureDescriptor(descriptor, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task OpenglSurfaceSetTargetAsync(
        OpenglSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_opengl_surface_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_opengl_surface_set_target(
                    Handle,
                    scope.Value(NativeOpenglSurfaceDescriptor(descriptor, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public RenderAbandonResult Abandon()
    {
        using var read = state.Read(this, "mln_render_session_abandon");
        var outResult = new mln_render_abandon_result
        {
            size = (uint)sizeof(mln_render_abandon_result),
        };
        Check(NativeMethods.mln_render_session_abandon(read.Handle, &outResult, Diagnostic));
        return CopyRenderAbandonResult(outResult);
    }

    public AcquiredFrameHandle AcquireFrame()
    {
        using var call = Enter(this, "mln_render_session_acquire_frame");
        MlnAcquiredFrame outFrame = default;
        Check(NativeMethods.mln_render_session_acquire_frame(Handle, &outFrame, Diagnostic));
        return AcquiredFrameHandle.Adopt(this, outFrame);
    }

    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_barrier");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_barrier(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public Task ClearDataAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_clear_data");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_clear_data(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_render_session_destroy");
        state.Close();
    }

    public Task DetachAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_detach");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_detach(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public RenderFrameBatchHandle DrainFrameResults()
    {
        using var call = Enter(this, "mln_render_session_drain_frame_results");
        MlnRenderFrameBatch outBatch = default;
        Check(NativeMethods.mln_render_session_drain_frame_results(Handle, &outBatch, Diagnostic));
        return RenderFrameBatchHandle.Adopt(outBatch);
    }

    public Task DumpDebugLogsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_dump_debug_logs");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_dump_debug_logs(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public RenderSessionCapabilities GetCapabilities()
    {
        using var read = state.Read(this, "mln_render_session_get_capabilities");
        var outCapabilities = new mln_render_session_capabilities
        {
            size = (uint)sizeof(mln_render_session_capabilities),
        };
        Check(
            NativeMethods.mln_render_session_get_capabilities(
                read.Handle,
                &outCapabilities,
                Diagnostic
            )
        );
        return CopyRenderSessionCapabilities(outCapabilities);
    }

    public RenderSessionSnapshot GetSnapshot()
    {
        using var read = state.Read(this, "mln_render_session_get_snapshot");
        var outSnapshot = new mln_render_session_snapshot
        {
            size = (uint)sizeof(mln_render_session_snapshot),
        };
        Check(NativeMethods.mln_render_session_get_snapshot(read.Handle, &outSnapshot, Diagnostic));
        return CopyRenderSessionSnapshot(outSnapshot);
    }

    public MapProjectionHandle ProjectionCreate()
    {
        using var call = Enter(this, "mln_render_session_projection_create");
        MlnMapProjection outProjection = default;
        Check(
            NativeMethods.mln_render_session_projection_create(Handle, &outProjection, Diagnostic)
        );
        return MapProjectionHandle.Adopt(outProjection);
    }

    public Task<byte[]> QueryFeatureExtensionsAsync(
        string sourceId,
        byte[] feature,
        string extension,
        string extensionField,
        byte[]? arguments,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_render_session_query_feature_extensions");
        return scope.Query<mln_buffer_view, byte[]>(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_query_feature_extensions(
                    Handle,
                    scope.Utf8(sourceId),
                    scope.Buffer(feature),
                    scope.Utf8(extension),
                    scope.Utf8(extensionField),
                    arguments is null ? null : scope.Value(scope.Buffer(arguments)),
                    completion,
                    diagnostic
                ),
            ValueStructs.CopyBufferView,
            cancellationToken
        );
    }

    public Task<QueriedFeature[]> QueryRenderedFeaturesAsync(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_render_session_query_rendered_features");
        return scope.QueryArray<mln_queried_feature, QueriedFeature>(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_query_rendered_features(
                    Handle,
                    scope.Value(NativeRenderedQueryGeometry(geometry, scope)),
                    options is null
                        ? null
                        : scope.Value(NativeRenderedFeatureQueryOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            CopyQueriedFeature,
            cancellationToken
        );
    }

    public Task<QueriedFeature[]> QuerySourceFeaturesAsync(
        string sourceId,
        SourceFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_render_session_query_source_features");
        return scope.QueryArray<mln_queried_feature, QueriedFeature>(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_query_source_features(
                    Handle,
                    scope.Utf8(sourceId),
                    options is null
                        ? null
                        : scope.Value(NativeSourceFeatureQueryOptions(options, scope)),
                    completion,
                    diagnostic
                ),
            CopyQueriedFeature,
            cancellationToken
        );
    }

    public Task ReduceMemoryUseAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_reduce_memory_use");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_reduce_memory_use(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    public void RequestFrame(FrameDemand demand)
    {
        using var call = Enter(this, "mln_render_session_request_frame");
        var nativeDemand = NativeFrameDemand(demand);
        Check(NativeMethods.mln_render_session_request_frame(Handle, &nativeDemand, Diagnostic));
    }

    public Task<CommandCompletion> ResizeAsync(
        RenderTargetExtent extent,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_render_session_resize");
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_resize(
                    Handle,
                    scope.Value(NativeRenderTargetExtent(extent)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public ulong ServiceDriverWork(ulong maxWork)
    {
        using var read = state.Read(this, "mln_render_session_service_driver_work");
        nuint outServiced = default;
        Check(
            NativeMethods.mln_render_session_service_driver_work(
                read.Handle,
                checked((nuint)maxWork),
                &outServiced,
                Diagnostic
            )
        );
        return (ulong)outServiced;
    }

    public Task<TextureReadbackResult> TextureReadPremultipliedRgba8Async(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_texture_read_premultiplied_rgba8");
        return scope.Query<mln_texture_readback_result, TextureReadbackResult>(
            (completion, diagnostic) =>
                NativeMethods.mln_texture_read_premultiplied_rgba8(Handle, completion, diagnostic),
            CopyTextureReadbackResult,
            cancellationToken
        );
    }

    public Task VulkanBorrowedTextureSetTargetAsync(
        VulkanBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_vulkan_borrowed_texture_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_vulkan_borrowed_texture_set_target(
                    Handle,
                    scope.Value(NativeVulkanBorrowedTextureDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task VulkanSurfaceSetTargetAsync(
        VulkanSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_vulkan_surface_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_vulkan_surface_set_target(
                    Handle,
                    scope.Value(NativeVulkanSurfaceDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task WebgpuBorrowedTextureSetTargetAsync(
        WebgpuBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_webgpu_borrowed_texture_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_webgpu_borrowed_texture_set_target(
                    Handle,
                    scope.Value(NativeWebgpuBorrowedTextureDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public Task WebgpuSurfaceSetTargetAsync(
        WebgpuSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this, "mln_webgpu_surface_set_target");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_webgpu_surface_set_target(
                    Handle,
                    scope.Value(NativeWebgpuSurfaceDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }
}
