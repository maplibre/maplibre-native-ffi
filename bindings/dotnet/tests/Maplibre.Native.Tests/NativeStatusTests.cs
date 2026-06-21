using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class NativeStatusTests
{
    [BindingSpecTest("BND-020")]
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
    public void NativeStatusesMapToPublicExceptionCategories(
        int rawStatus,
        MaplibreStatus expectedStatus,
        Type expectedExceptionType
    )
    {
        using var diagnostics = NativeStatus.UseDiagnosticProviderForTest(() =>
            "mapped diagnostic"
        );

        var error = Assert.Throws(expectedExceptionType, () => NativeStatus.Check(rawStatus));
        var maplibreError = Assert.IsAssignableFrom<MaplibreException>(error);

        Assert.Equal(expectedStatus, maplibreError.Status);
        Assert.Equal(rawStatus, maplibreError.RawStatus);
        Assert.Equal("mapped diagnostic", maplibreError.Diagnostic);
    }

    [BindingSpecTest("BND-020", "BND-022")]
    [Fact]
    public void NativeInvalidStatusMapsToExceptionWithCopiedDiagnostic()
    {
        NativeLibraryLoader.EnsureLoaded();

        var error = Assert.Throws<InvalidArgumentException>(() =>
            NativeStatus.Check(NativeMethods.mln_network_status_set(999_999))
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Equal((int)mln_status.MLN_STATUS_INVALID_ARGUMENT, error.RawStatus);
        Assert.Contains("network status", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }

    [BindingSpecTest("BND-021")]
    [Fact]
    public void UnknownNativeStatusPreservesRawStatus()
    {
        using var diagnostics = NativeStatus.UseDiagnosticProviderForTest(() => "future status");

        var error = Assert.Throws<MaplibreException>(() => NativeStatus.Check(-12_345));

        Assert.Equal(MaplibreStatus.Unknown, error.Status);
        Assert.Equal(-12_345, error.RawStatus);
        Assert.Equal("future status", error.Diagnostic);
    }

    [BindingSpecTest("BND-022")]
    [Fact]
    public void DiagnosticIsCopiedBeforeLaterFailureChangesThreadLocalMessage()
    {
        var nextDiagnostic = "first diagnostic";
        using var diagnostics = NativeStatus.UseDiagnosticProviderForTest(() => nextDiagnostic);

        var first = Assert.Throws<NativeErrorException>(() =>
            NativeStatus.Check(mln_status.MLN_STATUS_NATIVE_ERROR)
        );

        nextDiagnostic = "second diagnostic";
        Assert.Throws<UnsupportedFeatureException>(() =>
            NativeStatus.Check(mln_status.MLN_STATUS_UNSUPPORTED)
        );

        Assert.Equal("first diagnostic", first.Diagnostic);
    }
}
