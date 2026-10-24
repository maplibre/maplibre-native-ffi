using System.Runtime.InteropServices;

namespace Maplibre.NativeFfi.Internal;

/// <summary>
/// The storage of an array member of a public value, compared element by
/// element so that record synthesis gives the value structural equality.
/// </summary>
/// <remarks>
/// A value takes and gives its elements as an ImmutableArray over this storage,
/// which nothing mutates, so neither direction copies. Generated converters
/// read and adopt the storage directly. A default instance holds the empty
/// array. An optional member stores a nullable instance, which keeps an absent
/// array distinct from an empty one.
/// </remarks>
internal readonly struct ValueArray<T>(T[]? items) : IEquatable<ValueArray<T>>
{
    /// <summary>The adopted elements, which a converter must not mutate.</summary>
    internal T[] Items => items ?? [];

    internal ImmutableArray<T> AsImmutable() => ImmutableCollectionsMarshal.AsImmutableArray(Items);

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
    /// <summary>Adopts a caller's elements, which an ImmutableArray never changes.</summary>
    internal static ValueArray<T> From<T>(ImmutableArray<T> items) =>
        new(ImmutableCollectionsMarshal.AsArray(items));

    /// <summary>Adopts a caller's optional elements, keeping an absent array absent.</summary>
    internal static ValueArray<T>? FromOptional<T>(ImmutableArray<T>? items) =>
        items is { } value ? From(value) : null;

    /// <summary>Adopts an optional array that a converter just copied from native memory.</summary>
    internal static ValueArray<T>? Optional<T>(T[]? items) =>
        items is null ? null : new ValueArray<T>(items);
}
