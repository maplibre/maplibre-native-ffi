using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class NativeStatusTests
{
    [Fact]
    public void NativeInvalidStatusMapsToExceptionWithCopiedDiagnostic()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        NativeLibraryLoader.EnsureLoaded();

        var error = Assert.Throws<InvalidArgumentException>(() =>
            NativeStatus.Check(NativeMethods.mln_network_status_set(999_999))
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Equal((int)mln_status.MLN_STATUS_INVALID_ARGUMENT, error.RawStatus);
        Assert.Contains("network status", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }
}
