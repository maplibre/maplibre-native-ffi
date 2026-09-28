using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class StyleImageTests
{
    [BindingSpecTest("")]
    [Fact]
    public void PremultipliedRgba8ImageSnapshotsPixelsAndReturnsCopies()
    {
        var source = new byte[] { 1, 2, 3, 4 };
        var image = new PremultipliedRgba8Image(1, 1, 4, source);
        source[0] = 9;

        var first = image.Pixels;
        Assert.Equal([1, 2, 3, 4], first);
        first[0] = 8;
        Assert.Equal([1, 2, 3, 4], image.Pixels);
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task ImageSourceApisAdaptCoordinatesAndImagesThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        Assert.Null(
            await map.GetImageSourceCoordinatesAsync(
                "missing-image-source",
                TestContext.Current.CancellationToken
            )
        );
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
        var image = new PremultipliedRgba8Image(1, 1, 4, [0, 255, 0, 255]);

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
            StyleSourceType.Image,
            (await map.GetStyleSourceInfoAsync("image-url", TestContext.Current.CancellationToken))
                ?.Info
                .Type
        );
        Assert.Equal(
            StyleSourceType.Image,
            (
                await map.GetStyleSourceInfoAsync(
                    "image-inline",
                    TestContext.Current.CancellationToken
                )
            )
                ?.Info
                .Type
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

    [BindingSpecTest("")]
    [Fact]
    public async Task StyleImageRoundTripsMetadataAndPixelsThroughNativeMap()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);
        var image = new PremultipliedRgba8Image(1, 1, 4, [255, 0, 0, 255]);
        var options = new StyleImageOptions { PixelRatio = 2, Sdf = true };

        RuntimeEventTestHelpers.AssertCommitted(
            map.SetStyleImageAsync("dot", image, options, TestContext.Current.CancellationToken)
        );

        var copied = Assert.IsType<StyleImageResult>(
            await map.GetStyleImageInfoAsync("dot", TestContext.Current.CancellationToken)
        );
        Assert.Equal([255, 0, 0, 255], copied.Pixels);
        Assert.Equal(
            (1u, 1u, 4u, 4ul),
            (copied.Info.Width, copied.Info.Height, copied.Info.Stride, copied.Info.ByteLength)
        );
        Assert.Equal(2, copied.Info.PixelRatio);
        Assert.True(copied.Info.Sdf);

        RuntimeEventTestHelpers.AssertCommitted(
            map.RemoveStyleImageAsync("dot", TestContext.Current.CancellationToken)
        );
        Assert.Null(await map.GetStyleImageInfoAsync("dot", TestContext.Current.CancellationToken));
    }

    [BindingSpecTest("")]
    [Fact]
    public async Task NinePatchStyleImageRoundTripsStretchContentAndTextFit()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 64, Height = 64 },
            }
        );
        _ = map.SetStyleJsonAsync(TestStyles.Empty, TestContext.Current.CancellationToken);

        var image = new PremultipliedRgba8Image(2, 2, 8, new byte[16]);
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

        var copied = Assert.IsType<StyleImageResult>(
            await map.GetStyleImageInfoAsync("patch", TestContext.Current.CancellationToken)
        );
        Assert.Equal([new ImageStretch(0, 1)], copied.StretchX);
        Assert.Equal([new ImageStretch(0, 1), new ImageStretch(1, 2)], copied.StretchY);
        Assert.Equal(new ImageContent(0.5f, 0.5f, 1.5f, 1.5f), copied.Info.Content);
        Assert.Null(copied.Info.TextFitWidth);
        Assert.Equal(StyleImageTextFit.Proportional, copied.Info.TextFitHeight);
        Assert.Null(
            await map.GetStyleImageInfoAsync("missing", TestContext.Current.CancellationToken)
        );

        // The narrow copies read the same image one part at a time.
        Assert.Equal(
            copied.Pixels,
            await map.CopyStyleImagePremultipliedRgba8Async(
                "patch",
                TestContext.Current.CancellationToken
            )
        );
        var stretches = await map.CopyStyleImageStretchesAsync(
            "patch",
            TestContext.Current.CancellationToken
        );
        Assert.NotNull(stretches);
        Assert.Equal(copied.StretchX, stretches!.Value.StretchX);
        Assert.Equal(copied.StretchY, stretches.Value.StretchY);
        Assert.Null(
            await map.CopyStyleImagePremultipliedRgba8Async(
                "missing",
                TestContext.Current.CancellationToken
            )
        );
        Assert.Null(
            await map.CopyStyleImageStretchesAsync("missing", TestContext.Current.CancellationToken)
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
