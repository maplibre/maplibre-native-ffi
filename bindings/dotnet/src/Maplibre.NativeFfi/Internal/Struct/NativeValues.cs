using Maplibre.NativeFfi.Error;

namespace Maplibre.NativeFfi.Internal.Struct;

/// <summary>The steps that generated value encoders repeat for each member.</summary>
internal static class NativeValues
{
    /// <summary>
    /// Stores a present optional member in its native slot and returns the
    /// presence bit to set, or no bits when the member is absent.
    /// </summary>
    internal static TFlags Put<T, TFlags>(T? value, ref T slot, TFlags bit)
        where T : struct
        where TFlags : struct, Enum
    {
        if (value is not { } present)
            return default;
        slot = present;
        return bit;
    }

    /// <inheritdoc cref="Put{T, TFlags}(T?, ref T, TFlags)"/>
    internal static TFlags Put<TPublic, TNative, TFlags>(
        TPublic? value,
        ref TNative slot,
        TFlags bit,
        Func<TPublic, TNative> encode
    )
        where TPublic : struct
        where TFlags : struct, Enum
    {
        if (value is not { } present)
            return default;
        slot = encode(present);
        return bit;
    }

    /// <inheritdoc cref="Put{T, TFlags}(T?, ref T, TFlags)"/>
    internal static TFlags Put<TPublic, TNative, TFlags>(
        TPublic? value,
        ref TNative slot,
        TFlags bit,
        Func<TPublic, TNative> encode
    )
        where TPublic : class
        where TFlags : struct, Enum
    {
        if (value is null)
            return default;
        slot = encode(value);
        return bit;
    }

    /// <summary>
    /// Rejects a null in a member whose type does not admit one, which a record
    /// struct's default value or a partial initializer can leave behind.
    /// </summary>
    internal static void Required(object? value, string message)
    {
        if (value is null)
            throw new InvalidArgumentException(MaplibreStatus.InvalidArgument, null, message, null);
    }
}
