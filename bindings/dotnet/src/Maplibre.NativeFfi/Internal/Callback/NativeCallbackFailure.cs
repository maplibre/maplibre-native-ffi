namespace Maplibre.NativeFfi.Internal.Callback;

internal static class NativeCallbackFailure
{
    /// <summary>
    /// Reports an exception that the trampoline of <paramref name="callback" /> caught to each
    /// <see cref="Maplibre.CallbackException" /> handler, or to standard error when there is none.
    /// Never throws, since it runs inside a native callback.
    /// </summary>
    internal static void Report(string callback, Exception exception)
    {
        var handlers = Maplibre.CallbackExceptionHandlers;
        if (handlers is null)
        {
            try
            {
                Console.Error.WriteLine(
                    $"Maplibre.NativeFfi CallbackException: {callback} threw and native received its fallback: {exception}"
                );
            }
            catch
            {
                // Reporting must not throw across the native boundary.
            }
            return;
        }
        var args = new CallbackExceptionEventArgs(callback, exception);
        foreach (var handler in handlers.GetInvocationList())
        {
            try
            {
                ((EventHandler<CallbackExceptionEventArgs>)handler)(null, args);
            }
            catch
            {
                // One handler's failure must neither reach native nor skip the others.
            }
        }
    }
}
