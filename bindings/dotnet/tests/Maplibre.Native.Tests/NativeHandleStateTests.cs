using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Handle;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class NativeHandleStateTests
{
    private static readonly Lock Gate = new();
    private static mln_status destroyStatus;
    private static int destroyCount;

    [Fact]
    public void CloseIsIdempotentAfterSuccess()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_OK;
        destroyCount = 0;
        var state = new NativeHandleState<mln_runtime>((mln_runtime*)1234, Destroy, "RuntimeHandle");

        state.Close();
        state.Close();

        Assert.True(state.IsClosed);
        Assert.Equal(1, destroyCount);
    }

    [Fact]
    public void FailedCloseKeepsHandleLiveForRetry()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_INVALID_STATE;
        destroyCount = 0;
        var state = new NativeHandleState<mln_runtime>((mln_runtime*)1234, Destroy, "RuntimeHandle");

        var error = Assert.Throws<InvalidStateException>(state.Close);

        Assert.Equal(MaplibreStatus.InvalidState, error.Status);
        Assert.False(state.IsClosed);
        Assert.Equal(1, destroyCount);

        destroyStatus = mln_status.MLN_STATUS_OK;
        state.Close();

        Assert.True(state.IsClosed);
        Assert.Equal(2, destroyCount);
    }

    [Fact]
    public void TryCloseSuppressesFailureWithoutClosingHandle()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_INVALID_STATE;
        destroyCount = 0;
        var state = new NativeHandleState<mln_runtime>((mln_runtime*)1234, Destroy, "RuntimeHandle");

        Assert.False(state.TryClose());
        Assert.False(state.IsClosed);
        Assert.Equal(1, destroyCount);
    }

    private static mln_status Destroy(mln_runtime* handle)
    {
        Assert.NotEqual((nint)0, (nint)handle);
        destroyCount++;
        return destroyStatus;
    }
}
