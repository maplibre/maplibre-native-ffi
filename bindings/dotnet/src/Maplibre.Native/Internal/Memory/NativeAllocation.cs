using System.Runtime.InteropServices;

namespace Maplibre.Native.Internal.Memory;

internal static unsafe class NativeAllocation
{
    internal static T* AllocZeroedArray<T>(int count)
        where T : unmanaged
    {
        ArgumentOutOfRangeException.ThrowIfNegative(count);
        return count == 0 ? null : (T*)NativeMemory.AllocZeroed((nuint)count, (nuint)sizeof(T));
    }

    internal static T* AllocArray<T>(int count)
        where T : unmanaged
    {
        ArgumentOutOfRangeException.ThrowIfNegative(count);
        return count == 0 ? null : (T*)NativeMemory.Alloc((nuint)count, (nuint)sizeof(T));
    }
}
