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
    /// Gets the Android hardware buffer of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>Wait for <c>AcquiredFrameHandle.WithProducerSync()</c> before you
    /// read the buffer. Once you release or discard the frame, the session may
    /// render into the buffer again. A reference that you take with
    /// <c>AHardwareBuffer_acquire</c> keeps the buffer's memory, but not its
    /// contents.</para>
    /// <para>See <c>mln_acquired_frame_get_android_hardware_buffer</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session failed, or
    /// you detached, abandoned, or disposed it.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    public void WithAndroidHardwareBuffer(Action<AndroidHardwareBufferFrameView> callback)
    {
        using var call = Enter(this);
        var outFrame = new mln_android_hardware_buffer_frame
        {
            size = (uint)sizeof(mln_android_hardware_buffer_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_android_hardware_buffer(
                    Handle,
                    &outFrame,
                    Diagnostic
                )
            );
            callback(
                new AndroidHardwareBufferFrameView(
                    CopyAndroidHardwareBufferFrame(outFrame),
                    viewScope
                )
            );
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Gets the dmabuf of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>Wait for <c>AcquiredFrameHandle.WithProducerSync()</c> before you
    /// read the buffer. Once you release or discard the frame, the session may
    /// render into the buffer again. You can map a buffer with
    /// DRM_FORMAT_MOD_LINEAR with <c>mmap</c>, and bracket your reads with
    /// DMA_BUF_IOCTL_SYNC.</para>
    /// <para>The session owns the file descriptor, so don't close it. To keep
    /// the buffer after you release the frame, duplicate the file descriptor
    /// with <c>dup</c>. A duplicate keeps the buffer's memory, but not its
    /// contents.</para>
    /// <para>See <c>mln_acquired_frame_get_dma_buf</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/linux_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session failed, or
    /// you detached, abandoned, or disposed it.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("linux")]
    public void WithDmaBuf(Action<DmaBufFrameView> callback)
    {
        using var call = Enter(this);
        var outFrame = new mln_dma_buf_frame { size = (uint)sizeof(mln_dma_buf_frame) };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(NativeMethods.mln_acquired_frame_get_dma_buf(Handle, &outFrame, Diagnostic));
            callback(new DmaBufFrameView(CopyDmaBufFrame(outFrame), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Gets the DXGI shared texture of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>Wait for <c>AcquiredFrameHandle.WithProducerSync()</c> before you
    /// read the texture. Once you release or discard the frame, the session may
    /// render into the texture again. Open the handle with
    /// <c>ID3D11Device1::OpenSharedResource1</c> or
    /// <c>ID3D12Device::OpenSharedHandle</c>. A texture that you open keeps its
    /// memory, but not its contents.</para>
    /// <para>The session owns the handle, so don't close it. To keep the handle
    /// after you release the frame, duplicate it with
    /// <c>DuplicateHandle</c>.</para>
    /// <para>See <c>mln_acquired_frame_get_dxgi_shared_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.TargetLost</c> when the session failed, or
    /// you detached, abandoned, or disposed it.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("windows")]
    public void WithDxgiSharedTexture(Action<DxgiSharedTextureFrameView> callback)
    {
        using var call = Enter(this);
        var outFrame = new mln_dxgi_shared_texture_frame
        {
            size = (uint)sizeof(mln_dxgi_shared_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_dxgi_shared_texture(
                    Handle,
                    &outFrame,
                    Diagnostic
                )
            );
            callback(
                new DxgiSharedTextureFrameView(CopyDxgiSharedTextureFrame(outFrame), viewScope)
            );
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Gets the IOSurface of an acquired frame.
    /// </summary>
    /// <remarks>
    /// <para>Wait for <c>AcquiredFrameHandle.WithProducerSync()</c> before you
    /// read the surface. Once you release or discard the frame, the session may
    /// render into the surface again. For an API that takes a
    /// <c>CVPixelBuffer</c>, wrap the surface with
    /// <c>CVPixelBufferCreateWithIOSurface</c>. A reference that you take with
    /// <c>CFRetain</c>, or a pixel buffer that wraps the surface, keeps the
    /// surface's memory, but not its contents.</para>
    /// <para>See <c>mln_acquired_frame_get_io_surface</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html">C API reference</see>.</para>
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
    public void WithIoSurface(Action<IoSurfaceFrameView> callback)
    {
        using var call = Enter(this);
        var outFrame = new mln_io_surface_frame { size = (uint)sizeof(mln_io_surface_frame) };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(Handle, &token, Diagnostic));
        try
        {
            Check(NativeMethods.mln_acquired_frame_get_io_surface(Handle, &outFrame, Diagnostic));
            callback(new IoSurfaceFrameView(CopyIoSurfaceFrame(outFrame), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_acquired_frame_view_end(token);
        }
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
    /// texture finishes. Pass <c>GpuSyncKind.CpuComplete</c> when that work has
    /// already finished. The session waits for that point before it renders
    /// into the texture again. If that wait fails, such as for an OpenGL fence
    /// outside the session's share group, the session fails.</para>
    /// <para>See <c>mln_acquired_frame_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when
    /// <c>RenderSessionCapabilities.ConsumerSyncKinds</c> lacks the kind of
    /// <c>consumerCompletion</c>.</description></item>
    /// </list>
    /// </remarks>
    public void Release(GpuSync consumerCompletion)
    {
        using var scope = new NativeCallScope();
        var nativeConsumerCompletion = scope.Value(NativeGpuSync(consumerCompletion));
        state.Release(
            (live, diagnostic) =>
                NativeMethods.mln_acquired_frame_release(live, nativeConsumerCompletion, diagnostic)
        );
    }
}
