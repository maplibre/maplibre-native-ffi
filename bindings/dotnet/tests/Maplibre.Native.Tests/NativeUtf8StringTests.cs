using System.Text;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.Memory;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class NativeUtf8StringTests
{
    [Fact]
    public void RejectsEmbeddedNul()
    {
        var error = Assert.Throws<InvalidArgumentException>(() =>
            NativeUtf8String.FromNullableString("a\0b", "value")
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Contains("embedded NUL", error.Diagnostic, StringComparison.Ordinal);
    }

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

    [Fact]
    public void NullStringProducesNullPointer()
    {
        using var value = NativeUtf8String.FromNullableString(null, "value");

        Assert.Equal((nint)0, (nint)value.Pointer);
    }
}
