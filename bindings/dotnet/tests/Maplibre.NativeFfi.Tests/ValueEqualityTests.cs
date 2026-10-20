using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Records that hold a byte array or a list snapshot it, hand out copies, and compare contents.
/// </summary>
public sealed class ValueEqualityTests
{
    private static readonly byte[] Bytes = [1, 2, 3, 4, 5, 6, 7, 8];

    [Fact]
    public void AByteValuedRecordOwnsAndComparesItsBytes()
    {
        var source = Bytes.ToArray();
        var value = new PremultipliedRgba8Image(2, 1, 8, source);
        source[0] = 99;

        Assert.Equal(new PremultipliedRgba8Image(2, 1, 8, Bytes), value);
        Assert.Equal(
            new PremultipliedRgba8Image(2, 1, 8, Bytes).GetHashCode(),
            value.GetHashCode()
        );
        Assert.NotEqual(new PremultipliedRgba8Image(2, 1, 8, [1, 2, 3, 4, 5, 6, 7, 9]), value);
        value.Pixels[1] = 99;
        Assert.Equal(Bytes, value.Pixels);
    }

    [Fact]
    public void AListValuedRecordOwnsAndComparesItsElements()
    {
        var source = new[] { "a", "b" };
        var value = new StyleSourceTileInfo { TileUrls = source };
        source[0] = "z";

        Assert.Equal(new StyleSourceTileInfo { TileUrls = ["a", "b"] }, value);
        Assert.Equal(
            new StyleSourceTileInfo { TileUrls = ["a", "b"] }.GetHashCode(),
            value.GetHashCode()
        );
        Assert.NotEqual(new StyleSourceTileInfo { TileUrls = ["a"] }, value);
        value.TileUrls[1] = "z";
        Assert.Equal(["a", "b"], value.TileUrls);
    }
}
