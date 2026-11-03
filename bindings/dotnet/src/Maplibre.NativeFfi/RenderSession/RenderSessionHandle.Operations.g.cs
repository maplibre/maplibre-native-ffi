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
    /// Stops the session right away, without running its pending graphics work.
    /// </summary>
    /// <remarks>
    /// <para>Use it when the target's graphics objects are going away and a
    /// detach can't finish first. Pending work completes with
    /// <c>Status.TargetLost</c>, and pending frame requests finish with
    /// <c>RenderResult.TargetNotReady</c>. Once the call returns, the library
    /// no longer uses your graphics objects, and you can attach another session
    /// to the map. Dispose the session afterward.</para>
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
    /// Takes a rendered frame from the session's texture ring, if one is ready.
    /// </summary>
    /// <remarks>
    /// <para>It takes the oldest frame that you haven't acquired. The session
    /// can render over a frame before you acquire it. While you hold a frame in
    /// every texture of the ring, new frames wait until you release one. When
    /// no frame is ready, it succeeds and writes the null handle.</para>
    /// <para>See <c>mln_render_session_acquire_frame</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the session lacks
    /// <c>RenderSessionCapabilityFlag.FrameAcquisition</c>.</description></item>
    /// <item><description><c>Status.InvalidState</c> when the session has
    /// failed.</description></item>
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
    /// Ends the session's pending frame requests and waits for its earlier
    /// work.
    /// </summary>
    /// <remarks>
    /// <para>Frame requests that haven't started, including those that wait for
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
    /// Detaches the session from its map and frees its graphics objects.
    /// </summary>
    /// <remarks>
    /// <para>Release every acquired frame first. Pending frame requests finish
    /// with <c>RenderResult.TargetNotReady</c>. Once the detach completes, the
    /// library no longer uses your graphics objects, and you can attach another
    /// session to the map. Dispose the session afterward.</para>
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
    /// Releases the session in any state.
    /// </summary>
    /// <remarks>
    /// <para>An attached session driven by <c>RenderDriverKind.CoreWorker</c>
    /// detaches first unless you hold one of its frames. Any other session is
    /// abandoned as <c>RenderSessionHandle.Abandon()</c> does. To free the
    /// graphics objects of a session driven by
    /// <c>RenderDriverKind.CallerGraphicsThread</c>, detach it before you
    /// dispose it. Once the disposal completes, the library no longer uses your
    /// graphics objects.</para>
    /// <para>See <c>mln_render_session_dispose</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
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
    /// Reads a value that a source computes for one of its features.
    /// </summary>
    /// <remarks>
    /// <para>A clustered GeoJSON source answers the <c>supercluster</c>
    /// extension for a cluster feature. Its <c>extensionField</c> is
    /// <c>children</c> for the cluster's child features, <c>leaves</c> for the
    /// points in the cluster, or <c>expansion-zoom</c> for the zoom level where
    /// the cluster splits. For <c>leaves</c>, <c>arguments</c> can set
    /// <c>limit</c> and <c>offset</c>. Without <c>limit</c>, the result has up
    /// to 10 points and ignores <c>offset</c>. The result is JSON: a GeoJSON
    /// feature collection, a number, or null when the source has no
    /// answer.</para>
    /// <para>See <c>mln_render_session_query_feature_extensions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.</para>
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
    /// Lets go of the session's surface or textures, and keeps the rest of the
    /// session.
    /// </summary>
    /// <remarks>
    /// <para>Use it when the platform takes a surface away, such as when your
    /// app goes to the background. Release every acquired frame first. Until
    /// you give the session a new target, frame requests finish with
    /// <c>RenderResult.TargetNotReady</c>. Once the release completes, you can
    /// destroy the old target.</para>
    /// <para>See <c>mln_render_session_release_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the session renders
    /// offscreen.</description></item>
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
    /// Asks the session to render a frame.
    /// </summary>
    /// <remarks>
    /// <para>Call it once the attach completes. Every request finishes with one
    /// frame result that carries its token, and
    /// <c>RenderSessionHandle.DrainFrameResults()</c> takes it. A request
    /// without <c>FrameDemandFlag.Force</c> replaces each pending request with
    /// the same flags.</para>
    /// <para>See <c>mln_render_session_request_frame</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> when the session has
    /// failed.</description></item>
    /// </list>
    /// </remarks>
    public void RequestFrame(FrameDemand demand)
    {
        using var call = Enter(this);
        var nativeDemand = NativeFrameDemand(demand);
        Check(NativeMethods.mln_render_session_request_frame(Handle, &nativeDemand, Diagnostic));
    }

    /// <summary>
    /// Resizes the session's target and its map.
    /// </summary>
    /// <remarks>
    /// <para><c>extent</c> must keep the session's scale factor. To change the
    /// scale factor, give the session a new target or attach a new session.
    /// Release every acquired frame first.</para>
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
    /// Runs the session's ready graphics work on your graphics thread.
    /// </summary>
    /// <remarks>
    /// <para>Use it with <c>RenderDriverKind.CallerGraphicsThread</c>, and call
    /// it from the same thread every time. Call it each time
    /// <c>RenderSessionAttachOptions.DriverWorkWake</c> is called, even while
    /// you request no frames, and until a detach completes. It returns once no
    /// work is ready.</para>
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
    /// Replaces the Metal textures that a borrowed ring renders into.
    /// </summary>
    /// <remarks>
    /// <para>Use it to resize the ring, or to give the session textures again
    /// after <c>RenderSessionHandle.ReleaseTargetAsync()</c>. Release every
    /// acquired frame first. The new ring must have as many textures as the old
    /// one, and the same device and command queue. Once the replacement
    /// completes, the session no longer uses the old textures.</para>
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
    /// Replaces the Metal layer that the session renders to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a layer again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, or to move it to
    /// another layer. The new layer must use the same device. Once the
    /// replacement completes, the session no longer uses the old layer.</para>
    /// <para>See <c>mln_render_session_set_metal_surface_target</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
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
    /// Replaces the OpenGL textures that a borrowed ring renders into.
    /// </summary>
    /// <remarks>
    /// <para>Use it to resize the ring, or to give the session textures again
    /// after <c>RenderSessionHandle.ReleaseTargetAsync()</c>. Release every
    /// acquired frame first. The new ring must have as many textures as the old
    /// one, and the same context. Once the replacement completes, the session
    /// no longer uses the old textures.</para>
    /// <para>See <c>mln_render_session_set_open_gl_borrowed_texture_target</c>
    /// in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
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
    /// Replaces the OpenGL surface that the session presents to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a surface again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, or to move it to
    /// another surface. The new surface must use the same context. Once the
    /// replacement completes, the session no longer uses the old
    /// surface.</para>
    /// <para>See <c>mln_render_session_set_open_gl_surface_target</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
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
    /// Replaces the Vulkan images that a borrowed ring renders into.
    /// </summary>
    /// <remarks>
    /// <para>Use it to resize the ring, or to give the session images again
    /// after <c>RenderSessionHandle.ReleaseTargetAsync()</c>. Release every
    /// acquired frame first. The new ring must have as many images as the old
    /// one, and the same context. Once the replacement completes, the session
    /// no longer uses the old images.</para>
    /// <para>See <c>mln_render_session_set_vulkan_borrowed_texture_target</c>
    /// in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the new ring has a
    /// different <c>format</c>, <c>final_layout</c>, or <c>owner</c> than the
    /// old one. Attach a new session to change them.</description></item>
    /// </list>
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
    /// Replaces the Vulkan surface that the session presents to.
    /// </summary>
    /// <remarks>
    /// <para>Use it to give the session a surface again after
    /// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, or to move it to
    /// another surface. The new surface must use the same context. Once the
    /// replacement completes, the session no longer uses the old
    /// surface.</para>
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
