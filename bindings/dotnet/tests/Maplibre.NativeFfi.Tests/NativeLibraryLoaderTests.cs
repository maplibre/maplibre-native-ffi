using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.Loader;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class NativeLibraryLoaderTests
{
    // Validation runs on the version export before the loader caches the handle, so a library
    // with another ABI never serves a call.
    [Fact]
    public void ALibraryWithAnotherAbiVersionIsRejected()
    {
        var error = Assert.Throws<MaplibreException>(() =>
            NativeLibraryLoader.ValidateAbiVersion(NativeLibraryLoader.ExpectedAbiVersion + 1)
        );

        Assert.Equal(MaplibreStatus.AbiMismatch, error.Status);
        Assert.Null(error.RawStatus);
        Assert.Contains(
            $"expected {NativeLibraryLoader.ExpectedAbiVersion}",
            error.Diagnostic,
            StringComparison.Ordinal
        );
    }

    // The suite's module initializer resolves the library through the standard lookup, so an
    // explicit path can no longer choose a different one.
    [Fact]
    public void AnExplicitPathAfterTheStandardLookupIsRefused()
    {
        var error = Assert.Throws<InvalidOperationException>(() =>
            Maplibre.LoadNativeLibrary(Path.Combine(AppContext.BaseDirectory, "missing-library"))
        );

        Assert.Contains("already been resolved", error.Message, StringComparison.Ordinal);
    }
}
