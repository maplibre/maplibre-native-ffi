using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Log;

namespace Maplibre.Native.Internal.Callback;

internal sealed unsafe class LogCallbackState : IDisposable
{
    private static readonly Lock Gate = new();
    private static readonly Lock RegistryGate = new();
    private static readonly Dictionary<nint, LogCallbackState> Registry = [];
    private static LogCallbackState? current;
    private static nint nextToken;

    private readonly nint token;
    private readonly LogCallback callback;
    private bool retired;

    private LogCallbackState(LogCallback callback)
    {
        this.callback = callback;
        lock (RegistryGate)
        {
            token = ++nextToken;
            if (token == 0)
            {
                token = ++nextToken;
            }

            Registry.Add(token, this);
        }
    }

    internal static void Set(LogCallback callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        NativeLibraryLoader.EnsureLoaded();
        var replacement = new LogCallbackState(callback);
        lock (Gate)
        {
            try
            {
                NativeStatus.Check(
                    NativeMethods.mln_log_set_callback(&OnLog, replacement.UserData)
                );
                var old = current;
                current = replacement;
                old?.Retire();
            }
            catch
            {
                replacement.Retire();
                throw;
            }
        }
    }

    internal static void Clear()
    {
        NativeLibraryLoader.EnsureLoaded();
        lock (Gate)
        {
            NativeStatus.Check(NativeMethods.mln_log_clear_callback());
            var old = current;
            current = null;
            old?.Retire();
        }
    }

    private void* UserData => (void*)token;

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static uint OnLog(void* userData, uint severity, uint @event, long code, sbyte* message)
    {
        try
        {
            var state = Enter((nint)userData);
            if (state is null)
            {
                return 0;
            }

            return state.Invoke(severity, @event, code, message);
        }
        catch
        {
            return 0;
        }
    }

    private uint Invoke(uint severity, uint @event, long code, sbyte* message)
    {
        try
        {
            var text = message is null
                ? string.Empty
                : Marshal.PtrToStringUTF8((nint)message) ?? string.Empty;
            return callback(
                new LogRecord((LogSeverity)severity, severity, (LogEvent)@event, @event, code, text)
            )
                ? 1u
                : 0u;
        }
        catch
        {
            return 0;
        }
    }

    private static LogCallbackState? Enter(nint token)
    {
        lock (RegistryGate)
        {
            if (!Registry.TryGetValue(token, out var state) || state.retired)
            {
                return null;
            }

            return state;
        }
    }

    public void Dispose()
    {
        Retire();
    }

    private void Retire()
    {
        lock (RegistryGate)
        {
            if (retired)
            {
                return;
            }

            retired = true;
            Registry.Remove(token);
        }
    }
}
