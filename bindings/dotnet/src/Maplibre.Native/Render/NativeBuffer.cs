using System.Runtime.InteropServices;

namespace Maplibre.Native.Render;

/// <summary>Owned off-heap byte buffer for reusable native I/O.</summary>
public sealed unsafe class NativeBuffer : IDisposable
{
    private nint address;

    public NativeBuffer(int byteLength)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(byteLength);
        ByteLength = byteLength;
        address = (nint)NativeMemory.Alloc((nuint)(byteLength == 0 ? 1 : byteLength));
    }

    public int ByteLength { get; }
    public Span<byte> Span => new((void*)AddressOrThrow(), ByteLength);

    public void Dispose()
    {
        var allocation = (void*)System.Threading.Interlocked.Exchange(ref address, 0);
        if (allocation is null)
        {
            return;
        }

        NativeMemory.Free(allocation);
    }

    private nint AddressOrThrow()
    {
        if (address == 0)
        {
            throw new ObjectDisposedException(nameof(NativeBuffer));
        }

        return address;
    }
}
