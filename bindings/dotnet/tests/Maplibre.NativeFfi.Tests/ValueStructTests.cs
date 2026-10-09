using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class ValueStructTests
{
    [Fact]
    public void EmptyBuffersHavePresentStorage()
    {
        using var scope = new NativeCallScope();
        var text = scope.Utf8("");
        var bytes = scope.Buffer([]);
        Assert.True(text.data != null);
        Assert.True(bytes.data != null);
        Assert.Equal((nuint)0, text.size);
        Assert.Equal((nuint)0, bytes.size);
    }

    [Fact]
    public void StyleImagePixelsOutliveACompactingCollection()
    {
        var image = new PremultipliedRgba8Image(1, 1, 4, [1, 2, 3, 4]);
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativePremultipliedRgba8Image(image, scope);
        GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, blocking: true, compacting: true);
        Assert.Equal([1, 2, 3, 4], new ReadOnlySpan<byte>(native.pixels, 4).ToArray());
    }
}
