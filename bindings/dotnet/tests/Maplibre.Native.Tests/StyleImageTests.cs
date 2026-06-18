using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class StyleImageTests
{
    [BindingSpecTest("BND-069")]
    [Fact]
    public void PremultipliedRgba8ImageSnapshotsPixelsAndReturnsCopies()
    {
        var source = new byte[] { 1, 2, 3, 4 };
        var image = new PremultipliedRgba8Image(source, new TextureImageInfo(1, 1, 4, 4));
        source[0] = 9;

        var first = image.Bytes;
        Assert.Equal([1, 2, 3, 4], first);
        first[0] = 8;
        Assert.Equal([1, 2, 3, 4], image.Bytes);
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void ImageSourceApisAdaptCoordinatesAndImagesThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        var coordinates = new[]
        {
            new LatLng(10, 10),
            new LatLng(10, 20),
            new LatLng(0, 20),
            new LatLng(0, 10),
        };
        var updatedCoordinates = new[]
        {
            new LatLng(20, 20),
            new LatLng(20, 30),
            new LatLng(10, 30),
            new LatLng(10, 20),
        };
        var image = new PremultipliedRgba8Image([0, 255, 0, 255], new TextureImageInfo(1, 1, 4, 4));

        map.AddImageSourceUrl("image-url", coordinates, "https://example.test/image.png");
        map.SetImageSourceUrl("image-url", "https://example.test/other.png");
        map.SetImageSourceCoordinates("image-url", updatedCoordinates);
        map.AddImageSourceImage("image-inline", coordinates, image);
        map.SetImageSourceImage("image-inline", image);

        Assert.Equal(SourceType.Image, map.StyleSourceType("image-url"));
        Assert.Equal(SourceType.Image, map.StyleSourceType("image-inline"));
        Assert.Equal(updatedCoordinates, map.GetImageSourceCoordinates("image-url"));
        Assert.Equal(coordinates, map.GetImageSourceCoordinates("image-inline"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public void StyleImageRoundTripsMetadataAndPixelsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        map.SetStyleJson("{\"version\":8,\"sources\":{},\"layers\":[]}");
        var image = new PremultipliedRgba8Image([255, 0, 0, 255], new TextureImageInfo(1, 1, 4, 4));
        var options = new StyleImageOptions { PixelRatio = 2, Sdf = true };

        map.SetStyleImage("dot", image, options);

        Assert.True(map.StyleImageExists("dot"));
        var info = map.StyleImageInfo("dot");
        Assert.NotNull(info);
        Assert.Equal(1u, info.Width);
        Assert.Equal(1u, info.Height);
        Assert.Equal(4u, info.Stride);
        Assert.Equal(4u, info.ByteLength);
        Assert.Equal(2, info.PixelRatio);
        Assert.True(info.Sdf);

        var copied = map.CopyStyleImagePremultipliedRgba8("dot");
        Assert.NotNull(copied);
        Assert.Equal([255, 0, 0, 255], copied.Image.Bytes);
        Assert.Equal(new TextureImageInfo(1, 1, 4, 4), copied.Image.Info);
        Assert.Equal(2, copied.Options.PixelRatio);
        Assert.True(copied.Options.Sdf);

        Assert.True(map.RemoveStyleImage("dot"));
        Assert.False(map.StyleImageExists("dot"));
        Assert.Null(map.StyleImageInfo("dot"));
        Assert.Null(map.CopyStyleImagePremultipliedRgba8("dot"));
    }
}
