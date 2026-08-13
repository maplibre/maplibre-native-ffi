using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Public values wrapping copied byte buffers compare buffer contents.
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

    [BindingSpecTest("BND-069", "BND-070")]
    [Fact]
    public void OfflineGeometryRegionOwnsAndComparesGeometryContents()
    {
        var geometry = new byte[] { 1, 2, 3 };
        var left = new OfflineRegionDefinition.GeometryRegion(
            "https://example.invalid/style.json",
            geometry,
            0,
            10,
            1,
            false
        );
        var right = new OfflineRegionDefinition.GeometryRegion(
            "https://example.invalid/style.json",
            [1, 2, 3],
            0,
            10,
            1,
            false
        );

        geometry[0] = 9;
        var returned = left.Geometry;
        returned[1] = 9;

        Assert.Equal(left, right);
        Assert.Equal(left.GetHashCode(), right.GetHashCode());
        Assert.Equal(new byte[] { 1, 2, 3 }, left.Geometry);
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
