// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Render;

public sealed unsafe partial class RenderSessionHandle : IDisposable
{
    private readonly NativeHandleState<MlnRenderSession> state;
    private readonly ulong nativeId;
    public ulong Id => nativeId;
    public Task Completion { get; }

    internal RenderSessionHandle(MapHandle parent, MlnRenderSession handle, Task completion)
    {
        nativeId = handle.Value;
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
        state = new NativeHandleState<MlnRenderSession>(
            handle,
            static live => NativeMethods.mln_render_session_destroy(live),
            nameof(RenderSessionHandle),
            static live => NativeMethods.mln_render_session_dispose(live),
            retainedParent: parent
        );
    }

    internal static RenderSessionHandle Adopt(
        MapHandle parent,
        MlnRenderSession handle,
        Task completion
    )
    {
        RenderSessionHandle? owner = null;
        try
        {
            owner = new RenderSessionHandle(parent, handle, completion);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_render_session_dispose(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnRenderSession Handle => state.Handle;

    internal NativeHandleState<MlnRenderSession>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_dispose"
        );
        state.Retire();
    }

    public Task MetalBorrowedTextureSetTargetAsync(
        MetalBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_metal_borrowed_texture_set_target"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeDescriptor = NativeMetalBorrowedTextureDescriptor(descriptor);
                    return NativeMethods.mln_metal_borrowed_texture_set_target(
                        Handle,
                        &nativeDescriptor,
                        completion
                    );
                },
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task MetalSurfaceSetTargetAsync(
        MetalSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_metal_surface_set_target"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeDescriptor = NativeMetalSurfaceDescriptor(descriptor);
                    return NativeMethods.mln_metal_surface_set_target(
                        Handle,
                        &nativeDescriptor,
                        completion
                    );
                },
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task OpenglBorrowedTextureSetTargetAsync(
        OpenglBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_opengl_borrowed_texture_set_target"
        );
        var operation = NativeCompletion.Submit(
            completion =>
            {
                var nativeDescriptor = NativeOpenglBorrowedTextureDescriptor(descriptor, scope);
                return NativeMethods.mln_opengl_borrowed_texture_set_target(
                    Handle,
                    &nativeDescriptor,
                    completion
                );
            },
            result => true
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task OpenglSurfaceSetTargetAsync(
        OpenglSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_opengl_surface_set_target"
        );
        var operation = NativeCompletion.Submit(
            completion =>
            {
                var nativeDescriptor = NativeOpenglSurfaceDescriptor(descriptor, scope);
                return NativeMethods.mln_opengl_surface_set_target(
                    Handle,
                    &nativeDescriptor,
                    completion
                );
            },
            result => true
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public RenderAbandonResult Abandon()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_abandon"
        );
        var outResult = new mln_render_abandon_result
        {
            size = (uint)sizeof(mln_render_abandon_result),
        };
        NativeStatus.Check(NativeMethods.mln_render_session_abandon(read.Handle, &outResult));
        return CopyRenderAbandonResult(outResult);
    }

    public AcquiredFrameHandle AcquireFrame()
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_acquire_frame"
        );
        MlnAcquiredFrame outFrame = default;
        NativeStatus.Check(NativeMethods.mln_render_session_acquire_frame(Handle, &outFrame));
        return AcquiredFrameHandle.Adopt(this, outFrame);
    }

    public Task BarrierAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_barrier"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_render_session_barrier(Handle, completion),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task ClearDataAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_clear_data"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_render_session_clear_data(Handle, completion),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public void Close()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_destroy"
        );
        state.Close();
    }

    public Task DetachAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_detach"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_render_session_detach(Handle, completion),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public RenderFrameBatchHandle DrainFrameResults()
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_drain_frame_results"
        );
        MlnRenderFrameBatch outBatch = default;
        NativeStatus.Check(NativeMethods.mln_render_session_drain_frame_results(Handle, &outBatch));
        return RenderFrameBatchHandle.Adopt(outBatch);
    }

    public Task DumpDebugLogsAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_dump_debug_logs"
        );
        return NativeCompletion
            .Submit(
                completion => NativeMethods.mln_render_session_dump_debug_logs(Handle, completion),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public RenderSessionCapabilities GetCapabilities()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_get_capabilities"
        );
        var outCapabilities = new mln_render_session_capabilities
        {
            size = (uint)sizeof(mln_render_session_capabilities),
        };
        NativeStatus.Check(
            NativeMethods.mln_render_session_get_capabilities(read.Handle, &outCapabilities)
        );
        return CopyRenderSessionCapabilities(outCapabilities);
    }

    public RenderSessionSnapshot GetSnapshot()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_get_snapshot"
        );
        var outSnapshot = new mln_render_session_snapshot
        {
            size = (uint)sizeof(mln_render_session_snapshot),
        };
        NativeStatus.Check(
            NativeMethods.mln_render_session_get_snapshot(read.Handle, &outSnapshot)
        );
        return CopyRenderSessionSnapshot(outSnapshot);
    }

    public MapProjectionHandle ProjectionCreate()
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_projection_create"
        );
        MlnMapProjection outProjection = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_projection_create(Handle, &outProjection)
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
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_query_feature_extensions"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        using var nativeFeature = NativeStringView.From(feature, nameof(feature));
        using var nativeExtension = NativeStringView.From(extension, nameof(extension));
        using var nativeExtensionField = NativeStringView.From(
            extensionField,
            nameof(extensionField)
        );
        var operation = NativeCompletion.Submit(
            completion =>
            {
                var nativeArguments = arguments is null
                    ? default(mln_buffer_view)
                    : scope.Buffer(arguments);
                return NativeMethods.mln_render_session_query_feature_extensions(
                    Handle,
                    nativeSourceId.Value,
                    nativeFeature.Value,
                    nativeExtension.Value,
                    nativeExtensionField.Value,
                    arguments is null ? null : &nativeArguments,
                    completion
                );
            },
            result =>
            {
                var value = NativeCompletion.Value<mln_buffer_view>(result);
                return ValueStructs.CopyBufferView(value);
            }
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<QueriedFeature[]> QueryRenderedFeaturesAsync(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_query_rendered_features"
        );
        var operation = NativeCompletion.Submit(
            completion =>
            {
                var nativeGeometry = NativeRenderedQueryGeometry(geometry, scope);
                var nativeOptions = options is null
                    ? default(mln_rendered_feature_query_options)
                    : NativeRenderedFeatureQueryOptions(options, scope);
                return NativeMethods.mln_render_session_query_rendered_features(
                    Handle,
                    &nativeGeometry,
                    options is null ? null : &nativeOptions,
                    completion
                );
            },
            result =>
            {
                var values = NativeCompletion.Values<mln_queried_feature>(result);
                var copied = new QueriedFeature[values.Length];
                for (var index = 0; index < values.Length; index++)
                    copied[index] = CopyQueriedFeature(values[index]);
                return copied;
            }
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task<QueriedFeature[]> QuerySourceFeaturesAsync(
        string sourceId,
        SourceFeatureQueryOptions? options,
        CancellationToken cancellationToken = default
    )
    {
        using var scope = new NativeCallScope();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_query_source_features"
        );
        using var nativeSourceId = NativeStringView.From(sourceId, nameof(sourceId));
        var operation = NativeCompletion.Submit(
            completion =>
            {
                var nativeOptions = options is null
                    ? default(mln_source_feature_query_options)
                    : NativeSourceFeatureQueryOptions(options, scope);
                return NativeMethods.mln_render_session_query_source_features(
                    Handle,
                    nativeSourceId.Value,
                    options is null ? null : &nativeOptions,
                    completion
                );
            },
            result =>
            {
                var values = NativeCompletion.Values<mln_queried_feature>(result);
                var copied = new QueriedFeature[values.Length];
                for (var index = 0; index < values.Length; index++)
                    copied[index] = CopyQueriedFeature(values[index]);
                return copied;
            }
        );
        scope.Accept();
        return operation.WaitAsync(cancellationToken);
    }

    public Task ReduceMemoryUseAsync(CancellationToken cancellationToken = default)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_reduce_memory_use"
        );
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_render_session_reduce_memory_use(Handle, completion),
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public void RequestFrame(FrameDemand demand)
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_request_frame"
        );
        var nativeDemand = NativeFrameDemand(demand);
        NativeStatus.Check(NativeMethods.mln_render_session_request_frame(Handle, &nativeDemand));
    }

    public Task<CommandCompletion> ResizeAsync(
        RenderTargetExtent extent,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_resize"
        );
        return NativeCompletion
            .SubmitCommand(completion =>
            {
                var nativeExtent = NativeRenderTargetExtent(extent);
                return NativeMethods.mln_render_session_resize(Handle, &nativeExtent, completion);
            })
            .WaitAsync(cancellationToken);
    }

    public ulong ServiceDriverWork(ulong maxWork)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_render_session_service_driver_work"
        );
        nuint outServiced = default;
        NativeStatus.Check(
            NativeMethods.mln_render_session_service_driver_work(
                read.Handle,
                checked((nuint)maxWork),
                &outServiced
            )
        );
        return (ulong)outServiced;
    }

    public Task<TextureReadbackResult> TextureReadPremultipliedRgba8Async(
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_texture_read_premultiplied_rgba8"
        );
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_texture_read_premultiplied_rgba8(Handle, completion),
                result =>
                    CopyTextureReadbackResult(
                        NativeCompletion.Value<mln_texture_readback_result>(result)
                    )
            )
            .WaitAsync(cancellationToken);
    }

    public Task VulkanBorrowedTextureSetTargetAsync(
        VulkanBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_vulkan_borrowed_texture_set_target"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeDescriptor = NativeVulkanBorrowedTextureDescriptor(descriptor);
                    return NativeMethods.mln_vulkan_borrowed_texture_set_target(
                        Handle,
                        &nativeDescriptor,
                        completion
                    );
                },
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task VulkanSurfaceSetTargetAsync(
        VulkanSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_vulkan_surface_set_target"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeDescriptor = NativeVulkanSurfaceDescriptor(descriptor);
                    return NativeMethods.mln_vulkan_surface_set_target(
                        Handle,
                        &nativeDescriptor,
                        completion
                    );
                },
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task WebgpuBorrowedTextureSetTargetAsync(
        WebgpuBorrowedTextureDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_webgpu_borrowed_texture_set_target"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeDescriptor = NativeWebgpuBorrowedTextureDescriptor(descriptor);
                    return NativeMethods.mln_webgpu_borrowed_texture_set_target(
                        Handle,
                        &nativeDescriptor,
                        completion
                    );
                },
                result => true
            )
            .WaitAsync(cancellationToken);
    }

    public Task WebgpuSurfaceSetTargetAsync(
        WebgpuSurfaceDescriptor descriptor,
        CancellationToken cancellationToken = default
    )
    {
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_webgpu_surface_set_target"
        );
        return NativeCompletion
            .Submit(
                completion =>
                {
                    var nativeDescriptor = NativeWebgpuSurfaceDescriptor(descriptor);
                    return NativeMethods.mln_webgpu_surface_set_target(
                        Handle,
                        &nativeDescriptor,
                        completion
                    );
                },
                result => true
            )
            .WaitAsync(cancellationToken);
    }
}
