using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Loader;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Log;

namespace Maplibre.Native.Internal.Callback;

internal sealed unsafe class LogCallbackState : IDisposable
{
  private static readonly Lock Gate = new();
  private static LogCallbackState? current;

  private readonly LogCallback callback;
  private nint handle;

  private LogCallbackState(LogCallback callback)
  {
    this.callback = callback;
    handle = GCHandle.ToIntPtr(GCHandle.Alloc(this));
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
        NativeStatus.Check(NativeMethods.mln_log_set_callback(&OnLog, (void*)replacement.handle));
        var old = current;
        current = replacement;
        old?.Dispose();
      }
      catch
      {
        replacement.Dispose();
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
      old?.Dispose();
    }
  }

  [UnmanagedCallersOnly(CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)])]
  private static uint OnLog(void* userData, uint severity, uint @event, long code, sbyte* message)
  {
    try
    {
      var state = (LogCallbackState?)GCHandle.FromIntPtr((nint)userData).Target;
      if (state is null)
      {
        return 0;
      }

      var text = message is null ? string.Empty : Marshal.PtrToStringUTF8((nint)message) ?? string.Empty;
      return state.callback(new LogRecord(
          (LogSeverity)severity,
          severity,
          (LogEvent)@event,
          @event,
          code,
          text)) ? 1u : 0u;
    }
    catch
    {
      return 0;
    }
  }

  public void Dispose()
  {
    var current = System.Threading.Interlocked.Exchange(ref handle, 0);
    if (current != 0)
    {
      GCHandle.FromIntPtr(current).Free();
    }
  }
}
