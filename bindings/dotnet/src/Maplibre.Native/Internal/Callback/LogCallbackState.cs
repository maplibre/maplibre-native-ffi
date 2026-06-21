using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Log;

namespace Maplibre.Native.Internal.Callback;

internal unsafe delegate mln_status LogSetCallback(
    delegate* unmanaged[Cdecl]<void*, uint, uint, long, sbyte*, uint> callback,
    void* userData
);

internal delegate mln_status LogClearCallback();

internal sealed unsafe class LogCallbackState : IDisposable
{
    private static readonly LogSetCallback DefaultSetCallback = static (callback, userData) =>
        NativeMethods.mln_log_set_callback(callback, userData);
    private static readonly LogClearCallback DefaultClearCallback = static () =>
        NativeMethods.mln_log_clear_callback();
    private static readonly Lock Gate = new();
    private static readonly Lock RegistryGate = new();
    private static readonly Dictionary<nint, LogCallbackState> Registry = [];
    private static LogCallbackState? current;
    private static nint nextToken;

    [ThreadStatic]
    private static LogSetCallback? setCallbackForTest;

    [ThreadStatic]
    private static LogClearCallback? clearCallbackForTest;

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
                NativeStatus.Check(SetCallback(&OnLog, replacement.UserData));
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
            NativeStatus.Check(ClearCallback());
            var old = current;
            current = null;
            old?.Retire();
        }
    }

    internal static LogCallbackState? CurrentForTest => current;

    internal bool IsRetiredForTest => retired;

    internal static uint EmitForTest(uint severity, uint @event, long code, string message)
    {
        using var nativeMessage = NativeUtf8String.FromNullableString(message, nameof(message));
        var state = current;
        return state is null ? 0 : state.Invoke(severity, @event, code, nativeMessage.Pointer);
    }

    internal static LogCallbackState? StateForTokenForTest(nint token)
    {
        lock (RegistryGate)
        {
            return Registry.GetValueOrDefault(token);
        }
    }

    internal static IDisposable UseCallbackMethodsForTest(
        LogSetCallback setCallback,
        LogClearCallback clearCallback
    )
    {
        var previousSet = setCallbackForTest;
        var previousClear = clearCallbackForTest;
        setCallbackForTest = setCallback;
        clearCallbackForTest = clearCallback;
        return new RestoreCallbackMethods(previousSet, previousClear);
    }

    private static LogSetCallback SetCallback => setCallbackForTest ?? DefaultSetCallback;

    private static LogClearCallback ClearCallback => clearCallbackForTest ?? DefaultClearCallback;

    private sealed class RestoreCallbackMethods(
        LogSetCallback? previousSet,
        LogClearCallback? previousClear
    ) : IDisposable
    {
        public void Dispose()
        {
            setCallbackForTest = previousSet;
            clearCallbackForTest = previousClear;
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
