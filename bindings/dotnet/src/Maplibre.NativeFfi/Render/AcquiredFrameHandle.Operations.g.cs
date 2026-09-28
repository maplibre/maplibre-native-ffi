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

public sealed unsafe partial class AcquiredFrameHandle : IDisposable
{
    private readonly NativeHandleState<MlnAcquiredFrame> state;
    private readonly ulong nativeId;
    internal ulong NativeId => nativeId;

    internal AcquiredFrameHandle(RenderSessionHandle parent, MlnAcquiredFrame handle)
    {
        nativeId = handle.Value;
        state = new NativeHandleState<MlnAcquiredFrame>(
            handle,
            static live => NativeMethods.mln_acquired_frame_dispose(live),
            nameof(AcquiredFrameHandle),
            static live => NativeMethods.mln_acquired_frame_dispose(live),
            retainedParent: parent
        );
    }

    internal static AcquiredFrameHandle Adopt(RenderSessionHandle parent, MlnAcquiredFrame handle)
    {
        AcquiredFrameHandle? owner = null;
        try
        {
            owner = new AcquiredFrameHandle(parent, handle);
            return owner;
        }
        catch
        {
            if (owner is null)
                NativeMethods.mln_acquired_frame_dispose(handle);
            else
                owner.state.Retire();
            throw;
        }
    }

    internal MlnAcquiredFrame Handle => state.Handle;

    internal NativeHandleState<MlnAcquiredFrame>.ReadScope Borrow() => state.Borrow();

    internal global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackOwner CallbackOwner =>
        state.CallbackOwner;
    public bool IsClosed => state.IsClosed;

    public void Dispose()
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_dispose"
        );
        state.Retire();
    }

    public void WithMetalTexture(Action<MetalOwnedTextureFrameView> callback)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_get_metal_texture"
        );
        var outFrame = new mln_metal_owned_texture_frame
        {
            size = (uint)sizeof(mln_metal_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        NativeStatus.Check(
            NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token)
        );
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_metal_texture(read.Handle, &outFrame)
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

    public void WithOpenglTexture(Action<OpenglOwnedTextureFrameView> callback)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_get_opengl_texture"
        );
        var outFrame = new mln_opengl_owned_texture_frame
        {
            size = (uint)sizeof(mln_opengl_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        NativeStatus.Check(
            NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token)
        );
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_opengl_texture(read.Handle, &outFrame)
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

    public void WithProducerSync(Action<GpuSyncView> callback)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_get_producer_sync"
        );
        var outSync = new mln_gpu_sync { size = (uint)sizeof(mln_gpu_sync) };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        NativeStatus.Check(
            NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token)
        );
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_producer_sync(read.Handle, &outSync)
            );
            callback(new GpuSyncView(CopyGpuSync(outSync), viewScope));
        }
        finally
        {
            viewScope.Expire();
            NativeMethods.mln_adapter_acquired_frame_view_end(token);
        }
    }

    public RenderFrameResult GetResult()
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_get_result"
        );
        var outResult = new mln_render_frame_result
        {
            size = (uint)sizeof(mln_render_frame_result),
        };
        NativeStatus.Check(NativeMethods.mln_acquired_frame_get_result(read.Handle, &outResult));
        return CopyRenderFrameResult(outResult);
    }

    public void WithVulkanTexture(Action<VulkanOwnedTextureFrameView> callback)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_get_vulkan_texture"
        );
        var outFrame = new mln_vulkan_owned_texture_frame
        {
            size = (uint)sizeof(mln_vulkan_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        NativeStatus.Check(
            NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token)
        );
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_vulkan_texture(read.Handle, &outFrame)
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

    public void WithWebgpuTexture(Action<WebgpuOwnedTextureFrameView> callback)
    {
        using var read = state.Borrow();
        using var retained = this.state.Retain();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_get_webgpu_texture"
        );
        var outFrame = new mln_webgpu_owned_texture_frame
        {
            size = (uint)sizeof(mln_webgpu_owned_texture_frame),
        };
        ArgumentNullException.ThrowIfNull(callback);
        var viewScope = new NativeViewScope();
        void* token = null;
        NativeStatus.Check(
            NativeMethods.mln_adapter_acquired_frame_view_begin(read.Handle, &token)
        );
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_acquired_frame_get_webgpu_texture(read.Handle, &outFrame)
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

    public void Release(GpuSync consumerCompletion)
    {
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_acquired_frame_release"
        );
        state.Release(live =>
        {
            var nativeConsumerCompletion = NativeGpuSync(consumerCompletion);
            return NativeMethods.mln_acquired_frame_release(&live, &nativeConsumerCompletion);
        });
    }
}
