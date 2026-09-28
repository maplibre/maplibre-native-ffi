using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Silk.NET.GLFW;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

internal sealed unsafe partial class WglOwnedTextureFixture : OwnedTextureFixture
{
    private static readonly object GlfwLock = new();
    private readonly ManualResetEventSlim stop = new();
    private readonly TaskCompletionSource ready = new(
        TaskCreationOptions.RunContinuationsAsynchronously
    );
    private readonly Thread thread;
    private nint deviceContext;
    private int disposed;

    internal static bool IsSupported =>
        OperatingSystem.IsWindows()
        && Maplibre.SupportedRenderBackendMask().HasFlag(RenderBackendFlag.Opengl)
        && Maplibre.OpenglSupportedContextProviderMask().HasFlag(OpenglContextProviderFlag.Wgl);

    internal override bool SupportsFrameAcquisition => false;

    internal WglOwnedTextureFixture()
    {
        thread = new Thread(Run) { IsBackground = true, Name = "WGL test window" };
        thread.Start();
        try
        {
            ready.Task.GetAwaiter().GetResult();
        }
        catch
        {
            thread.Join();
            stop.Dispose();
            throw;
        }
    }

    private void Run()
    {
        // GLFW window creation and destruction must stay on the same thread, even when
        // the test resumes an async operation on another worker.
        lock (GlfwLock)
        {
            Glfw? glfw = null;
            WindowHandle* window = null;
            nint hwnd = 0;
            nint library = 0;
            var initialized = false;
            try
            {
                string[] candidates =
                [
                    Path.Combine(
                        AppContext.BaseDirectory,
                        "runtimes",
                        RuntimeInformation.RuntimeIdentifier,
                        "native",
                        "glfw3.dll"
                    ),
                    Path.Combine(AppContext.BaseDirectory, "glfw3.dll"),
                    "glfw3.dll",
                ];
                foreach (var candidate in candidates)
                {
                    if (NativeLibrary.TryLoad(candidate, out library))
                        break;
                }
                if (library == 0)
                    throw new DllNotFoundException("GLFW is unavailable.");
                glfw = Glfw.GetApi();
                initialized = glfw.Init();
                if (!initialized)
                    throw new InvalidOperationException("GLFW initialization failed.");
                glfw.DefaultWindowHints();
                glfw.WindowHint(WindowHintBool.Visible, false);
                glfw.WindowHint(WindowHintInt.ContextVersionMajor, 3);
                glfw.WindowHint(WindowHintInt.ContextVersionMinor, 3);
                window = glfw.CreateWindow(64, 64, "Binding tests", null, null);
                if (window == null)
                    throw new InvalidOperationException("WGL window creation failed.");
                var getWindow = (delegate* unmanaged[Cdecl]<WindowHandle*, nint>)
                    NativeLibrary.GetExport(library, "glfwGetWin32Window");
                hwnd = getWindow(window);
                deviceContext = GetDC(hwnd);
                if (deviceContext == 0)
                    throw new InvalidOperationException("WGL device context is unavailable.");
                ready.SetResult();
                stop.Wait();
            }
            catch (Exception error)
            {
                ready.TrySetException(error);
            }
            finally
            {
                if (deviceContext != 0)
                    _ = ReleaseDC(hwnd, deviceContext);
                if (window != null)
                    glfw!.DestroyWindow(window);
                if (initialized)
                    glfw!.Terminate();
                glfw?.Dispose();
                if (library != 0)
                    NativeLibrary.Free(library);
            }
        }
    }

    private OpenglContextDescriptor Context() =>
        new(
            OpenglContextOwnership.Dedicated,
            new OpenglContextDescriptor.DataValue.Wgl(
                new WglContextDescriptor
                {
                    DeviceContext = NativePointer.FromBorrowedAddress(deviceContext),
                }
            )
        );

    internal override RenderSessionHandle Attach(MapHandle map, RenderTargetExtent extent) =>
        map.OpenglOwnedTextureAttach(
            new OpenglOwnedTextureDescriptor { Extent = extent, Context = Context() },
            new RenderSessionAttachOptions
            {
                Driver = RenderDriverKind.CoreWorker,
                RequestedTextureRingDepth = 2,
            }
        );

    internal override Task SetTargetAsync(RenderSessionHandle session, RenderTargetExtent extent) =>
        session.OpenglSurfaceSetTargetAsync(
            new OpenglSurfaceDescriptor
            {
                Extent = extent,
                Context = Context(),
                Surface = NativePointer.FromBorrowedAddress(deviceContext),
            },
            TestContext.Current.CancellationToken
        );

    public override void Dispose()
    {
        if (Interlocked.Exchange(ref disposed, 1) != 0)
            return;
        stop.Set();
        thread.Join();
        stop.Dispose();
    }

    [LibraryImport("user32.dll")]
    private static partial nint GetDC(nint window);

    [LibraryImport("user32.dll")]
    private static partial int ReleaseDC(nint window, nint deviceContext);
}
