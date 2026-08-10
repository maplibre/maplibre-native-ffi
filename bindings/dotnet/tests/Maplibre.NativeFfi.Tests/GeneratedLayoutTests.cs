using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class GeneratedLayoutTests
{
    [Fact]
    public void StringViewMatchesPointerAndSizeLayout()
    {
        Assert.Equal(2 * IntPtr.Size, Unsafe.SizeOf<mln_buffer_view>());
        Assert.Equal(0, Marshal.OffsetOf<mln_buffer_view>(nameof(mln_buffer_view.data)).ToInt32());
        Assert.Equal(
            IntPtr.Size,
            Marshal.OffsetOf<mln_buffer_view>(nameof(mln_buffer_view.size)).ToInt32()
        );
    }
}
