using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Render;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class MaplibreTests
{
    [Fact]
    public void CVersionComesFromNativeLibrary()
    {
        Assert.Equal(NativeLibraryLoader.ExpectedAbiVersion, Maplibre.CVersion());
    }

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

    [Fact]
    public void SupportedOpenGLContextProvidersComeFromNativeLibrary()
    {
        var providers = Maplibre.OpenglSupportedContextProviderMask();

        Assert.Equal(
            providers,
            providers
                & (
                    OpenglContextProviderFlag.Wgl
                    | OpenglContextProviderFlag.Egl
                    | OpenglContextProviderFlag.Webgl
                )
        );
    }

    [Fact]
    public void ProjectionHelpersRoundTripThroughNativeLibrary()
    {
        var coordinate = new Map.LatLng(45.0, -122.0);

        var meters = Maplibre.ProjectedMetersForLatLng(coordinate);
        var roundTripped = Maplibre.LatLngForProjectedMeters(meters);

        Assert.True(Math.Abs(roundTripped.Latitude - coordinate.Latitude) < 1e-9);
        Assert.True(Math.Abs(roundTripped.Longitude - coordinate.Longitude) < 1e-9);
    }

    [Fact]
    public void UnknownNetworkStatusIsRejectedByNativeValidation()
    {
        var status = (Runtime.NetworkStatus)999_999;

        var error = Assert.Throws<InvalidArgumentException>(() =>
            Maplibre.NetworkStatusSet(status)
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Equal(-1, error.RawStatus);
        Assert.NotNull(error.Diagnostic);
    }
}
