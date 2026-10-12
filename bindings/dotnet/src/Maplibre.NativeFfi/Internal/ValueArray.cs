namespace Maplibre.NativeFfi.Internal;

/// <summary>
/// The storage of an array member of a public value, compared element by
/// element so that record synthesis gives the value structural equality.
/// </summary>
/// <remarks>
/// A value copies an array on the way in and out, so a caller never shares
/// storage with it; generated converters read and adopt the storage directly.
/// A default instance holds the empty array. An optional member stores a
/// nullable instance, which keeps an absent array distinct from an empty one.
/// </remarks>
internal readonly struct ValueArray<T>(T[]? items) : IEquatable<ValueArray<T>>
{
    /// <summary>The adopted elements, which a converter must not mutate.</summary>
    internal T[] Items => items ?? [];

    internal T[] ToArray() => items is null ? [] : [.. items];

    public bool Equals(ValueArray<T> other) =>
        Items.AsSpan().SequenceEqual(other.Items, EqualityComparer<T>.Default);

    public override bool Equals(object? obj) => obj is ValueArray<T> other && Equals(other);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Items.Length);
        foreach (var item in Items)
            hash.Add(item);
        return hash.ToHashCode();
    }
}

internal static class ValueArray
{
    /// <summary>Copies a caller's array into storage that the caller cannot mutate.</summary>
    internal static ValueArray<T> Copy<T>(T[]? items) => new(items is null ? null : [.. items]);

    /// <summary>Copies a caller's optional array, keeping an absent array absent.</summary>
    internal static ValueArray<T>? CopyOptional<T>(T[]? items) =>
        items is null ? null : new ValueArray<T>([.. items]);

    /// <summary>Adopts an optional array that a converter just copied from native memory.</summary>
    internal static ValueArray<T>? Optional<T>(T[]? items) =>
        items is null ? null : new ValueArray<T>(items);
}
