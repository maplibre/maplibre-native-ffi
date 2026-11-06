// The map's window: a Win32 child window in the WPF window, which the session
// presents to. WPF can't draw over it, so the status shows in a StatusOverlay.
//
// Its mouse and key input turn into camera changes. A drag pans, a right or
// Ctrl drag rotates and pitches, the wheel zooms about the pointer, and a double
// click zooms in, or out with Shift. The arrow keys pan, and plus and minus
// zoom.
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Windows.Interop;

namespace Maplibre.NativeFfi.Example.Wpf;

internal sealed unsafe partial class MapView : HwndHost
{
    private const string ClassName = "MaplibreWpfExampleMap";
    private static bool registered;

    private bool dragging;
    private bool rotating;
    private ScreenPoint last;

    public MapController? Controller { get; set; }

    protected override HandleRef BuildWindowCore(HandleRef parent)
    {
        Register();
        var window = CreateWindowExW(
            0,
            ClassName,
            null,
            WS_CHILD | WS_VISIBLE,
            0,
            0,
            0,
            0,
            parent.Handle,
            0,
            GetModuleHandleW(null),
            0
        );
        if (window == 0)
            throw new Win32Exception();
        Controller?.WindowCreated(window);
        return new HandleRef(this, window);
    }

    protected override void DestroyWindowCore(HandleRef window)
    {
        Controller?.WindowDestroying(window.Handle);
        DestroyWindow(window.Handle);
    }

    protected override nint WndProc(
        nint window,
        int message,
        nint wParam,
        nint lParam,
        ref bool handled
    )
    {
        switch (message)
        {
            case WM_SIZE:
                Controller?.WindowResized((int)(lParam & 0xFFFF), (int)((lParam >> 16) & 0xFFFF));
                break;
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                SetFocus(window);
                SetCapture(window);
                dragging = true;
                rotating = message == WM_RBUTTONDOWN || (wParam & MK_CONTROL) != 0;
                last = Point(lParam);
                Apply(new CameraDelta { GesturePhase = GesturePhase.Begin });
                handled = true;
                break;
            case WM_MOUSEMOVE when dragging:
                var point = Point(lParam);
                var (dx, dy) = (point.X - last.X, point.Y - last.Y);
                last = point;
                Apply(
                    rotating
                        ? new CameraDelta
                        {
                            Bearing = dx * 0.5,
                            Pitch = -dy * 0.5,
                            GesturePhase = GesturePhase.Update,
                        }
                        : new CameraDelta
                        {
                            Offset = new ScreenPoint(dx, dy),
                            GesturePhase = GesturePhase.Update,
                        }
                );
                handled = true;
                break;
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
                if (dragging)
                {
                    dragging = false;
                    Apply(new CameraDelta { GesturePhase = GesturePhase.End });
                    ReleaseCapture();
                }
                handled = true;
                break;
            // Another window took the mouse during a drag.
            case WM_CAPTURECHANGED when dragging:
                dragging = false;
                Apply(new CameraDelta { GesturePhase = GesturePhase.Cancel });
                break;
            case WM_LBUTTONDBLCLK:
                ZoomBy((wParam & MK_SHIFT) != 0 ? 0.5 : 2, Point(lParam));
                handled = true;
                break;
            case WM_MOUSEWHEEL:
                // The wheel reports the pointer in screen coordinates.
                var screen = new POINT
                {
                    X = (short)(lParam & 0xFFFF),
                    Y = (short)((lParam >> 16) & 0xFFFF),
                };
                ScreenToClient(window, &screen);
                var notches = (short)((wParam >> 16) & 0xFFFF) / 120.0;
                // A wheel zooms about the pointer, by steps of 40 pixels.
                Apply(
                    new CameraDelta
                    {
                        Scale = Math.Pow(2, notches * 40 / 300),
                        Anchor = Logical(screen.X, screen.Y),
                    }
                );
                handled = true;
                break;
            case WM_KEYDOWN:
                handled = Key((int)wParam);
                break;
        }
        return base.WndProc(window, message, wParam, lParam, ref handled);
    }

    private bool Key(int key)
    {
        const double Step = 100;
        var delta = key switch
        {
            VK_LEFT => new CameraDelta { Offset = new ScreenPoint(Step, 0) },
            VK_RIGHT => new CameraDelta { Offset = new ScreenPoint(-Step, 0) },
            VK_UP => new CameraDelta { Offset = new ScreenPoint(0, Step) },
            VK_DOWN => new CameraDelta { Offset = new ScreenPoint(0, -Step) },
            VK_OEM_PLUS or VK_ADD => new CameraDelta { Scale = 2 },
            VK_OEM_MINUS or VK_SUBTRACT => new CameraDelta { Scale = 0.5 },
            _ => (CameraDelta?)null,
        };
        if (delta is null)
            return false;
        Apply(delta with { Animation = Animation() });
        return true;
    }

