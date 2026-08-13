namespace Maplibre.NativeFfi.Runtime;

internal static class OperationAwaiter
{
    internal static async Task<TResult> WaitThen<TResult>(
        Task readiness,
        Func<TResult> take,
        Action release
    )
    {
        try
        {
            await readiness.ConfigureAwait(false);
            return take();
        }
        finally
        {
            release();
        }
    }

    internal static async Task<TResult> WaitThen<TResult>(
        Task readiness,
        Func<TResult> take,
        Action release,
        Action failure
    )
    {
        try
        {
            await readiness.ConfigureAwait(false);
            return take();
        }
        catch
        {
            failure();
            throw;
        }
        finally
        {
            release();
        }
    }

    internal static async Task WaitThen(Task readiness, Action complete, Action release)
    {
        try
        {
            await readiness.ConfigureAwait(false);
            complete();
        }
        finally
        {
            release();
        }
    }
}
