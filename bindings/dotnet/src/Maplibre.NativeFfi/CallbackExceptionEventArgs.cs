namespace Maplibre.NativeFfi;

/// <summary>An exception that a native callback threw, which native could not receive.</summary>
public sealed class CallbackExceptionEventArgs : EventArgs
{
    internal CallbackExceptionEventArgs(string callback, Exception exception)
    {
        Callback = callback;
        Exception = exception;
    }

    /// <summary>The C callback type that threw, such as <c>mln_resource_provider_callback</c>.</summary>
    public string Callback { get; }

    /// <summary>The exception that the callback threw.</summary>
    public Exception Exception { get; }
}
