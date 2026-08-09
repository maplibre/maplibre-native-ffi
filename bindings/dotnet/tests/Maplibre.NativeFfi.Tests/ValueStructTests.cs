using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class ValueStructTests
{
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
