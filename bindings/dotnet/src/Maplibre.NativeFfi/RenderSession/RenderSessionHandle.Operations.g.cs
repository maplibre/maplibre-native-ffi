// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A render session, which renders one map to one render target.
/// </summary>
/// <remarks>
/// <para>With a caller-graphics-thread driver, <c>Ready</c>,
/// <c>DetachAsync</c>, <c>BarrierAsync</c>, and <c>ResizeAsync</c> progress
/// only through <c>ServiceDriverWork</c>, so awaiting one on the graphics
/// thread without servicing driver work never completes.</para>
/// <para>See <c>mln_render_session</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed unsafe partial class RenderSessionHandle
    : IDisposable,
        IAsyncDisposable,
        INativeOwner<MlnRenderSession>
{
    private readonly NativeHandleState<MlnRenderSession> state;
    private volatile Task teardown = Task.CompletedTask;

    /// <summary>
    /// Completes once the handle is ready, and faults with a
    /// <see cref="MaplibreException" /> when it fails to become ready.
    /// </summary>
    public Task Ready { get; }

    internal RenderSessionHandle(MapHandle parent, MlnRenderSession handle, Task completion)
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
        state = new(
            handle,
            StartRelease,
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
        NativeMethods.mln_render_session_dispose(live, null, diagnostic);

    NativeHandleState<MlnRenderSession> INativeOwner<MlnRenderSession>.State => state;
    internal MlnRenderSession Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after close.
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
    /// <c>RenderResult.NoUpdate</c> and completes after earlier driver work has
    /// finished. A running demand finishes its attempt and ends any wait for a
    /// map update or a free slot. After completion, a new frame demand is
    /// required to render again, except that a pending still image submits its
    /// next demand itself; see <c>MapHandle.RequestStillImageAsync()</c>.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_barrier</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task BarrierAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_barrier(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts asynchronous renderer-data clearing.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_clear_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task ClearDataAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_clear_data(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts normal graphics-owner teardown and map detachment.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_detach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task DetachAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_detach(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Consumes a session in any state and retires it, then runs completion, if
    /// given, once the host may destroy the target's graphics objects.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_dispose</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task CloseAsync()
    {
        state.Close(idempotent: false);
        return teardown;
    }

    public ValueTask DisposeAsync() => state.Close() ? new(teardown) : default;

    private mln_status StartRelease(MlnRenderSession handle, mln_diagnostic* _)
    {
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_dispose(
            handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        teardown = nativePending.Submitted(nativeStatus);
        return mln_status.MLN_STATUS_OK;
    }

    /// <summary>
    /// Drains every currently queued terminal frame result.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_drain_frame_results</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public RenderFrameResult[] DrainFrameResults()
    {
        using var call = Enter(this);
        MlnRenderFrameBatch outBatch = default;
        var outView = new mln_render_frame_batch_view
        {
            size = (uint)sizeof(mln_render_frame_batch_view),
        };
        if (
            !Present(
                NativeMethods.mln_render_session_drain_frame_results(
                    Handle,
                    &outBatch,
                    &outView,
                    Diagnostic
                ),
                mln_status.MLN_STATUS_NOT_READY
            )
        )
            return [];
        try
        {
            return CopyRenderFrameBatchViewResults(outView);
        }
        finally
        {
            NativeMethods.mln_render_frame_batch_release(outBatch);
        }
    }

    /// <summary>
    /// Starts asynchronous renderer diagnostic-log emission.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_dump_debug_logs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task DumpDebugLogsAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_dump_debug_logs(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
    /// Reports the session's first terminal failure from any native thread.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_get_failure</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void GetFailure()
    {
        using var call = Enter(this);
        Check(NativeMethods.mln_render_session_get_failure(Handle, Diagnostic));
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
    /// Starts a feature-extension query against the latest driver state.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_query_feature_extensions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
    public Task<string> QueryFeatureExtensionsAsync(
        string sourceId,
        string feature,
        string extension,
        string extensionField,
        string arguments
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_query_feature_extensions(
            Handle,
            scope.Utf8(sourceId),
            scope.Utf8(feature),
            scope.Utf8(extension),
            scope.Utf8(extensionField),
            scope.Utf8(arguments),
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
    /// Starts a rendered-feature query against the last frame that the session
    /// rendered, so render after a camera or style change for a result that
    /// matches it.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_query_rendered_features</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
    public Task<RenderedFeature[]> QueryRenderedFeaturesAsync(
        RenderedQueryGeometry geometry,
        RenderedFeatureQueryOptions? options = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeGeometry = NativeRenderedQueryGeometry(geometry, scope);
        var nativeOptions = options is null
            ? default(mln_rendered_feature_query_options)
            : NativeRenderedFeatureQueryOptions(options, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_query_rendered_features(
            Handle,
            &nativeGeometry,
            options is null ? null : &nativeOptions,
            NativeCompletion.Start<RenderedFeature[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_rendered_feature, RenderedFeature>(
                        nativeResult,
                        CopyRenderedFeature
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts a source-feature query against the tiles that the source loaded
    /// for rendering, which it loads only while a visible layer uses it.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_query_source_features</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
    /// </remarks>
    public Task<SourceFeature[]> QuerySourceFeaturesAsync(
        string sourceId,
        SourceFeatureQueryOptions? options = null
    )
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = options is null
            ? default(mln_source_feature_query_options)
            : NativeSourceFeatureQueryOptions(options, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_query_source_features(
            Handle,
            scope.Utf8(sourceId),
            options is null ? null : &nativeOptions,
            NativeCompletion.Start<SourceFeature[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_source_feature, SourceFeature>(
                        nativeResult,
                        CopySourceFeature
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Reads back the latest frame of the session's owned texture as
    /// premultiplied RGBA8.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_read_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public Task<TextureReadbackResult> ReadTextureAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_read_texture(
            Handle,
            NativeCompletion.Start<TextureReadbackResult>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_texture_readback_result, TextureReadbackResult>(
                        nativeResult,
                        CopyTextureReadbackResult
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts best-effort release of renderer caches.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_reduce_memory_use</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public Task ReduceMemoryUseAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_reduce_memory_use(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
    public Task<CommandCompletion> ResizeAsync(LogicalExtent extent)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_resize(
            Handle,
            NativeLogicalExtent(extent),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Services every queued item for a caller-graphics-thread driver. The
    /// first successful service call fixes the session's graphics-thread
    /// identity; later calls from another native thread return
    /// <c>Status.WrongThread</c>. The target context must be current.
    /// Core-worker sessions return <c>Status.InvalidState</c>.
    /// </summary>
    /// <remarks>
    /// <para>With a caller-graphics-thread driver, <c>Ready</c>,
    /// <c>DetachAsync</c>, <c>BarrierAsync</c>, and <c>ResizeAsync</c> progress
    /// only through <c>ServiceDriverWork</c>, so awaiting one on the graphics
    /// thread without servicing driver work never completes.</para>
    /// <para>See <c>mln_render_session_service_driver_work</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
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
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
    public Task SetMetalBorrowedTextureTargetAsync(MetalBorrowedTextureDescriptor descriptor)
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeMetalBorrowedTextureDescriptor(descriptor, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_metal_borrowed_texture_target(
            Handle,
            &nativeDescriptor,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered Metal surface replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_metal_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
    public Task SetMetalSurfaceTargetAsync(MetalSurfaceDescriptor descriptor)
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeMetalSurfaceDescriptor(descriptor);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_metal_surface_target(
            Handle,
            &nativeDescriptor,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered replacement of every texture of a caller-owned OpenGL
    /// ring.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_open_gl_borrowed_texture_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOpenGlBorrowedTextureTargetAsync(OpenGlBorrowedTextureDescriptor descriptor)
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeOpenGlBorrowedTextureDescriptor(descriptor, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_open_gl_borrowed_texture_target(
            Handle,
            &nativeDescriptor,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered OpenGL surface replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_open_gl_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    public Task SetOpenGlSurfaceTargetAsync(OpenGlSurfaceDescriptor descriptor)
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeOpenGlSurfaceDescriptor(descriptor, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_open_gl_surface_target(
            Handle,
            &nativeDescriptor,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
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
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public Task SetVulkanBorrowedTextureTargetAsync(VulkanBorrowedTextureDescriptor descriptor)
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeVulkanBorrowedTextureDescriptor(descriptor, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_vulkan_borrowed_texture_target(
            Handle,
            &nativeDescriptor,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts an ordered Vulkan surface replacement.
    /// </summary>
    /// <remarks>
    /// See <c>mln_render_session_set_vulkan_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public Task SetVulkanSurfaceTargetAsync(VulkanSurfaceDescriptor descriptor)
    {
        using var scope = new NativeCallScope(this);
        var nativeDescriptor = NativeVulkanSurfaceDescriptor(descriptor);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_vulkan_surface_target(
            Handle,
            &nativeDescriptor,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
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
        private readonly RenderSessionHandle owner;

        internal DiscardingCommands(RenderSessionHandle owner)
        {
            this.owner = owner;
        }

        public void Resize(LogicalExtent extent)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_render_session_resize(
                    receiverHandle,
                    NativeLogicalExtent(extent),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }
    }
}
