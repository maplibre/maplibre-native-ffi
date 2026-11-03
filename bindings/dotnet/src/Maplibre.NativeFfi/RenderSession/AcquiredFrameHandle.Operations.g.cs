// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A rendered frame that the caller holds to read or show.
/// </summary>
/// <remarks>
/// See <c>mln_acquired_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class AcquiredFrameHandle : INativeOwner<MlnAcquiredFrame>
{
    private readonly NativeHandleState<MlnAcquiredFrame> state;

    internal AcquiredFrameHandle(RenderSessionHandle parent, MlnAcquiredFrame handle)
    {
        state = new(handle, Abandon, nameof(AcquiredFrameHandle), Abandon, retainedParent: parent);
    }

    internal static AcquiredFrameHandle Adopt(
        RenderSessionHandle parent,
        MlnAcquiredFrame handle
    ) =>
        NativeHandleState<MlnAcquiredFrame>.Adopt(
            handle,
            () => new AcquiredFrameHandle(parent, handle),
            Abandon
        );

    private static mln_status Abandon(MlnAcquiredFrame live, mln_diagnostic* diagnostic) =>
        NativeMethods.mln_acquired_frame_discard(live, diagnostic);

    NativeHandleState<MlnAcquiredFrame> INativeOwner<MlnAcquiredFrame>.State => state;
    internal MlnAcquiredFrame Handle => state.Handle;

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

    /// <summary>
    /// Gives up an acquired frame without saying when your GPU stops reading
    /// it.
    /// </summary>
    /// <remarks>
    /// <para>Because your GPU may still read the frame's texture, the session
    /// never renders into that texture again.</para>
    /// <para>See <c>mln_acquired_frame_discard</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Discard()
    {
        state.Retire();
    }

    /// <summary>
    /// Gets the Metal texture of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_acquired_frame_get_metal_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session failed, or
    /// you detached, abandoned, or disposed it.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
    public void WithMetalTexture(Action<MetalTextureFrameView> callback)
    {
        using var call = Enter(this);
        var outFrame = new mln_metal_texture_frame { size = (uint)sizeof(mln_metal_texture_frame) };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_metal_texture(Handle, &outFrame, Diagnostic)
            );
            callback(new MetalTextureFrameView(CopyMetalTextureFrame(outFrame), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Gets the OpenGL texture of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_acquired_frame_get_open_gl_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session failed, or
    /// you detached, abandoned, or disposed it.</description></item>
    /// </list>
    /// </remarks>
    public void WithOpenGlTexture(Action<OpenGlTextureFrameView> callback)
    {
        using var call = Enter(this);
        var outFrame = new mln_open_gl_texture_frame
        {
            size = (uint)sizeof(mln_open_gl_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_open_gl_texture(Handle, &outFrame, Diagnostic)
            );
            callback(new OpenGlTextureFrameView(CopyOpenGlTextureFrame(outFrame), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Reads what to wait for before you read an acquired frame's texture.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_acquired_frame_get_producer_sync</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session failed, or
    /// you detached, abandoned, or disposed it.</description></item>
    /// </list>
    /// </remarks>
    public void WithProducerSync(Action<GpuSyncView> callback)
    {
        using var call = Enter(this);
        var outSync = new mln_gpu_sync { size = (uint)sizeof(mln_gpu_sync) };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(NativeMethods.mln_acquired_frame_get_producer_sync(Handle, &outSync, Diagnostic));
            callback(new GpuSyncView(CopyGpuSync(outSync), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Reads the frame result of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>It succeeds until you release or discard the frame, whatever the
    /// session's state.</para>
    /// <para>See <c>mln_acquired_frame_get_result</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// </remarks>
    public RenderFrameResult GetResult()
    {
        using var call = Enter(this);
        var outResult = new mln_render_frame_result
        {
            size = (uint)sizeof(mln_render_frame_result),
        };
        Check(NativeMethods.mln_acquired_frame_get_result(Handle, &outResult, Diagnostic));
        return CopyRenderFrameResult(outResult);
    }

    /// <summary>
    /// Gets the Vulkan image of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_acquired_frame_get_vulkan_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session failed, or
    /// you detached, abandoned, or disposed it.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public void WithVulkanTexture(Action<VulkanTextureFrameView> callback)
    {
        using var call = Enter(this);
        var outFrame = new mln_vulkan_texture_frame
        {
            size = (uint)sizeof(mln_vulkan_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_vulkan_texture(Handle, &outFrame, Diagnostic)
            );
            callback(new VulkanTextureFrameView(CopyVulkanTextureFrame(outFrame), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Gives an acquired frame back to the session once you're done reading it.
    /// </summary>
    /// <remarks>
    /// <para><c>consumerCompletion</c> says when your GPU work that reads the
    /// texture finishes. Null means that the work has already finished. The
    /// session waits for that point before it renders into the texture
    /// again.</para>
    /// <para>See <c>mln_acquired_frame_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>RenderSessionCapabilities.ConsumerSyncKinds</c> lacks the kind of
    /// <c>consumerCompletion</c>.</description></item>
    /// </list>
    /// </remarks>
    public void Release(GpuSync? consumerCompletion)
    {
        using var scope = new NativeCallScope();
        var nativeConsumerCompletion = consumerCompletion is null
            ? null
            : scope.Value(NativeGpuSync(consumerCompletion.Value));
        state.Release(
            (live, diagnostic) =>
                NativeMethods.mln_acquired_frame_release(live, nativeConsumerCompletion, diagnostic)
        );
    }
}
