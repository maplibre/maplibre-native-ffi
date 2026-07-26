using Maplibre.Native.Geo;
using Maplibre.Native.Offline;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

/// <summary>
/// BND-070: public values wrapping copied byte buffers compare buffer contents, matching the
/// Kotlin binding's <c>contentEquals</c> behavior for the same types.
/// </summary>
public sealed class BufferValueEqualityTests
{
    private static readonly TextureImageInfo Info = new(2, 1, 8, 8);

    [BindingSpecTest("BND-070")]
    [Fact]
    public void ImagesComparePixelContents()
    {
        var left = new PremultipliedRgba8Image([1, 2, 3, 4, 5, 6, 7, 8], Info);
        var right = new PremultipliedRgba8Image([1, 2, 3, 4, 5, 6, 7, 8], Info);

        Assert.Equal(left, right);
        Assert.Equal(left.GetHashCode(), right.GetHashCode());
        Assert.NotEqual(left, new PremultipliedRgba8Image([1, 2, 3, 4, 5, 6, 7, 9], Info));
        Assert.NotEqual(left, new PremultipliedRgba8Image([1, 2, 3, 4], new(1, 1, 4, 4)));
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void OfflineRegionInfoComparesMetadataContents()
    {
        var definition = new OfflineRegionDefinition.TilePyramid(
            "https://example.invalid/style.json",
            new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
            0,
            10,
            1,
            false
        );

        var left = new OfflineRegionInfo(7, definition, [1, 2, 3]);
        var right = new OfflineRegionInfo(7, definition, [1, 2, 3]);

        Assert.Equal(left, right);
        Assert.Equal(left.GetHashCode(), right.GetHashCode());
        Assert.NotEqual(left, new OfflineRegionInfo(7, definition, [1, 2, 4]));
        Assert.NotEqual(left, new OfflineRegionInfo(8, definition, [1, 2, 3]));
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void UnknownEventPayloadComparesPayloadContents()
    {
        var left = new RuntimeEventPayload.Unknown(3, [9, 8, 7]);
        var right = new RuntimeEventPayload.Unknown(3, [9, 8, 7]);

        Assert.Equal(left, right);
        Assert.Equal(left.GetHashCode(), right.GetHashCode());
        Assert.NotEqual(left, new RuntimeEventPayload.Unknown(3, [9, 8, 6]));
        Assert.NotEqual(left, new RuntimeEventPayload.Unknown(4, [9, 8, 7]));
    }

    [BindingSpecTest("BND-070")]
    [Fact]
    public void MutatingTheCallerBufferDoesNotChangeEquality()
    {
        // BND-069: the copied buffer is what participates in equality.
        var pixels = new byte[] { 1, 2, 3, 4, 5, 6, 7, 8 };
        var image = new PremultipliedRgba8Image(pixels, Info);

        pixels[0] = 99;

        Assert.Equal(new PremultipliedRgba8Image([1, 2, 3, 4, 5, 6, 7, 8], Info), image);
    }
}
