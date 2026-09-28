namespace Maplibre.NativeFfi.Internal.Callback;

internal static class NativeCallbackGuard
{
    [ThreadStatic]
    private static int forbidden;

    [ThreadStatic]
    private static object? permittedOwner;

    [ThreadStatic]
    private static string[]? permittedOperations;

    internal static Restriction Restrict(object owner, string[] operations)
    {
        var previous = new Restriction(permittedOwner, permittedOperations);
        permittedOwner = owner;
        permittedOperations = operations;
        return previous;
    }

    internal static Scope ForbidReentry()
    {
        forbidden++;
        return new Scope();
    }

    internal static void EnsureAllowed(object? owner = null, string? operation = null)
    {
        if (
            forbidden != 0
            || (
                permittedOperations is { } operations
                && (!ReferenceEquals(owner, permittedOwner) || !operations.Contains(operation))
            )
        )
            throw new InvalidOperationException(
                "Native API calls are forbidden inside this callback."
            );
    }

    internal readonly ref struct Restriction(object? owner, string[]? operations)
    {
        public void Dispose()
        {
            permittedOwner = owner;
            permittedOperations = operations;
        }
    }

    internal readonly ref struct Scope
    {
        public void Dispose() => forbidden--;
    }
}
