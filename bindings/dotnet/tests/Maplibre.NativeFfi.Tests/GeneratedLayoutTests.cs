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

    // The opaque window this binding copies for an undeclared payload kind runs from the payload
    // union to the end of the event record, so a field the header adds after the union would make
    // that window read unrelated bytes.
    [Fact]
    public void ThePayloadUnionEndsTheEventRecord()
    {
        Assert.Equal(
            Unsafe.SizeOf<mln_runtime_event>(),
            Marshal.OffsetOf<mln_runtime_event>(nameof(mln_runtime_event.payload)).ToInt32()
                + Unsafe.SizeOf<mln_runtime_event_payload>()
        );
    }
}
