using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class GeneratedLayoutTests
{
    [Fact]
    public void StringViewMatchesPointerAndSizeLayout()
    {
        Assert.Equal(2 * IntPtr.Size, Unsafe.SizeOf<mln_string_view>());
        Assert.Equal(0, Marshal.OffsetOf<mln_string_view>(nameof(mln_string_view.data)).ToInt32());
        Assert.Equal(
            IntPtr.Size,
            Marshal.OffsetOf<mln_string_view>(nameof(mln_string_view.size)).ToInt32()
        );
    }

    [Fact]
    public void BooleanFieldsUseOneByteNativeBoolStorage()
    {
        var boolField = typeof(mln_json_value._data_e__Union).GetField(
            nameof(mln_json_value._data_e__Union.bool_value)
        );

        Assert.NotNull(boolField);
        Assert.Equal(typeof(byte), boolField.FieldType);
        Assert.Equal(
            0,
            Marshal
                .OffsetOf<mln_json_value._data_e__Union>(
                    nameof(mln_json_value._data_e__Union.bool_value)
                )
                .ToInt32()
        );
    }

    [Fact]
    public void OpaqueHandlesArePointerSizedAtCallBoundary()
    {
        unsafe
        {
            Assert.Equal(IntPtr.Size, sizeof(mln_runtime*));
            Assert.Equal(IntPtr.Size, sizeof(mln_map*));
            Assert.Equal(IntPtr.Size, sizeof(mln_render_session*));
        }
    }
}
