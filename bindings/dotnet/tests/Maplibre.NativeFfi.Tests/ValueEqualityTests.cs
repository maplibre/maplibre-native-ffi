using System.Collections.Immutable;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Records that hold a byte array or a list compare its contents, and share its elements
/// without a copy.
/// </summary>
public sealed class ValueEqualityTests
{
    private static readonly ImmutableArray<byte> Bytes = [1, 2, 3, 4, 5, 6, 7, 8];

    [Fact]
    public void AByteValuedRecordComparesAndSharesItsBytes()
    {
        var value = new PremultipliedRgba8Image(2, 1, 8, Bytes);

        Assert.Equal(new PremultipliedRgba8Image(2, 1, 8, [1, 2, 3, 4, 5, 6, 7, 8]), value);
        Assert.Equal(
            new PremultipliedRgba8Image(2, 1, 8, [1, 2, 3, 4, 5, 6, 7, 8]).GetHashCode(),
            value.GetHashCode()
        );
        Assert.NotEqual(new PremultipliedRgba8Image(2, 1, 8, [1, 2, 3, 4, 5, 6, 7, 9]), value);
        // The same underlying array, not a copy of it.
        Assert.True(value.Pixels == Bytes);
    }

    [Fact]
    public void AListValuedRecordComparesAndSharesItsElements()
    {
        ImmutableArray<string> urls = ["a", "b"];
        var value = new StyleSourceTileInfo { TileUrls = urls };

        Assert.Equal(new StyleSourceTileInfo { TileUrls = ["a", "b"] }, value);
        Assert.Equal(
            new StyleSourceTileInfo { TileUrls = ["a", "b"] }.GetHashCode(),
            value.GetHashCode()
        );
        Assert.NotEqual(new StyleSourceTileInfo { TileUrls = ["a"] }, value);
        Assert.True(value.TileUrls == urls);
    }
}
