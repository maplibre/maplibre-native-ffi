using Maplibre.Native.Error;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Render;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class MaplibreTests
{
    [BindingSpecTest("BND-001")]
    [Fact]
    public void CVersionComesFromNativeLibrary()
    {
        Assert.Equal(NativeLibraryLoader.ExpectedAbiVersion, Maplibre.CVersion());
    }

    [BindingSpecTest("BND-001")]
    [Fact]
    public void AbiVersionMismatchUsesStableBindingError()
    {
        var error = Assert.Throws<MaplibreException>(() =>
            NativeLibraryLoader.ValidateAbiVersion(NativeLibraryLoader.ExpectedAbiVersion + 1)
        );

        Assert.Equal(MaplibreStatus.AbiMismatch, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains(
            NativeLibraryLoader.ExpectedAbiVersion.ToString(),
            error.Diagnostic,
            StringComparison.Ordinal
        );
    }

    [BindingSpecTest("BND-160")]
    [Fact]
    public void SupportedOpenGLContextProvidersComeFromNativeLibrary()
    {
        var providers = Maplibre.SupportedOpenGLContextProviders();

        Assert.Equal(
            providers,
            providers & (OpenGLContextProvider.Wgl | OpenGLContextProvider.Egl)
        );
    }

    [BindingSpecTest("BND-103")]
    [Fact]
    public void ProjectionHelpersRoundTripThroughNativeLibrary()
    {
        var coordinate = new Geo.LatLng(45.0, -122.0);

        var meters = Maplibre.ProjectedMetersForLatLng(coordinate);
        var roundTripped = Maplibre.LatLngForProjectedMeters(meters);

        Assert.True(Math.Abs(roundTripped.Latitude - coordinate.Latitude) < 1e-9);
        Assert.True(Math.Abs(roundTripped.Longitude - coordinate.Longitude) < 1e-9);
    }

    [BindingSpecTest("BND-068")]
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
