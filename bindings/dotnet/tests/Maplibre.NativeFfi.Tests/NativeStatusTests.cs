using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class NativeStatusTests
{
    [Fact]
    public void AnUnknownStatusKeepsItsValueAndDiagnostic()
    {
        var error = Assert.Throws<MaplibreException>(() =>
            NativeStatus.Check(-12345, "mapped diagnostic", 0)
        );
        Assert.Equal((Status)(-12345), error.Status);
        Assert.Equal("mapped diagnostic", error.Diagnostic);
    }
}
