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
            static (live, diagnostic) => NativeMethods.mln_render_session_release(live, diagnostic),
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

    // Runtime events report their source by this identity.
    public ulong Id => state.Handle.Value;

    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Irreversibly ends session work and destroys the session's graphics
    /// objects where that is safe.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_abandon</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public RenderAbandonResult Abandon()
    {
        using var call = Enter(this);
        var outResult = new mln_render_abandon_result
        {
            size = (uint)sizeof(mln_render_abandon_result),
        };
        Check(NativeMethods.mln_render_session_abandon(Handle, &outResult, Diagnostic));
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
        using var call = Enter(this);
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
    /// Ends every frame demand that has not started with
    /// <c>MLN_RENDER_RESULT_NO_UPDATE</c> and completes after earlier driver
    /// work has finished. A running demand finishes its attempt and ends any
    /// wait for a map update or a free slot. After completion, a new frame
    /// demand is required to render again.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_barrier</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        using var scope = new NativeCallScope(this);
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
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_clear_data(Handle, completion, diagnostic),
            cancellationToken
        );
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
        using var scope = new NativeCallScope(this);
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
        using var call = Enter(this);
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
        using var scope = new NativeCallScope(this);
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
        using var call = Enter(this);
        var outCapabilities = new mln_render_session_capabilities
        {
            size = (uint)sizeof(mln_render_session_capabilities),
        };
        Check(
            NativeMethods.mln_render_session_get_capabilities(Handle, &outCapabilities, Diagnostic)
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
        using var call = Enter(this);
        var outSnapshot = new mln_render_session_snapshot
        {
            size = (uint)sizeof(mln_render_session_snapshot),
        };
        Check(NativeMethods.mln_render_session_get_snapshot(Handle, &outSnapshot, Diagnostic));
        return CopyRenderSessionSnapshot(outSnapshot);
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
        using var scope = new NativeCallScope(this);
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
    public Task<RenderedFeature[]> QueryRenderedFeaturesAsync(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryArray<mln_rendered_feature, RenderedFeature>(
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
            CopyRenderedFeature,
            cancellationToken
        );
    }

    /// <summary>
    /// Starts a source-feature query against the session's latest driver state.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_query_source_features</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
    public Task<SourceFeature[]> QuerySourceFeaturesAsync(
        string sourceId,
        SourceFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.QueryArray<mln_source_feature, SourceFeature>(
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
            CopySourceFeature,
            cancellationToken
        );
    }

    /// <summary>
    /// Reads back the latest frame of the session's owned texture as
    /// premultiplied RGBA8.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_read_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public Task<TextureReadbackResult> ReadTextureAsync(
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Query<mln_texture_readback_result, TextureReadbackResult>(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_read_texture(Handle, completion, diagnostic),
            CopyTextureReadbackResult,
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
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_reduce_memory_use(Handle, completion, diagnostic),
            cancellationToken
        );
    }

    /// <summary>
    /// Retires a detached or abandoned session handle. The call is CPU-only and
    /// may run on any native thread, including from one of the session's own
    /// completions. If an abandonment is still in progress on another thread,
    /// this waits for it to finish before consuming the session owner.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void Close()
    {
        state.Close();
    }

    /// <summary>
    /// Requests a frame without waiting. Every accepted demand produces one
    /// terminal result record. Caller-driver sessions require service on the
    /// graphics thread. Core-worker sessions need no host service.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_request_frame</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void RequestFrame(FrameDemand demand)
    {
        using var call = Enter(this);
        var nativeDemand = NativeFrameDemand(demand);
        Check(NativeMethods.mln_render_session_request_frame(Handle, &nativeDemand, Diagnostic));
    }

    /// <summary>
    /// Starts an ordered logical resize of the driver and the map. The driver
    /// applies the extent in order with its other work, and later driver work
    /// does not wait for the map. The completion runs after both have applied
    /// it and the map has published its snapshot. Frames render once a map
    /// update matches the applied extent; static and tile maps publish that
    /// update for their next still image.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_resize</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> ResizeAsync(
        LogicalExtent extent,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Command(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_resize(
                    Handle,
                    NativeLogicalExtent(extent),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Services every queued item for a caller-graphics-thread driver. The
    /// first successful service call fixes the session's graphics-thread
    /// identity; later calls from another native thread return
    /// <c>MLN_STATUS_WRONG_THREAD</c>. The target context must be current.
    /// Core-worker sessions return <c>MLN_STATUS_INVALID_STATE</c>.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_service_driver_work</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void ServiceDriverWork()
    {
        using var call = Enter(this);
        Check(NativeMethods.mln_render_session_service_driver_work(Handle, Diagnostic));
    }

    /// <summary>
    /// Starts an ordered replacement of every texture of a caller-owned Metal
    /// ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_metal_borrowed_texture_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetMetalBorrowedTextureTargetAsync(
        MetalBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_metal_borrowed_texture_target(
                    Handle,
                    scope.Value(NativeMetalBorrowedTextureDescriptor(descriptor, scope)),
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
    /// See <c>mln_render_session_set_metal_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetMetalSurfaceTargetAsync(
        MetalSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_metal_surface_target(
                    Handle,
                    scope.Value(NativeMetalSurfaceDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered replacement of every texture of a caller-owned OpenGL
    /// ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_opengl_borrowed_texture_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOpenglBorrowedTextureTargetAsync(
        OpenglBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_opengl_borrowed_texture_target(
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
    /// See <c>mln_render_session_set_opengl_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOpenglSurfaceTargetAsync(
        OpenglSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_opengl_surface_target(
                    Handle,
                    scope.Value(NativeOpenglSurfaceDescriptor(descriptor, scope)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered replacement of every image of a caller-owned Vulkan
    /// ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_vulkan_borrowed_texture_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetVulkanBorrowedTextureTargetAsync(
        VulkanBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_vulkan_borrowed_texture_target(
                    Handle,
                    scope.Value(NativeVulkanBorrowedTextureDescriptor(descriptor, scope)),
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
    /// See <c>mln_render_session_set_vulkan_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetVulkanSurfaceTargetAsync(
        VulkanSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_vulkan_surface_target(
                    Handle,
                    scope.Value(NativeVulkanSurfaceDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    /// <summary>
    /// Starts an ordered replacement of every texture of a caller-owned WebGPU
    /// ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_webgpu_borrowed_texture_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetWebgpuBorrowedTextureTargetAsync(
        WebgpuBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_webgpu_borrowed_texture_target(
                    Handle,
                    scope.Value(NativeWebgpuBorrowedTextureDescriptor(descriptor, scope)),
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
    /// See <c>mln_render_session_set_webgpu_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetWebgpuSurfaceTargetAsync(
        WebgpuSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope(this);
        return scope.Run(
            (completion, diagnostic) =>
                NativeMethods.mln_render_session_set_webgpu_surface_target(
                    Handle,
                    scope.Value(NativeWebgpuSurfaceDescriptor(descriptor)),
                    completion,
                    diagnostic
                ),
            cancellationToken
        );
    }

    public DiscardingCommands Discarding => new(this);

    public readonly struct DiscardingCommands
    {
        private readonly RenderSessionHandle owner;

        internal DiscardingCommands(RenderSessionHandle owner)
        {
            this.owner = owner;
        }

        public void Resize(LogicalExtent extent)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            scope.Discard(
                (completion, diagnostic) =>
                    NativeMethods.mln_render_session_resize(
                        receiverHandle,
                        NativeLogicalExtent(extent),
                        completion,
                        diagnostic
                    )
            );
        }
    }
}
