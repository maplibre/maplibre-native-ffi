namespace Maplibre.Native.Internal;

/// <summary>
/// Structural comparison helpers for public value types that hold list-valued members. Record
/// synthesis compares such members by reference, so types holding them supply their own
/// <c>Equals</c> and <c>GetHashCode</c> built on these helpers.
/// </summary>
internal static class ValueEquality
{
    internal static bool SequenceEquals<T>(IReadOnlyList<T>? left, IReadOnlyList<T>? right) =>
        SequenceEquals(left, right, static (a, b) => EqualityComparer<T>.Default.Equals(a, b));

    internal static bool SequenceEquals<T>(
        IReadOnlyList<T>? left,
        IReadOnlyList<T>? right,
        Func<T, T, bool> elementEquals
    )
    {
        if (ReferenceEquals(left, right))
        {
            return true;
        }
        if (left is null || right is null || left.Count != right.Count)
        {
            return false;
        }
        for (var index = 0; index < left.Count; index++)
        {
            if (!elementEquals(left[index], right[index]))
            {
                return false;
            }
        }
        return true;
    }

    internal static bool NestedSequenceEquals<T>(
        IReadOnlyList<IReadOnlyList<T>>? left,
        IReadOnlyList<IReadOnlyList<T>>? right
    ) => SequenceEquals(left, right, static (a, b) => SequenceEquals(a, b));

    internal static int SequenceHashCode<T>(IReadOnlyList<T>? values) =>
        SequenceHashCode(values, static value => value?.GetHashCode() ?? 0);

    internal static int SequenceHashCode<T>(IReadOnlyList<T>? values, Func<T, int> elementHash)
    {
        if (values is null)
        {
            return 0;
        }
        var hash = new HashCode();
        hash.Add(values.Count);
        for (var index = 0; index < values.Count; index++)
        {
            hash.Add(elementHash(values[index]));
        }
        return hash.ToHashCode();
    }

    internal static int NestedSequenceHashCode<T>(IReadOnlyList<IReadOnlyList<T>>? values) =>
        SequenceHashCode(values, static value => SequenceHashCode(value));
}
