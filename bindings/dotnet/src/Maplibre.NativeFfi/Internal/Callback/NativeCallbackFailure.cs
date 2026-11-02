namespace Maplibre.NativeFfi.Internal.Callback;

internal static class NativeCallbackFailure
{
    internal static void Report(string callback, Exception exception)
    {
        var handlers = MaplibreNative.CallbackExceptionHandlers;
        if (handlers is null)
        {
            try
            {
                Console.Error.WriteLine(
                    $"Maplibre.NativeFfi CallbackException: {callback} threw: {exception}"
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
