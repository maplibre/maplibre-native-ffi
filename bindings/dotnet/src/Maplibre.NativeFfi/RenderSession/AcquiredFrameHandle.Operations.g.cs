// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class AcquiredFrameHandle : IDisposable, INativeOwner<MlnAcquiredFrame>
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
        NativeMethods.mln_acquired_frame_dispose(live, diagnostic);

    NativeHandleState<MlnAcquiredFrame> INativeOwner<MlnAcquiredFrame>.State => state;
    internal MlnAcquiredFrame Handle => state.Handle;
    internal NativeCallbackOwner CallbackOwner => state.CallbackOwner;

    // Runtime events report their source by this identity.
    public ulong Id => state.IssuedHandle.Value;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_acquired_frame_dispose");
        state.Retire();
    }

    /// <summary>
    /// Copies Metal-native metadata from an acquired frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_acquired_frame_get_metal_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public void WithMetalTexture(Action<MetalOwnedTextureFrameView> callback)
    {
        using var read = state.Read(this, "mln_acquired_frame_get_metal_texture");
        var outFrame = new mln_metal_owned_texture_frame
        {
            size = (uint)sizeof(mln_metal_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_metal_texture(
                    read.Handle,
                    &outFrame,
                    Diagnostic
                )
            );
            callback(
                new MetalOwnedTextureFrameView(CopyMetalOwnedTextureFrame(outFrame), viewScope)
            );
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_adapter_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Copies OpenGL-native metadata from an acquired frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_acquired_frame_get_opengl_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public void WithOpenglTexture(Action<OpenglOwnedTextureFrameView> callback)
    {
        using var read = state.Read(this, "mln_acquired_frame_get_opengl_texture");
        var outFrame = new mln_opengl_owned_texture_frame
        {
            size = (uint)sizeof(mln_opengl_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_opengl_texture(
                    read.Handle,
                    &outFrame,
                    Diagnostic
                )
            );
            callback(
                new OpenglOwnedTextureFrameView(CopyOpenglOwnedTextureFrame(outFrame), viewScope)
            );
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_adapter_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Copies the producer synchronization for an acquired texture frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_acquired_frame_get_producer_sync</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void WithProducerSync(Action<GpuSyncView> callback)
    {
        using var read = state.Read(this, "mln_acquired_frame_get_producer_sync");
        var outSync = new mln_gpu_sync { size = (uint)sizeof(mln_gpu_sync) };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_producer_sync(
                    read.Handle,
                    &outSync,
                    Diagnostic
                )
            );
            callback(new GpuSyncView(CopyGpuSync(outSync), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_adapter_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Copies common metadata for an acquired frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_acquired_frame_get_result</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public RenderFrameResult GetResult()
    {
        using var read = state.Read(this, "mln_acquired_frame_get_result");
        var outResult = new mln_render_frame_result
        {
            size = (uint)sizeof(mln_render_frame_result),
        };
        Check(NativeMethods.mln_acquired_frame_get_result(read.Handle, &outResult, Diagnostic));
        return CopyRenderFrameResult(outResult);
    }

    /// <summary>
    /// Copies Vulkan-native metadata from an acquired frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_acquired_frame_get_vulkan_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public void WithVulkanTexture(Action<VulkanOwnedTextureFrameView> callback)
    {
        using var read = state.Read(this, "mln_acquired_frame_get_vulkan_texture");
        var outFrame = new mln_vulkan_owned_texture_frame
        {
            size = (uint)sizeof(mln_vulkan_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_vulkan_texture(
                    read.Handle,
                    &outFrame,
                    Diagnostic
                )
            );
            callback(
                new VulkanOwnedTextureFrameView(CopyVulkanOwnedTextureFrame(outFrame), viewScope)
            );
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_adapter_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Copies WebGPU-native metadata from an acquired frame.
    /// </summary>
    /// <remarks>
    /// See <c>mln_acquired_frame_get_webgpu_texture</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
    /// </remarks>
    public void WithWebgpuTexture(Action<WebgpuOwnedTextureFrameView> callback)
    {
        using var read = state.Read(this, "mln_acquired_frame_get_webgpu_texture");
        var outFrame = new mln_webgpu_owned_texture_frame
        {
            size = (uint)sizeof(mln_webgpu_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
        try
        {
            Check(
                NativeMethods.mln_acquired_frame_get_webgpu_texture(
                    read.Handle,
                    &outFrame,
                    Diagnostic
                )
            );
            callback(
                new WebgpuOwnedTextureFrameView(CopyWebgpuOwnedTextureFrame(outFrame), viewScope)
            );
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_adapter_acquired_frame_view_end(token);
        }
    }

    /// <summary>
    /// Releases an acquired frame after optional consumer GPU work.
    /// </summary>
    /// <remarks>
    /// See <c>mln_acquired_frame_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
    /// </remarks>
    public void Release(GpuSync consumerCompletion)
    {
        NativeCallbackGuard.EnsureAllowed(this, "mln_acquired_frame_release");
        using var scope = new NativeCallScope();
        state.Release(
            (live, diagnostic) =>
                NativeMethods.mln_acquired_frame_release(
                    &live,
                    scope.Value(NativeGpuSync(consumerCompletion)),
                    diagnostic
                )
        );
    }
}
