// The map's window: a Win32 child window in the WPF window, which the session
// presents to. WPF can't draw over it, so the status shows in a StatusOverlay.
//
// It passes its mouse, touch, pen, wheel, and key input to the map's gesture
// recognizer, which moves the camera. A drag pans, a right or Ctrl drag rotates
// and pitches, and two fingers also pinch and rotate. A flick glides on, and a
// double click zooms in, or out with Shift. The wheel zooms about the pointer,
// the arrow keys pan, and plus and minus zoom.
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Windows.Interop;

namespace Maplibre.NativeFfi.Example.Wpf;

internal sealed unsafe partial class MapView : HwndHost
{
    private const string ClassName = "MaplibreWpfExampleMap";
    private const uint MouseId = 0;
    private static bool registered;

    // The pointers that went down on the map and haven't lifted yet.
    private readonly HashSet<uint> pointers = [];

    // The release of the button that started the mouse's drag.
    private int dragButtonUp;

    // The modes that held keys select, as the recognizer last got them.
    private GestureMode keyModes;

    // The client size in pixels.
    private (int Width, int Height) size;

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
                size = ((int)(lParam & 0xFFFF), (int)((lParam >> 16) & 0xFFFF));
                Controller?.WindowResized(size.Width, size.Height);
                break;
            // The left button drags, and the right one rotates and pitches.
            // The others mean nothing to the map.
            case WM_LBUTTONDOWN or WM_RBUTTONDOWN when !pointers.Contains(MouseId):
                SetFocus(window);
                SetCapture(window);
                dragButtonUp = message == WM_RBUTTONDOWN ? WM_RBUTTONUP : WM_LBUTTONUP;
                handled = Down(
                    MouseId,
                    PointerKind.Mouse,
                    Point(lParam),
                    message == WM_RBUTTONDOWN ? GestureMode.Rotate : default
                );
                break;
            case WM_MOUSEMOVE when pointers.Contains(MouseId):
                handled = Move(MouseId, Point(lParam));
                break;
            case WM_LBUTTONUP or WM_RBUTTONUP:
                if (message != dragButtonUp || !pointers.Contains(MouseId))
                    break;
                handled = Up(MouseId, Point(lParam));
                // The mouse is up, so the WM_CAPTURECHANGED that follows does
                // nothing.
                ReleaseCapture();
                break;
            // Another window took the mouse during a drag.
            case WM_CAPTURECHANGED when pointers.Contains(MouseId):
                Cancel(MouseId);
                break;
            // Touch and pen input. Handling it keeps Windows from turning it into
            // mouse input.
            case WM_POINTERDOWN:
                SetFocus(window);
                handled = Down(
                    PointerId(wParam),
                    KindOf(wParam),
                    FromScreen(window, lParam),
                    default
                );
                break;
            case WM_POINTERUPDATE when pointers.Contains(PointerId(wParam)):
                handled = Move(PointerId(wParam), FromScreen(window, lParam));
                break;
            case WM_POINTERUP when pointers.Contains(PointerId(wParam)):
                handled = Up(PointerId(wParam), FromScreen(window, lParam));
                break;
            case WM_POINTERCAPTURECHANGED when pointers.Contains(PointerId(wParam)):
                Cancel(PointerId(wParam));
                break;
            // A wheel's detent zooms a third of a level, eased. A precise wheel
            // or a touchpad scrolls by less, which zooms at once.
            case WM_MOUSEWHEEL:
                var wheel = (short)((wParam >> 16) & 0xFFFF);
                handled = Input(
                    new GestureInput.Delta(
                        new GestureDelta
                        {
                            Position = FromScreen(window, lParam),
                            Discrete = wheel % WHEEL_DELTA == 0,
                            Zoom = wheel / (3.0 * WHEEL_DELTA),
                        }
                    )
                );
                break;
            case WM_KEYDOWN:
                UpdateKeyModes();
                handled = Key((int)wParam);
                break;
            case WM_KEYUP:
                UpdateKeyModes();
                break;
            // Once the window loses the focus, it can't see drags and held keys
            // end.
            case WM_KILLFOCUS:
                var dragging = pointers.Contains(MouseId);
                foreach (var id in pointers.ToArray())
                    Cancel(id);
                if (dragging)
                    ReleaseCapture();
                SetKeyModes(default);
                break;
        }
        return base.WndProc(window, message, wParam, lParam, ref handled);
    }

    private bool Down(uint id, PointerKind kind, ScreenPoint position, GestureMode modes)
    {
        UpdateKeyModes();
        pointers.Add(id);
        return Input(
            new GestureInput.PointerDown(new GesturePointerDown(id, kind, position, modes))
        );
    }

    private bool Move(uint id, ScreenPoint position) =>
        Input(new GestureInput.PointerMove(new GesturePointer(id, position)));

    private bool Up(uint id, ScreenPoint position)
    {
        pointers.Remove(id);
        return Input(new GestureInput.PointerUp(new GesturePointer(id, position)));
    }

    private void Cancel(uint id)
    {
        pointers.Remove(id);
        Input(new GestureInput.PointerCancel(new GesturePointer(id, default)));
    }

    private bool Input(GestureInput input) => Controller?.Input(input) ?? false;

    // Ctrl makes a drag rotate and pitch, and Shift makes a double click zoom
    // out. The view can't draw over the map, so it has no box zoom to show.
    private void UpdateKeyModes() =>
        SetKeyModes(
            (GetKeyState(VK_CONTROL) < 0 ? GestureMode.Rotate : default)
                | (GetKeyState(VK_SHIFT) < 0 ? GestureMode.ZoomOut : default)
        );

    private void SetKeyModes(GestureMode modes)
    {
        if (modes == keyModes)
            return;
        keyModes = modes;
        Input(new GestureInput.Modes(modes));
    }

    // The arrow keys pan 100 pixels, and plus and minus zoom a level, eased.
    private bool Key(int key)
    {
        var delta = key switch
        {
            VK_LEFT => new GestureDelta { Pan = new ScreenPoint(100, 0) },
            VK_RIGHT => new GestureDelta { Pan = new ScreenPoint(-100, 0) },
            VK_UP => new GestureDelta { Pan = new ScreenPoint(0, 100) },
            VK_DOWN => new GestureDelta { Pan = new ScreenPoint(0, -100) },
            VK_OEM_PLUS or VK_ADD => new GestureDelta { Zoom = 1 },
            VK_OEM_MINUS or VK_SUBTRACT => new GestureDelta { Zoom = -1 },
            _ => (GestureDelta?)null,
        };
        if (delta is null)
            return false;
        return Input(
            new GestureInput.Delta(
                delta.Value with
                {
                    Position = Logical(size.Width / 2, size.Height / 2),
                    Discrete = true,
                }
            )
        );
    }

    // The recognizer's ID for a touch or a pen: its Win32 pointer ID plus one,
    // which keeps it apart from the mouse's.
    private static uint PointerId(nint wParam) => (uint)(wParam & 0xFFFF) + 1;

    private static PointerKind KindOf(nint wParam)
    {
        int type;
        return GetPointerType((uint)(wParam & 0xFFFF), &type) && type == PT_PEN
            ? PointerKind.Pen
            : PointerKind.Touch;
    }

    // The map takes points in logical pixels, and Win32 reports them in pixels.
    private ScreenPoint Point(nint lParam) =>
        Logical((short)(lParam & 0xFFFF), (short)((lParam >> 16) & 0xFFFF));

    // The wheel and pointer messages report the pointer in screen coordinates.
    private ScreenPoint FromScreen(nint window, nint lParam)
    {
        var point = new POINT
        {
            X = (short)(lParam & 0xFFFF),
            Y = (short)((lParam >> 16) & 0xFFFF),
        };
        ScreenToClient(window, &point);
        return Logical(point.X, point.Y);
    }

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
    private const int IDC_ARROW = 32512;
    private const int WM_SIZE = 0x0005;
    private const int WM_KILLFOCUS = 0x0008;
    private const int WM_KEYDOWN = 0x0100;
    private const int WM_KEYUP = 0x0101;
    private const int WM_MOUSEMOVE = 0x0200;
    private const int WM_LBUTTONDOWN = 0x0201;
    private const int WM_LBUTTONUP = 0x0202;
    private const int WM_RBUTTONDOWN = 0x0204;
    private const int WM_RBUTTONUP = 0x0205;
    private const int WM_MOUSEWHEEL = 0x020A;
    private const int WM_CAPTURECHANGED = 0x0215;
    private const int WM_POINTERUPDATE = 0x0245;
    private const int WM_POINTERDOWN = 0x0246;
    private const int WM_POINTERUP = 0x0247;
    private const int WM_POINTERCAPTURECHANGED = 0x024C;
    private const int WHEEL_DELTA = 120;
    private const int PT_PEN = 3;
    private const int VK_SHIFT = 0x10;
    private const int VK_CONTROL = 0x11;
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
    private static partial short GetKeyState(int key);

    [LibraryImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool GetPointerType(uint pointerId, int* pointerType);

    [LibraryImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool ScreenToClient(nint window, POINT* point);

    [LibraryImport("kernel32.dll", StringMarshalling = StringMarshalling.Utf16)]
    private static partial nint GetModuleHandleW(string? moduleName);
}