    private void ZoomBy(double scale, ScreenPoint anchor) =>
        Apply(
            new CameraDelta
            {
                Scale = scale,
                Anchor = anchor,
                Animation = Animation(),
            }
        );

    private void Apply(CameraDelta delta) => Controller?.Map?.Discarding.ApplyCameraDelta(delta);

    private static AnimationOptions Animation() => new() { DurationMs = 300 };

    // The map takes points in logical pixels, and Win32 reports them in pixels.
    private ScreenPoint Point(nint lParam) =>
        Logical((short)(lParam & 0xFFFF), (short)((lParam >> 16) & 0xFFFF));

    private ScreenPoint Logical(int x, int y)
    {
        var scale = Controller?.Scale ?? 1;
        return new ScreenPoint(x / scale, y / scale);
    }

    private static void Register()
    {
        if (registered)
            return;
        fixed (char* name = ClassName)
        {
            var windowClass = new WNDCLASSEXW
            {
                cbSize = (uint)sizeof(WNDCLASSEXW),
                style = CS_DBLCLKS,
                lpfnWndProc = &DefaultWindowProc,
                hInstance = GetModuleHandleW(null),
                hCursor = LoadCursorW(0, IDC_ARROW),
                lpszClassName = name,
            };
            if (RegisterClassExW(&windowClass) == 0)
                throw new Win32Exception();
        }
        registered = true;
    }

    // HwndHost subclasses the window once it's created, and calls WndProc.
    [UnmanagedCallersOnly]
    private static nint DefaultWindowProc(nint window, uint message, nint wParam, nint lParam) =>
        DefWindowProcW(window, message, wParam, lParam);

    private const int WS_CHILD = 0x40000000;
    private const int WS_VISIBLE = 0x10000000;
    private const uint CS_DBLCLKS = 0x0008;
    private const int IDC_ARROW = 32512;
    private const int WM_SIZE = 0x0005;
    private const int WM_KEYDOWN = 0x0100;
    private const int WM_MOUSEMOVE = 0x0200;
    private const int WM_LBUTTONDOWN = 0x0201;
    private const int WM_LBUTTONUP = 0x0202;
    private const int WM_LBUTTONDBLCLK = 0x0203;
    private const int WM_RBUTTONDOWN = 0x0204;
    private const int WM_RBUTTONUP = 0x0205;
    private const int WM_MOUSEWHEEL = 0x020A;
    private const int WM_CAPTURECHANGED = 0x0215;
    private const nint MK_SHIFT = 0x0004;
    private const nint MK_CONTROL = 0x0008;
    private const int VK_LEFT = 0x25;
    private const int VK_UP = 0x26;
    private const int VK_RIGHT = 0x27;
    private const int VK_DOWN = 0x28;
    private const int VK_ADD = 0x6B;
    private const int VK_SUBTRACT = 0x6D;
    private const int VK_OEM_PLUS = 0xBB;
    private const int VK_OEM_MINUS = 0xBD;

    private struct POINT
    {
        public int X;
        public int Y;
    }

    private struct WNDCLASSEXW
    {
        public uint cbSize;
        public uint style;
        public delegate* unmanaged<nint, uint, nint, nint, nint> lpfnWndProc;
        public int cbClsExtra;
        public int cbWndExtra;
        public nint hInstance;
        public nint hIcon;
        public nint hCursor;
        public nint hbrBackground;
        public char* lpszMenuName;
        public char* lpszClassName;
        public nint hIconSm;
    }

    [LibraryImport("user32.dll", SetLastError = true)]
    private static partial ushort RegisterClassExW(WNDCLASSEXW* windowClass);

    [LibraryImport("user32.dll", SetLastError = true, StringMarshalling = StringMarshalling.Utf16)]
    private static partial nint CreateWindowExW(
        int exStyle,
        string className,
        string? windowName,
        int style,
        int x,
        int y,
        int width,
        int height,
        nint parent,
        nint menu,
        nint instance,
        nint param
    );

    [LibraryImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool DestroyWindow(nint window);

    [LibraryImport("user32.dll")]
    private static partial nint DefWindowProcW(nint window, uint message, nint wParam, nint lParam);

    [LibraryImport("user32.dll")]
    private static partial nint LoadCursorW(nint instance, nint cursorName);

    [LibraryImport("user32.dll")]
    private static partial nint SetFocus(nint window);

    [LibraryImport("user32.dll")]
    private static partial nint SetCapture(nint window);

    [LibraryImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool ReleaseCapture();

    [LibraryImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool ScreenToClient(nint window, POINT* point);

    [LibraryImport("kernel32.dll", StringMarshalling = StringMarshalling.Utf16)]
    private static partial nint GetModuleHandleW(string? moduleName);
}
