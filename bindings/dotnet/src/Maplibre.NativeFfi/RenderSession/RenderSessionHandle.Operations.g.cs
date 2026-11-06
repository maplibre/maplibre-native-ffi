// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A render session draws a map into a surface or a texture.
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
        Ready = completion;
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
        NativeMethods.mln_render_session_dispose(live, diagnostic);

    NativeHandleState<MlnRenderSession> INativeOwner<MlnRenderSession>.State => state;
    internal MlnRenderSession Handle => state.Handle;

    /// <summary>
    /// The native handle value. It stays readable after release.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    /// <summary>
    /// Releases the session in any state, and finishes in the background.
    /// </summary>
    /// <remarks>
    /// <para>Use it from destructors and finalizers, and
    /// <c>RenderSessionHandle.ReleaseAsync()</c> elsewhere. It ends the session
    /// as a release does, except that it abandons a session whose frames you
    /// hold, which ends those frames. Keep servicing a session driven by
    /// <c>RenderDriverKind.CallerGraphicsThread</c> until
    /// <c>RenderSessionHandle.ServiceDriverWork()</c> no longer accepts
    /// it.</para>
    /// <para>See <c>mln_render_session_dispose</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Stops the session right away, without running its pending graphics work.
    /// </summary>
    /// <remarks>
    /// <para>Use it when the target's graphics objects are going away and a
    /// detach can't finish first. Pending work completes with
    /// <c>Status.TargetLost</c>, and pending frame demands finish with
    /// <c>RenderResult.TargetNotReady</c>. Once the call returns, the library
    /// no longer uses your graphics objects, and you can attach another session
    /// to the map. Release the session afterward.</para>
    /// <para>Abandon keeps the session's OpenGL and WebGPU objects until the
    /// process exits, and reports how many it kept. Detach the session instead
    /// to free them.</para>
    /// <para>See <c>mln_render_session_abandon</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
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
    /// Takes the newest rendered frame from the session's texture ring, if one
    /// is ready.
    /// </summary>
    /// <remarks>
    /// <para>Older frames that you haven't acquired are dropped, and the
    /// session can render over a frame before you acquire it. While you hold a
    /// frame in every texture of the ring, new frames wait until you release
    /// one. When no frame is ready, it succeeds and writes the null
    /// handle.</para>
    /// <para>See <c>mln_render_session_acquire_frame</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the session lacks
    /// <c>RenderSessionCapabilityFlag.FrameAcquisition</c>.</description></item>
    /// <item><description><c>Status.TargetLost</c> or
    /// <c>Status.NativeError</c>, as <c>RenderSessionSnapshot.FailureStatus</c>
    /// reports, when the session has failed.</description></item>
    /// <item><description><c>Status.InvalidState</c> when you detached,
    /// abandoned, released, or disposed the session.</description></item>
    /// </list>
    /// </remarks>
    public AcquiredFrameHandle? AcquireFrame()
    {
        using var call = Enter(this);
        MlnAcquiredFrame outFrame = default;
        Check(NativeMethods.mln_render_session_acquire_frame(Handle, &outFrame, Diagnostic));
        if (outFrame.Value == 0)
            return null;
        return AcquiredFrameHandle.Adopt(this, outFrame);
    }

    /// <summary>
    /// Ends the session's pending frame demands and waits for its earlier work.
    /// </summary>
    /// <remarks>
    /// <para>Frame demands that haven't started, including those that wait for
    /// a map update, finish with <c>RenderResult.NoUpdate</c>.</para>
    /// <para>See <c>mln_render_session_barrier</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session loses its
    /// target before the barrier runs.</description></item>
    /// </list>
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
    /// Clears the data that the session's renderer holds, such as loaded tiles,
    /// and draws the map again.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_render_session_clear_data</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session loses its
    /// target before the work runs.</description></item>
    /// </list>
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
    /// Asks the session to render a frame.
    /// </summary>
    /// <remarks>
    /// <para>The frame shows the map commands that you sent before the call.
    /// The demand waits for the map to apply them, but not for an open command
    /// group to end.</para>
    /// <para>You can call it once the attach call returns. While the attach
    /// runs, the demand waits for it to complete. Every demand finishes with
    /// one frame result that carries its token, and
    /// <c>RenderSessionHandle.DrainFrameResults()</c> takes it. A demand
    /// without <c>FrameDemandFlag.Force</c> replaces each pending demand with
    /// the same flags.</para>
    /// <para>See <c>mln_render_session_demand_frame</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> or
    /// <c>Status.NativeError</c>, as <c>RenderSessionSnapshot.FailureStatus</c>
    /// reports, when the session has failed.</description></item>
    /// <item><description><c>Status.InvalidState</c> when you detached,
    /// abandoned, released, or disposed the session, or discarded a frame from
    /// every texture of its ring.</description></item>
    /// </list>
    /// </remarks>
    public void DemandFrame(FrameDemand demand)
    {
        using var call = Enter(this);
        var nativeDemand = NativeFrameDemand(demand);
        Check(NativeMethods.mln_render_session_demand_frame(Handle, &nativeDemand, Diagnostic));
    }

    /// <summary>
    /// Detaches the session from its map and frees its graphics objects.
    /// </summary>
    /// <remarks>
    /// <para>Release every acquired frame first. Pending frame demands finish
    /// with <c>RenderResult.TargetNotReady</c>. Once the detach completes, the
    /// library no longer uses your graphics objects, and you can attach another
    /// session to the map. Release the session afterward.</para>
    /// <para>See <c>mln_render_session_detach</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session loses its
    /// target before the detach runs.</description></item>
    /// </list>
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
    /// Takes the frame results that the session has queued.
    /// </summary>
    /// <remarks>
    /// <para>When no result is queued, it succeeds and writes the null handle
    /// and an empty view.</para>
    /// <para>See <c>mln_render_session_drain_frame_results</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// </remarks>
    public RenderFrameResult[] DrainFrameResults()
    {
        using var call = Enter(this);
        MlnRenderFrameBatch outBatch = default;
        var outView = new mln_render_frame_batch_view
        {
            size = (uint)sizeof(mln_render_frame_batch_view),
        };
        Check(
            NativeMethods.mln_render_session_drain_frame_results(
                Handle,
                &outBatch,
                &outView,
                Diagnostic
            )
        );
        if (outBatch.Value == 0)
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
    /// Logs debugging information about the session's renderer.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_render_session_dump_debug_logs</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session loses its
    /// target before the work runs.</description></item>
    /// </list>
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
    /// Reads what the session supports.
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
    /// Reads the children of a cluster in a clustered GeoJSON source.
    /// </summary>
    /// <remarks>
    /// <para><c>clusterId</c> is the "cluster_id" property of a cluster
    /// feature. The result is a GeoJSON feature collection of the clusters and
    /// points that the cluster splits into at the next zoom level.</para>
    /// <para>The query reads the source's data from the session's most recent
    /// render attempt, so a change to the data shows once the session tries to
    /// render again.</para>
    /// <para>A cluster ID belongs to one version of the source's data. After
    /// the data changes, feature queries can return IDs from the earlier
    /// version until the source's tiles reload, and such an ID completes with
    /// <c>Status.NotFound</c> or names a different cluster. Query the features
    /// again for current IDs.</para>
    /// <para>See <c>mln_render_session_get_cluster_children</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the session's most recent
    /// render attempt had no source named <c>sourceId</c>, or the source has no
    /// cluster <c>clusterId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source isn't a
    /// GeoJSON source with clustering turned on.</description></item>
    /// </list>
    /// </remarks>
    public Task<string> GetClusterChildrenAsync(string sourceId, uint clusterId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_get_cluster_children(
            Handle,
            scope.Utf8(sourceId),
            clusterId,
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
    /// Reads the zoom level at which a cluster of a clustered GeoJSON source
    /// splits.
    /// </summary>
    /// <remarks>
    /// <para><c>clusterId</c> is the "cluster_id" property of a cluster
    /// feature. At the result's zoom level, the cluster splits into two or more
    /// clusters or points.</para>
    /// <para>A cluster ID can go out of date as
    /// <c>RenderSessionHandle.GetClusterChildrenAsync()</c> describes.</para>
    /// <para>See <c>mln_render_session_get_cluster_expansion_zoom</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the session's most recent
    /// render attempt had no source named <c>sourceId</c>, or the source has no
    /// cluster <c>clusterId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source isn't a
    /// GeoJSON source with clustering turned on.</description></item>
    /// </list>
    /// </remarks>
    public Task<uint> GetClusterExpansionZoomAsync(string sourceId, uint clusterId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_get_cluster_expansion_zoom(
            Handle,
            scope.Utf8(sourceId),
            clusterId,
            NativeCompletion.Start<uint>(
                &nativeCompletion,
                static nativeResult => NativeCompletion.Value<uint>(nativeResult),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Reads the points in a cluster of a clustered GeoJSON source.
    /// </summary>
    /// <remarks>
    /// <para><c>clusterId</c> is the "cluster_id" property of a cluster
    /// feature. The result is a GeoJSON feature collection of the original
    /// point features. It skips the first <c>offset</c> points and then holds
    /// up to <c>limit</c> of them, so a <c>limit</c> of 0 gives an empty
    /// collection.</para>
    /// <para>A cluster ID can go out of date as
    /// <c>RenderSessionHandle.GetClusterChildrenAsync()</c> describes.</para>
    /// <para>See <c>mln_render_session_get_cluster_leaves</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the session's most recent
    /// render attempt had no source named <c>sourceId</c>, or the source has no
    /// cluster <c>clusterId</c>.</description></item>
    /// <item><description><c>Status.InvalidArgument</c> when the source isn't a
    /// GeoJSON source with clustering turned on.</description></item>
    /// </list>
    /// </remarks>
    public Task<string> GetClusterLeavesAsync(
        string sourceId,
        uint clusterId,
        uint limit,
        uint offset
    )
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_get_cluster_leaves(
            Handle,
            scope.Utf8(sourceId),
            clusterId,
            limit,
            offset,
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
    /// Reads the message of the error that made the session fail.
    /// </summary>
    /// <remarks>
    /// <para><c>RenderSessionSnapshot.FailureStatus</c> reports the error's
    /// status. The message is empty while the session hasn't failed, and
    /// doesn't change once it fails.</para>
    /// <para>See <c>mln_render_session_get_failure</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// </remarks>
    public string GetFailure()
    {
        using var scope = new NativeCallScope(this);
        nuint messageCapacity = 256;
        nuint outMessageLength = 0;
        var outMessage = scope.OutputArray<sbyte>((int)messageCapacity);
        while (true)
        {
            Check(
                NativeMethods.mln_render_session_get_failure(
                    Handle,
                    outMessage,
                    messageCapacity,
                    &outMessageLength,
                    Diagnostic
                )
            );
            if (outMessageLength < messageCapacity)
                break;
            messageCapacity = outMessageLength + 1;
            outMessage = scope.OutputArray<sbyte>(checked((int)messageCapacity));
        }
        scope.Accept();
        return RuntimeStructs.CopyUtf8(outMessage, outMessageLength);
    }

    /// <summary>
    /// Reads the session's current state.
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
    /// Finds the features that the session drew in an area of the screen.
    /// </summary>
    /// <remarks>
    /// <para>The query reads the last frame that the session rendered. A
    /// feature that several layers draw appears once for each layer.</para>
    /// <para>See <c>mln_render_session_query_rendered_features</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.</para>
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
    /// Finds the features of the source named <c>sourceId</c>.
    /// </summary>
    /// <remarks>
    /// <para>The query covers only the tiles that the source has loaded for the
    /// current view. A source loads tiles only while a visible layer uses
    /// it.</para>
    /// <para>See <c>mln_render_session_query_source_features</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.</para>
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
    /// Reads the pixels of the last frame that the session rendered.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_render_session_read_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the session's
    /// capabilities lack
    /// <c>RenderSessionCapabilityFlag.Readback</c>.</description></item>
    /// <item><description><c>Status.InvalidState</c> when the session hasn't
    /// rendered a frame since it attached, resized, or changed its
    /// target.</description></item>
    /// </list>
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
    /// Frees memory that the session's renderer can rebuild, such as caches.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_render_session_reduce_memory_use</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session loses its
    /// target before the work runs.</description></item>
    /// </list>
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
    /// Releases the session, detaching it first when it is attached.
    /// </summary>
    /// <remarks>
    /// <para>Pending frame demands finish with
    /// <c>RenderResult.TargetNotReady</c>. Once the release completes, the
    /// library no longer uses your graphics objects, and you can attach another
    /// session to the map.</para>
    /// <para>A session driven by <c>RenderDriverKind.CallerGraphicsThread</c>
    /// detaches at your next <c>RenderSessionHandle.ServiceDriverWork()</c>, so
    /// keep servicing it until the release completes. When the session isn't
    /// attached, such as while it attaches or after it fails, or when you
    /// release or dispose its map before that service, the release abandons it
    /// as <c>RenderSessionHandle.Abandon()</c> does.</para>
    /// <para>See <c>mln_render_session_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> while you hold one of the
    /// session's acquired frames. Release or discard them
    /// first.</description></item>
    /// </list>
    /// </remarks>
    public Task ReleaseAsync()
    {
        state.Close(idempotent: false);
        return teardown;
    }

    ValueTask IAsyncDisposable.DisposeAsync() => state.Close() ? new(teardown) : default;

    private mln_status StartRelease(MlnRenderSession handle, mln_diagnostic* _)
    {
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_release(
            handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        teardown = nativePending.Submitted(nativeStatus);
        return mln_status.MLN_STATUS_OK;
    }

    /// <summary>
    /// Lets go of the session's surface or textures, and keeps the rest of the
    /// session.
    /// </summary>
    /// <remarks>
    /// <para>Use it when the platform takes a surface away, such as when your
    /// app goes to the background. Release every acquired frame first. Frame
    /// demands that are still pending, and later ones until you give the
    /// session a new target, finish with <c>RenderResult.TargetNotReady</c>.
    /// Once the release completes, you can destroy the old target.</para>
    /// <para>See <c>mln_render_session_release_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the session renders
    /// offscreen or into Android hardware buffers, dmabufs, DXGI shared
    /// textures, or IOSurfaces that it allocates.</description></item>
    /// <item><description><c>Status.Unsupported</c> when the target is a WebGL
    /// canvas.</description></item>
    /// <item><description><c>Status.TargetLost</c> when the session loses its
    /// target before the release runs.</description></item>
    /// </list>
    /// </remarks>
    public Task ReleaseTargetAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_release_target(
            Handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Resizes the session's target and its map.
    /// </summary>
    /// <remarks>
    /// <para>The session renders at the map's scale factor. Release every
    /// acquired frame first.</para>
    /// <para>See <c>mln_render_session_resize</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the session renders
    /// into a borrowed ring. Give it new textures of the new size
    /// instead.</description></item>
    /// <item><description><c>Status.TargetLost</c> when the session loses its
    /// target before the resize applies.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> ResizeAsync(LogicalSize logicalSize)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_resize(
            Handle,
            NativeLogicalSize(logicalSize),
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Runs the session's ready graphics work on your graphics thread.
    /// </summary>
    /// <remarks>
    /// <para>Use it with <c>RenderDriverKind.CallerGraphicsThread</c>, and call
    /// it from the same thread every time. Call it each time
    /// <c>RenderSessionAttachOptions.DriverWorkWake</c> is called, even while
    /// you demand no frames, until a detach, release, or disposal of the
    /// session finishes. It returns once no work is ready.</para>
    /// <para>It accepts a released or disposed session until that release or
    /// disposal finishes.</para>
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
    /// Replaces the Android window that the session presents to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a window again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, to move it to another
    /// window, or to resize it. Once the replacement completes, the session no
    /// longer uses the old window.</para>
    /// <para>See <c>mln_render_session_set_android_surface_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when <c>surface</c> has
    /// no window, such as after the platform destroyed it.</description></item>
    /// <item><description><c>Status.Unsupported</c> when, in a Vulkan build,
    /// the new window needs a different color format or pre-rotation support
    /// than the old one. Attach a new session instead.</description></item>
    /// <item><description><c>Status.NativeError</c> when the library can't
    /// create the window's surface.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    public Task SetAndroidSurfaceTargetAsync(AndroidSurfaceReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeAndroidSurfaceReplacement(replacement);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_android_surface_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sets the frame pacer that demands the session's frames.
    /// </summary>
    /// <remarks>
    /// <para>At the first display refresh after the session has something new
    /// to draw, <c>pacer</c> demands a frame without flags and with token 0.
    /// One pacer can serve many sessions. A null <c>pacer</c> stops pacing the
    /// session. You can call it once the attach call returns.</para>
    /// <para>See <c>mln_render_session_set_frame_pacer</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> or
    /// <c>Status.NativeError</c>, as <c>RenderSessionSnapshot.FailureStatus</c>
    /// reports, when the session has failed.</description></item>
    /// <item><description><c>Status.InvalidState</c> when you attached the
    /// session with a redraw wake, or detached, abandoned, released, or
    /// disposed it.</description></item>
    /// </list>
    /// </remarks>
    public void SetFramePacer(FramePacerHandle? pacer = null)
    {
        using var scope = new NativeCallScope(this);
        Check(
            NativeMethods.mln_render_session_set_frame_pacer(
                Handle,
                scope.UseOptional(pacer),
                Diagnostic
            )
        );
        scope.Accept();
    }

    /// <summary>
    /// Replaces the Metal textures that a borrowed ring renders into.
    /// </summary>
    /// <remarks>
    /// <para>Use it to resize the ring, or to give the session textures again
    /// after <c>RenderSessionHandle.ReleaseTargetAsync()</c>. Release every
    /// acquired frame first. Once the replacement completes, the session no
    /// longer uses the old textures.</para>
    /// <para>See <c>mln_render_session_set_metal_borrowed_texture_target</c> in
    /// the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the new textures have
    /// a different pixel format than the old ones. Attach a new session to
    /// change it.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
    public Task SetMetalBorrowedTextureTargetAsync(MetalBorrowedTextureReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeMetalBorrowedTextureReplacement(replacement, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_metal_borrowed_texture_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the Metal layer that the session renders to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a layer again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, to move it to another
    /// layer, or to resize it. Once the replacement completes, the session no
    /// longer uses the old layer.</para>
    /// <para>See <c>mln_render_session_set_metal_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
    public Task SetMetalSurfaceTargetAsync(MetalSurfaceReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeMetalSurfaceReplacement(replacement);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_metal_surface_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the OpenGL textures that a borrowed ring renders into.
    /// </summary>
    /// <remarks>
    /// <para>Use it to resize the ring, or to give the session textures again
    /// after <c>RenderSessionHandle.ReleaseTargetAsync()</c>. Release every
    /// acquired frame first. Once the replacement completes, the session no
    /// longer uses the old textures.</para>
    /// <para>See <c>mln_render_session_set_open_gl_borrowed_texture_target</c>
    /// in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task SetOpenGlBorrowedTextureTargetAsync(OpenGlBorrowedTextureReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeOpenGlBorrowedTextureReplacement(replacement, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_open_gl_borrowed_texture_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the OpenGL surface that the session presents to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a surface again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, to move it to another
    /// surface, or to resize it. Once the replacement completes, the session no
    /// longer uses the old surface.</para>
    /// <para>See <c>mln_render_session_set_open_gl_surface_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task SetOpenGlSurfaceTargetAsync(OpenGlSurfaceReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeOpenGlSurfaceReplacement(replacement);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_open_gl_surface_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the Vulkan images that a borrowed ring renders into.
    /// </summary>
    /// <remarks>
    /// <para>Use it to resize the ring, or to give the session images again
    /// after <c>RenderSessionHandle.ReleaseTargetAsync()</c>. Release every
    /// acquired frame first. Once the replacement completes, the session no
    /// longer uses the old images.</para>
    /// <para>See <c>mln_render_session_set_vulkan_borrowed_texture_target</c>
    /// in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public Task SetVulkanBorrowedTextureTargetAsync(VulkanBorrowedTextureReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeVulkanBorrowedTextureReplacement(replacement, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_vulkan_borrowed_texture_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the Vulkan surface that the session presents to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a surface again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, to move it to another
    /// surface, or to resize it. Once the replacement completes, the session no
    /// longer uses the old surface.</para>
    /// <para>See <c>mln_render_session_set_vulkan_surface_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the new surface needs
    /// a different color format or pre-rotation support than the old one.
    /// Attach a new session instead.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public Task SetVulkanSurfaceTargetAsync(VulkanSurfaceReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeVulkanSurfaceReplacement(replacement);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_vulkan_surface_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the Wayland surface that the session presents to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a surface again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, to move it to another
    /// surface, or to resize it. Once the replacement completes, the session no
    /// longer uses the old surface.</para>
    /// <para>See <c>mln_render_session_set_wayland_surface_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when, in a Vulkan build,
    /// the new surface needs a different color format than the old one. Attach
    /// a new session instead.</description></item>
    /// <item><description><c>Status.NativeError</c> when the library can't
    /// present to the new surface.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    public Task SetWaylandSurfaceTargetAsync(WaylandSurfaceReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeWaylandSurfaceReplacement(replacement);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_wayland_surface_target(
            Handle,
            &nativeReplacement,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the Win32 window that the session presents to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a window again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, to move it to another
    /// window, or to resize it. Once the replacement completes, the session no
    /// longer uses the old window.</para>
    /// <para>See <c>mln_render_session_set_win32_window_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when <c>window</c>
    /// isn't a window, or, in an OpenGL build, already has another pixel
    /// format.</description></item>
    /// <item><description><c>Status.InvalidState</c> when <c>window</c> is
    /// minimized or has no client area.</description></item>
    /// <item><description><c>Status.InvalidState</c> when <c>window</c> was
    /// minimized before the replacement ran.</description></item>
    /// <item><description><c>Status.Unsupported</c> when, in a Vulkan build,
    /// the new window needs a different color format than the old one. Attach a
    /// new session instead.</description></item>
    /// <item><description><c>Status.NativeError</c> when the library can't
    /// present to the new window.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public Task SetWin32WindowTargetAsync(Win32WindowReplacement replacement)
    {
        using var scope = new NativeCallScope(this);
        var nativeReplacement = NativeWin32WindowReplacement(replacement);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_render_session_set_win32_window_target(
            Handle,
            &nativeReplacement,
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

        public void Resize(LogicalSize logicalSize)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_render_session_resize(
                    receiverHandle,
                    NativeLogicalSize(logicalSize),
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }
    }
}
