using System.Text;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.Memory;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class NativeUtf8StringTests
{
    [BindingSpecTest("BND-024")]
    [Fact]
    public void RejectsEmbeddedNul()
    {
        var error = Assert.Throws<InvalidArgumentException>(() =>
            NativeUtf8String.FromNullableString("a\0b", "value")
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Contains("embedded NUL", error.Diagnostic, StringComparison.Ordinal);
    }

    // Support invariant for null-terminated C string inputs: non-null strings are
    // encoded as UTF-8 with exactly one terminator before crossing into C.
    [Fact]
    public void EncodesUtf8WithTerminatingNul()
    {
        using var value = NativeUtf8String.FromNullableString("hé", "value");
        var expected = Encoding.UTF8.GetBytes("hé");

        for (var index = 0; index < expected.Length; index++)
        {
            Assert.Equal(expected[index], ((byte*)value.Pointer)[index]);
        }

        Assert.Equal(0, ((byte*)value.Pointer)[expected.Length]);
    }

    // Support invariant for nullable C string inputs: null remains an absent
    // pointer rather than an empty string.
    [Fact]
    public void NullStringProducesNullPointer()
    {
        using var value = NativeUtf8String.FromNullableString(null, "value");

        Assert.Equal((nint)0, (nint)value.Pointer);
    }
}
