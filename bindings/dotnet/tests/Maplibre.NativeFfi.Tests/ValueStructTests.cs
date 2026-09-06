using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Render;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class ValueStructTests
{
    [Fact]
    public void StyleImageBorrowsBackingPixelsAcrossCompactingCollection()
    {
        var image = new PremultipliedRgba8Image([1, 2, 3, 4], new TextureImageInfo(1, 1, 4, 4));
        using var native = NativeStyleImage.From(image);

        GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, blocking: true, compacting: true);

        fixed (byte* expected = image.BytesTransit)
        {
            Assert.Equal((nint)expected, (nint)native.Value.pixels);
            Assert.Equal([1, 2, 3, 4], new ReadOnlySpan<byte>(native.Value.pixels, 4).ToArray());
        }
    }

    [BindingSpecTest("BND-066")]
    [Fact]
    public void BufferIsDestroyedWhenCopyingBytesFails()
    {
        var destroyCalls = 0;
        using var methods = ValueStructs.UseBufferMethodsForTest(
            (_, outView) =>
            {
                *outView = new mln_buffer_view { data = (void*)1, size = (nuint)int.MaxValue + 1 };
                return mln_status.MLN_STATUS_OK;
            },
            _ => destroyCalls++
        );

        Assert.Throws<OverflowException>(() => ValueStructs.ReadBuffer(new MlnBuffer(1234)));

        Assert.Equal(1, destroyCalls);
    }
}
