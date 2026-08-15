using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

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
    public async Task ImageSourceApisAdaptCoordinatesAndImagesThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
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

        Assert.Equal(SourceType.Image, (await map.StyleSourceInfoAsync("image-url"))?.Type);
        Assert.Equal(SourceType.Image, (await map.StyleSourceInfoAsync("image-inline"))?.Type);
        Assert.Equal(updatedCoordinates, await map.GetImageSourceCoordinatesAsync("image-url"));
        Assert.Equal(coordinates, await map.GetImageSourceCoordinatesAsync("image-inline"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task StyleImageRoundTripsMetadataAndPixelsThroughNativeMap()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());
        var image = new PremultipliedRgba8Image([255, 0, 0, 255], new TextureImageInfo(1, 1, 4, 4));
        var options = new StyleImageOptions { PixelRatio = 2, Sdf = true };

        Assert.NotEqual(0ul, map.SetStyleImage("dot", image, options));

        var info = await map.StyleImageInfoAsync("dot");
        Assert.NotNull(info);
        Assert.Equal(1u, info.Width);
        Assert.Equal(1u, info.Height);
        Assert.Equal(4u, info.Stride);
        Assert.Equal(4u, info.ByteLength);
        Assert.Equal(2, info.PixelRatio);
        Assert.True(info.Sdf);

        var copied = await map.CopyStyleImagePremultipliedRgba8Async("dot");
        Assert.NotNull(copied);
        Assert.Equal([255, 0, 0, 255], copied.Image.Bytes);
        Assert.Equal(new TextureImageInfo(1, 1, 4, 4), copied.Image.Info);
        Assert.Equal(2, copied.Options.PixelRatio);
        Assert.True(copied.Options.Sdf);

        var removed = map.RemoveStyleImage("dot");
        var finished = RuntimeEventTestHelpers.WaitForCommand(runtime, removed);
        var completion = Assert.IsType<RuntimeEventPayload.CommandFinished>(finished.Payload);
        Assert.Equal(CommandDisposition.Committed, completion.Disposition);
        Assert.Null(await map.StyleImageInfoAsync("dot"));
        Assert.Null(await map.CopyStyleImagePremultipliedRgba8Async("dot"));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task NinePatchStyleImageRoundTripsStretchContentAndTextFit()
    {
        using var runtime = TestHandles.CreateRuntime(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });
        map.SetStyleJson("""{"version":8,"sources":{},"layers":[]}"""u8.ToArray());

        var image = new PremultipliedRgba8Image(new byte[16], new TextureImageInfo(2, 2, 8, 16));
        var options = new StyleImageOptions
        {
            StretchX = [new ImageStretch(0, 1)],
            StretchY = [new ImageStretch(0, 1), new ImageStretch(1, 2)],
            Content = new ImageContent(0.5f, 0.5f, 1.5f, 1.5f),
            TextFitHeight = StyleImageTextFit.Proportional,
        };
        Assert.NotEqual(0ul, map.SetStyleImage("patch", image, options));

        var info = await map.StyleImageInfoAsync("patch");
        Assert.NotNull(info);
        Assert.Equal(1UL, info.StretchXCount);
        Assert.Equal(2UL, info.StretchYCount);
        Assert.Equal(new ImageContent(0.5f, 0.5f, 1.5f, 1.5f), info.Content);
        // An absent text fit stays distinguishable from a present default.
        Assert.Null(info.TextFitWidth);
        Assert.Equal(StyleImageTextFit.Proportional, info.TextFitHeight);

        var stretches = await map.StyleImageStretchesAsync("patch");
        Assert.NotNull(stretches);
        Assert.Equal([new ImageStretch(0, 1)], stretches.Value.StretchX);
        Assert.Equal([new ImageStretch(0, 1), new ImageStretch(1, 2)], stretches.Value.StretchY);
        Assert.Null(await map.StyleImageStretchesAsync("missing"));

        var copied = await map.CopyStyleImagePremultipliedRgba8Async("patch");
        Assert.NotNull(copied);
        Assert.Equal([new ImageStretch(0, 1)], copied.Options.StretchX);
        Assert.Equal([new ImageStretch(0, 1), new ImageStretch(1, 2)], copied.Options.StretchY);
        Assert.Equal(new ImageContent(0.5f, 0.5f, 1.5f, 1.5f), copied.Options.Content);
        Assert.Null(copied.Options.TextFitWidth);
        Assert.Equal(StyleImageTextFit.Proportional, copied.Options.TextFitHeight);

        // A backwards interval is rejected by C.
        Assert.Throws<InvalidArgumentException>(() =>
            map.SetStyleImage(
                "bad",
                image,
                new StyleImageOptions { StretchX = [new ImageStretch(2, 1)] }
            )
        );
    }
}
