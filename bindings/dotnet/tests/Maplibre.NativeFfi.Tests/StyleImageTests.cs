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
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
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

        _ = map.AddImageSourceUrlAsync(
            "image-url",
            coordinates,
            "https://example.test/image.png",
            TestContext.Current.CancellationToken
        );
        _ = map.SetImageSourceUrlAsync(
            "image-url",
            "https://example.test/other.png",
            TestContext.Current.CancellationToken
        );
        _ = map.SetImageSourceCoordinatesAsync(
            "image-url",
            updatedCoordinates,
            TestContext.Current.CancellationToken
        );
        _ = map.AddImageSourceImageAsync(
            "image-inline",
            coordinates,
            image,
            TestContext.Current.CancellationToken
        );
        _ = map.SetImageSourceImageAsync(
            "image-inline",
            image,
            TestContext.Current.CancellationToken
        );

        Assert.Equal(
            SourceType.Image,
            (
                await map.StyleSourceInfoAsync("image-url", TestContext.Current.CancellationToken)
            )?.Type
        );
        Assert.Equal(
            SourceType.Image,
            (
                await map.StyleSourceInfoAsync(
                    "image-inline",
                    TestContext.Current.CancellationToken
                )
            )?.Type
        );
        Assert.Equal(
            updatedCoordinates,
            await map.GetImageSourceCoordinatesAsync(
                "image-url",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Equal(
            coordinates,
            await map.GetImageSourceCoordinatesAsync(
                "image-inline",
                TestContext.Current.CancellationToken
            )
        );
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task StyleImageRoundTripsMetadataAndPixelsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(
            runtime,
            new MapOptions { Width = 512, Height = 512 }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var image = new PremultipliedRgba8Image([255, 0, 0, 255], new TextureImageInfo(1, 1, 4, 4));
        var options = new StyleImageOptions { PixelRatio = 2, Sdf = true };

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleImageAsync("dot", image, options, TestContext.Current.CancellationToken)
        );

        var copied = Assert.IsType<StyleImage>(
            await map.StyleImageAsync("dot", TestContext.Current.CancellationToken)
        );
        Assert.Equal([255, 0, 0, 255], copied.Image.Bytes);
        Assert.Equal(new TextureImageInfo(1, 1, 4, 4), copied.Image.Info);
        Assert.Equal(2, copied.Options.PixelRatio);
        Assert.True(copied.Options.Sdf);

        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleImageAsync("dot", TestContext.Current.CancellationToken)
        );
        Assert.Null(await map.StyleImageAsync("dot", TestContext.Current.CancellationToken));
    }

    [BindingSpecTest("BND-105")]
    [Fact]
    public async Task NinePatchStyleImageRoundTripsStretchContentAndTextFit()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = TestHandles.CreateMap(runtime, new MapOptions { Width = 64, Height = 64 });
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        var image = new PremultipliedRgba8Image(new byte[16], new TextureImageInfo(2, 2, 8, 16));
        var options = new StyleImageOptions
        {
            StretchX = [new ImageStretch(0, 1)],
            StretchY = [new ImageStretch(0, 1), new ImageStretch(1, 2)],
            Content = new ImageContent(0.5f, 0.5f, 1.5f, 1.5f),
            TextFitHeight = StyleImageTextFit.Proportional,
        };
        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleImageAsync("patch", image, options, TestContext.Current.CancellationToken)
        );

        var copied = Assert.IsType<StyleImage>(
            await map.StyleImageAsync("patch", TestContext.Current.CancellationToken)
        );
        Assert.Equal([new ImageStretch(0, 1)], copied.Options.StretchX);
        Assert.Equal([new ImageStretch(0, 1), new ImageStretch(1, 2)], copied.Options.StretchY);
        Assert.Equal(new ImageContent(0.5f, 0.5f, 1.5f, 1.5f), copied.Options.Content);
        Assert.Null(copied.Options.TextFitWidth);
        Assert.Equal(StyleImageTextFit.Proportional, copied.Options.TextFitHeight);
        Assert.Null(await map.StyleImageAsync("missing", TestContext.Current.CancellationToken));

        // The narrow copies read the same image one part at a time.
        Assert.Equal(
            copied.Image.Bytes,
            await map.GetStyleImagePremultipliedRgba8Async(
                "patch",
                TestContext.Current.CancellationToken
            )
        );
        var stretches = await map.GetStyleImageStretchesAsync(
            "patch",
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(stretches);
        Assert.Equal(copied.Options.StretchX, stretches!.Value.StretchX);
        Assert.Equal(copied.Options.StretchY, stretches.Value.StretchY);
        Assert.Null(
            await map.GetStyleImagePremultipliedRgba8Async(
                "missing",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Null(
            await map.GetStyleImageStretchesAsync("missing", TestContext.Current.CancellationToken)
        );

        // A backwards interval is rejected by C.
        await Assert.ThrowsAsync<InvalidArgumentException>(() =>
            map.SetStyleImageAsync(
                "bad",
                image,
                new StyleImageOptions { StretchX = [new ImageStretch(2, 1)] },
                TestContext.Current.CancellationToken
            )
        );
    }
}
