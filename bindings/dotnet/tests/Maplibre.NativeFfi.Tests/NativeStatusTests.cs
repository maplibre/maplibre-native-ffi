using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class NativeStatusTests
{
    [Theory]
    [InlineData(
        (int)mln_status.MLN_STATUS_INVALID_ARGUMENT,
        MaplibreStatus.InvalidArgument,
        typeof(InvalidArgumentException)
    )]
    [InlineData(
        (int)mln_status.MLN_STATUS_INVALID_STATE,
        MaplibreStatus.InvalidState,
        typeof(InvalidStateException)
    )]
    [InlineData(
        (int)mln_status.MLN_STATUS_WRONG_THREAD,
        MaplibreStatus.WrongThread,
        typeof(WrongThreadException)
    )]
    [InlineData(
        (int)mln_status.MLN_STATUS_UNSUPPORTED,
        MaplibreStatus.Unsupported,
        typeof(UnsupportedFeatureException)
    )]
    [InlineData(
        (int)mln_status.MLN_STATUS_NATIVE_ERROR,
        MaplibreStatus.NativeError,
        typeof(NativeErrorException)
    )]
    [InlineData((int)mln_status.MLN_STATUS_BUSY, MaplibreStatus.Busy, typeof(MaplibreException))]
    [InlineData(
        (int)mln_status.MLN_STATUS_TARGET_LOST,
        MaplibreStatus.TargetLost,
        typeof(MaplibreException)
    )]
    [InlineData(
        (int)mln_status.MLN_STATUS_NOT_READY,
        MaplibreStatus.NotReady,
        typeof(MaplibreException)
    )]
    [InlineData(
        (int)mln_status.MLN_STATUS_NOT_FOUND,
        MaplibreStatus.NotFound,
        typeof(MaplibreException)
    )]
    // A status this binding predates keeps its raw value.
    [InlineData(-12_345, MaplibreStatus.Unknown, typeof(MaplibreException))]
    public void NativeStatusesMapToPublicExceptionCategories(
        int rawStatus,
        MaplibreStatus expectedStatus,
        Type expectedExceptionType
    )
    {
        var error = Assert.Throws(
            expectedExceptionType,
            () => NativeStatus.Check(rawStatus, "mapped diagnostic")
        );
        var maplibreError = Assert.IsAssignableFrom<MaplibreException>(error);

        Assert.Equal(expectedStatus, maplibreError.Status);
        Assert.Equal(rawStatus, maplibreError.RawStatus);
        Assert.Equal("mapped diagnostic", maplibreError.Diagnostic);
    }

    [Fact]
    public void ANativeFailureRaisesItsStatusWithTheCallDiagnostic()
    {
        var error = Assert.Throws<InvalidArgumentException>(() =>
            Maplibre.NetworkSetStatus((NetworkStatus)999_999)
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Equal((int)mln_status.MLN_STATUS_INVALID_ARGUMENT, error.RawStatus);
        Assert.Contains("network status", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }
}
