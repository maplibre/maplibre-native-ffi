using System.Runtime.InteropServices;
using System.Text;
using Maplibre.Native.Error;

namespace Maplibre.Native.Internal.Memory;

internal sealed unsafe class NativeUtf8String : IDisposable
{
    private nint pointer;
    private readonly nuint byteLength;

    private NativeUtf8String(nint pointer, nuint byteLength)
    {
        this.pointer = pointer;
        this.byteLength = byteLength;
    }

    internal static NativeUtf8String Null { get; } = new(0, 0);

    internal sbyte* Pointer => (sbyte*)pointer;

    internal nuint ByteLength => byteLength;

    internal static NativeUtf8String FromNullableString(string? value, string parameterName)
    {
        if (value is null)
        {
            return Null;
        }

        if (value.Contains('\0', StringComparison.Ordinal))
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"{parameterName} contains an embedded NUL character."
            );
        }

        var byteCount = Encoding.UTF8.GetByteCount(value);
        var allocation = (byte*)NativeMemory.Alloc((nuint)byteCount + 1);
        try
        {
            Encoding.UTF8.GetBytes(value, new Span<byte>(allocation, byteCount));
            allocation[byteCount] = 0;
            return new NativeUtf8String((nint)allocation, (nuint)byteCount);
        }
        catch
        {
            NativeMemory.Free(allocation);
            throw;
        }
    }

    public void Dispose()
    {
        var allocation = (void*)pointer;
        if (allocation is null)
        {
            return;
        }

        pointer = 0;
        NativeMemory.Free(allocation);
    }
}
