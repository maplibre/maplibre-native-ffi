using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Internal.Callback;

internal sealed unsafe class ResourceProviderState : IDisposable
{
    private readonly ResourceProviderCallback callback;
    private nint handle;

    internal ResourceProviderState(ResourceProviderCallback callback)
    {
        this.callback = callback ?? throw new ArgumentNullException(nameof(callback));
        handle = GCHandle.ToIntPtr(GCHandle.Alloc(this));
    }

    internal mln_resource_provider Descriptor =>
        new()
        {
            size = (uint)sizeof(mln_resource_provider),
            callback = &OnRequest,
            user_data = (void*)handle,
        };

    internal uint HandleForTest(mln_resource_request* request)
    {
        return Invoke(this, request, (mln_resource_request_handle*)1);
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static uint OnRequest(
        void* userData,
        mln_resource_request* request,
        mln_resource_request_handle* requestHandle
    )
    {
        try
        {
            var state = (ResourceProviderState?)GCHandle.FromIntPtr((nint)userData).Target;
            return Invoke(state, request, requestHandle);
        }
        catch
        {
            return uint.MaxValue;
        }
    }

    private static uint Invoke(
        ResourceProviderState? state,
        mln_resource_request* request,
        mln_resource_request_handle* requestHandle
    )
    {
        if (state is null || request is null || requestHandle is null)
        {
            return uint.MaxValue;
        }

        var handle = new ResourceRequestHandle(requestHandle);
        try
        {
            var copiedRequest = ResourceStructs.ReadRequest(request);
            var decision = state.callback(copiedRequest, handle);
            return handle.FinishProviderDecision(decision);
        }
        catch
        {
            return handle.FinishProviderException();
        }
    }

    public void Dispose()
    {
        var current = System.Threading.Interlocked.Exchange(ref handle, 0);
        if (current != 0)
        {
            GCHandle.FromIntPtr(current).Free();
        }
    }
}
