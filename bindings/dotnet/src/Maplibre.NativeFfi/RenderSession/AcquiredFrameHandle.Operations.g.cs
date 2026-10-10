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
        Check(NativeMethods.mln_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
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
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

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
        Check(NativeMethods.mln_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
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
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

    public void WithProducerSync(Action<GpuSyncView> callback)
    {
        using var read = state.Read(this, "mln_acquired_frame_get_producer_sync");
        var outSync = new mln_gpu_sync { size = (uint)sizeof(mln_gpu_sync) };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        Check(NativeMethods.mln_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
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
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

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
        Check(NativeMethods.mln_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
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
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

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
        Check(NativeMethods.mln_acquired_frame_view_begin(read.Handle, &token, Diagnostic));
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
            NativeMethods.mln_acquired_frame_view_end(token);
        }
    }

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
