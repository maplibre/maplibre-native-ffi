using System.Runtime.CompilerServices;
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
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
            Destroy,
            "RuntimeHandle"
        );

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
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
            Destroy,
            "RuntimeHandle"
        );

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
        var reports = new List<NativeLeakReport>();
        using var capture = NativeLeakReporter.CaptureForTest(reports.Add);
        var state = new NativeHandleState<mln_runtime>(
            (mln_runtime*)1234,
            Destroy,
            "RuntimeHandle"
        );

        Assert.False(state.TryClose());

        Assert.False(state.IsClosed);
        Assert.Equal(1, destroyCount);
        var report = Assert.Single(reports);
        Assert.Equal(NativeLeakReportKind.DisposeFailed, report.Kind);
        Assert.Equal("RuntimeHandle", report.TypeName);
        Assert.Equal((nint)1234, report.Address);
        Assert.Equal(mln_status.MLN_STATUS_INVALID_STATE, report.Status);

        destroyStatus = mln_status.MLN_STATUS_OK;
        state.Close();
    }

    [Fact]
    public void FinalizerReportsLeakedLiveHandleWithoutDestroyingIt()
    {
        using var _ = Gate.EnterScope();
        destroyStatus = mln_status.MLN_STATUS_OK;
        destroyCount = 0;
        var reports = new List<NativeLeakReport>();
        using var capture = NativeLeakReporter.CaptureForTest(reports.Add);

        CreateLeakedState();
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();

        var report = Assert.Single(reports);
        Assert.Equal(NativeLeakReportKind.LeakedHandle, report.Kind);
        Assert.Equal("RuntimeHandle", report.TypeName);
        Assert.Equal((nint)5678, report.Address);
        Assert.Null(report.Status);
        Assert.Equal(0, destroyCount);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void CreateLeakedState()
    {
        _ = new NativeHandleState<mln_runtime>((mln_runtime*)5678, Destroy, "RuntimeHandle");
    }

    private static mln_status Destroy(mln_runtime* handle)
    {
        Assert.NotEqual((nint)0, (nint)handle);
        destroyCount++;
        return destroyStatus;
    }
}
