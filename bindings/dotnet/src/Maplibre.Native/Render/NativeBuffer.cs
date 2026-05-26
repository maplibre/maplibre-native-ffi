using System.Runtime.InteropServices;

namespace Maplibre.Native.Render;

/// <summary>Owned off-heap byte buffer for reusable native I/O.</summary>
public sealed unsafe class NativeBuffer : IDisposable
{
    private nint address;

    public NativeBuffer(nuint byteLength)
    {
        ByteLength = byteLength;
        address = (nint)NativeMemory.Alloc(byteLength == 0 ? 1 : byteLength);
    }

    public nuint ByteLength { get; }
    public NativePointer Pointer => new(AddressOrThrow());
    public Span<byte> Span => new((void*)AddressOrThrow(), checked((int)ByteLength));

    public void Dispose()
    {
        var allocation = (void*)address;
        if (allocation is null)
        {
            return;
        }

        address = 0;
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
