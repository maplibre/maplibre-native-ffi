using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Log;

namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed unsafe class LogCallbackState
{
    private readonly LogCallback callback;

    private LogCallbackState(LogCallback callback)
    {
        this.callback = callback;
    }

    internal static void Set(LogCallback callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        NativeLibraryLoader.EnsureLoaded();
        var root = GCHandle.Alloc(new LogCallbackState(callback));
        try
        {
            NativeStatus.Check(
                NativeMethods.mln_log_set_callback(&OnLog, (void*)GCHandle.ToIntPtr(root), &Release)
            );
        }
        catch
        {
            root.Free();
            throw;
        }
    }

    internal static void Clear()
    {
        NativeLibraryLoader.EnsureLoaded();
        NativeStatus.Check(NativeMethods.mln_log_clear_callback());
    }

    internal static uint EmitForTest(
        LogCallback callback,
        uint severity,
        uint @event,
        long code,
        string message
    )
    {
        using var nativeMessage = NativeUtf8String.FromNullableString(message, nameof(message));
        return new LogCallbackState(callback).Invoke(severity, @event, code, nativeMessage.Pointer);
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static uint OnLog(void* userData, uint severity, uint @event, long code, sbyte* message)
    {
        try
        {
            if (userData is null)
            {
                return 0;
            }

            return ((LogCallbackState?)GCHandle.FromIntPtr((nint)userData).Target)?.Invoke(
                    severity,
                    @event,
                    code,
                    message
                ) ?? 0;
        }
        catch
        {
            return 0;
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
    private static void Release(void* userData)
    {
        if (userData is null)
        {
            return;
        }

        try
        {
            GCHandle.FromIntPtr((nint)userData).Free();
        }
        catch
        {
            // A release callback must not unwind across the C boundary.
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
}
