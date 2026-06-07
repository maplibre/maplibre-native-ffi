using Maplibre.Native.Error;
using Maplibre.Native.Render;
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
    public void SupportedOpenGLContextProvidersComeFromNativeLibrary()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();

        var providers = Maplibre.SupportedOpenGLContextProviders();

        Assert.Equal(
            providers,
            providers & (OpenGLContextProvider.Wgl | OpenGLContextProvider.Egl)
        );
    }

    [Fact]
    public void ProjectionHelpersRoundTripThroughNativeLibrary()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        var coordinate = new Geo.LatLng(45.0, -122.0);

        var meters = Maplibre.ProjectedMetersForLatLng(coordinate);
        var roundTripped = Maplibre.LatLngForProjectedMeters(meters);

        Assert.True(Math.Abs(roundTripped.Latitude - coordinate.Latitude) < 1e-9);
        Assert.True(Math.Abs(roundTripped.Longitude - coordinate.Longitude) < 1e-9);
    }

    [Fact]
    public void UnknownNetworkStatusIsRejectedBeforeNativeCall()
    {
        var status = NetworkStatus.FromRaw(999_999);

        var error = Assert.Throws<InvalidArgumentException>(() =>
            Maplibre.SetNetworkStatus(status)
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains("999999", error.Diagnostic, StringComparison.Ordinal);
    }
}
