using Maplibre.Native.Map;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Maplibre.Native.Style;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class StyleImageTests
{
    [Fact]
    public void StyleImageRoundTripsMetadataAndPixelsThroughNativeMap()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();
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
