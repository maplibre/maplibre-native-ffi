// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A render session, which renders one map to one render target.
/// </summary>
/// <remarks>
/// See <c>mln_render_session</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
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

    /// <summary>
    /// Starts an ordered caller-owned Metal texture replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_metal_borrowed_texture_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered Metal surface replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_metal_surface_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered caller-owned OpenGL texture replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_opengl_borrowed_texture_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered OpenGL surface replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_opengl_surface_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Irreversibly closes control and mailboxes without graphics calls.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_abandon</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Acquires the oldest rendered frame that is not already acquired. The
    /// frame owns its slot until release. The call is nonblocking.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_acquire_frame</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public AcquiredFrameHandle? AcquireFrame()
    {
        using var call = Enter(this, "mln_render_session_acquire_frame");
        MlnAcquiredFrame outFrame = default;
        if (
            !Present(
                NativeMethods.mln_render_session_acquire_frame(Handle, &outFrame, Diagnostic),
                mln_status.MLN_STATUS_NOT_READY
            )
        )
            return null;
        return AcquiredFrameHandle.Adopt(this, outFrame);
    }

    /// <summary>
    /// Starts a barrier that completes after all render work accepted before it
    /// has a terminal result. A barrier does not request a frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_barrier</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_barrier");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_barrier(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts asynchronous renderer-data clearing.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_clear_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearDataAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_clear_data");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_clear_data(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Retires a detached or abandoned session handle. The call is CPU-only and
    /// may run on any native thread, including from one of the session's own
    /// completions. If frame disposal already started abandonment, this waits
    /// for that abandonment to finish before consuming the session owner.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_destroy</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_render_session_destroy");
        state.Close();
    }

    /// <summary>
    /// Starts normal graphics-owner teardown and map detachment.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_detach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task DetachAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_detach");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_detach(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Drains every currently queued terminal frame result into an
    /// independently owned batch. The records remain stable until the batch is
    /// released.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_drain_frame_results</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public RenderFrameBatchHandle? DrainFrameResults()
    {
        using var call = Enter(this, "mln_render_session_drain_frame_results");
        MlnRenderFrameBatch outBatch = default;
        if (
            !Present(
                NativeMethods.mln_render_session_drain_frame_results(Handle, &outBatch, Diagnostic),
                mln_status.MLN_STATUS_NOT_READY
            )
        )
            return null;
        return RenderFrameBatchHandle.Adopt(outBatch);
    }

    /// <summary>
    /// Starts asynchronous renderer diagnostic-log emission.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_dump_debug_logs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task DumpDebugLogsAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_dump_debug_logs");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_dump_debug_logs(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Returns the immutable capabilities fixed during attachment.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_get_capabilities</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies the latest render-session snapshot from any native thread.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_get_snapshot</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Copies the last completed rendered transform into an independent
    /// projection. Callable from any thread. Returns invalid state before a
    /// completed render, after an extent or target change, or after detachment.
    /// The caller owns the returned projection, which remains usable after the
    /// session is released. out_projection must point to a null handle.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_projection_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public MapProjectionHandle ProjectionCreate()
    {
        using var call = Enter(this, "mln_render_session_projection_create");
        MlnMapProjection outProjection = default;
        Check(
            NativeMethods.mln_render_session_projection_create(Handle, &outProjection, Diagnostic)
        );
        return MapProjectionHandle.Adopt(outProjection);
    }

    /// <summary>
    /// Starts a feature-extension query against the latest driver state. The
    /// completion borrows one <c>mln_buffer_view</c> holding UTF-8 JSON
    /// (value_count 1), valid only for the callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_query_feature_extensions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts a rendered-feature query against the session's latest driver
    /// state.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_query_rendered_features</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts a source-feature query against the session's latest driver state.
    /// The completion borrows an array of <c>mln_queried_feature</c> values
    /// (value_count entries), valid only for the callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_query_source_features</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts best-effort release of renderer caches.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_reduce_memory_use</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task ReduceMemoryUseAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this, "mln_render_session_reduce_memory_use");
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_reduce_memory_use(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Requests a frame without waiting. Every accepted demand produces one
    /// terminal result record. A core worker wakes itself; a caller driver
    /// publishes its driver-work endpoint.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_request_frame</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void RequestFrame(FrameDemand demand)
    {
        using var call = Enter(this, "mln_render_session_request_frame");
        var nativeDemand = NativeFrameDemand(demand);
        Check(NativeMethods.mln_render_session_request_frame(Handle, &nativeDemand, Diagnostic));
    }

    /// <summary>
    /// Starts an ordered logical resize. The completion runs after the selected
    /// driver applies the extent and updates the map viewport.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_resize</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Services up to max_work items for a caller-graphics-thread driver; zero
    /// services every item currently queued. The first successful service call
    /// fixes the session's graphics-thread identity; later calls from another
    /// native thread return <c>MLN_STATUS_WRONG_THREAD</c>. The target context
    /// must be current. Core-worker sessions return
    /// <c>MLN_STATUS_INVALID_STATE</c>.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_service_driver_work</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts readback of the latest rendered texture frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_texture_read_premultiplied_rgba8</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered caller-owned Vulkan texture replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_vulkan_borrowed_texture_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered Vulkan surface replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_vulkan_surface_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered caller-owned WebGPU texture replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_webgpu_borrowed_texture_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
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

    /// <summary>
    /// Starts an ordered WebGPU surface replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_webgpu_surface_set_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
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
