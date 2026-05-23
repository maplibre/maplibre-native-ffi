using Maplibre.Native.Error;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class MaplibreTests
{
    [Fact]
    public void CVersionComesFromNativeLibrary()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();

        Assert.Equal(0u, Maplibre.CVersion());
    }

    [Fact]
    public void UnknownNetworkStatusIsRejectedBeforeNativeCall()
    {
        var status = NetworkStatus.FromRaw(999_999);

        var error = Assert.Throws<InvalidArgumentException>(() => Maplibre.SetNetworkStatus(status));

        Assert.Equal(MaplibreStatus.Unknown, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains("999999", error.Diagnostic, StringComparison.Ordinal);
    }
}
