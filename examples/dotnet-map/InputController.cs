using Maplibre.Native.Camera;
using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Silk.NET.GLFW;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe class InputController : IDisposable
{
    private const double DragRotateFactor = 0.5;
    private const double DragPitchFactor = 0.5;
    private const double KeyboardPan = 120.0;
    private const double KeyboardZoom = 1.25;
    private const double KeyboardBearing = 10.0;
    private const double KeyboardPitch = 5.0;
    private static readonly AnimationOptions KeyboardAnimation = new() { Duration = 160 };
    private static readonly AnimationOptions ResetAnimation = new() { Duration = 220 };

    private readonly GlfwWindow window;
    private readonly MapHandle map;
    private readonly Action renderRequested;
    private readonly GlfwCallbacks.CursorPosCallback cursorCallback;
    private readonly GlfwCallbacks.MouseButtonCallback mouseButtonCallback;
    private readonly GlfwCallbacks.ScrollCallback scrollCallback;
    private readonly GlfwCallbacks.KeyCallback keyCallback;
    private bool leftDown;
    private bool rightDown;
    private bool ctrlDown;
    private double lastX;
    private double lastY;
    private double cursorX;
    private double cursorY;
    private bool closed;

    public InputController(GlfwWindow window, MapHandle map, Action renderRequested)
    {
        this.window = window;
        this.map = map;
        this.renderRequested = renderRequested;
        cursorCallback = OnCursor;
        mouseButtonCallback = OnMouseButton;
        scrollCallback = OnScroll;
        keyCallback = OnKey;
        window.Glfw.SetCursorPosCallback(window.Handle, cursorCallback);
        window.Glfw.SetMouseButtonCallback(window.Handle, mouseButtonCallback);
        window.Glfw.SetScrollCallback(window.Handle, scrollCallback);
        window.Glfw.SetKeyCallback(window.Handle, keyCallback);
    }

    public static void PrintControls()
    {
        Console.WriteLine(
            """
            Controls:
              left drag: pan
              right drag or Ctrl+left drag: rotate with X, pitch with Y
              scroll: zoom at cursor
              arrows or WASD: pan
              + / -: zoom at center
              Q / E: rotate
              ] / [: pitch
              0: reset pitch and bearing
            """
        );
    }

    public void Dispose()
    {
        if (closed)
        {
            return;
        }

        closed = true;
        window.Glfw.SetCursorPosCallback(window.Handle, null);
        window.Glfw.SetMouseButtonCallback(window.Handle, null);
        window.Glfw.SetScrollCallback(window.Handle, null);
        window.Glfw.SetKeyCallback(window.Handle, null);
    }

    private void OnCursor(WindowHandle* handle, double x, double y)
    {
        _ = handle;
        cursorX = x;
        cursorY = y;
        var dx = x - lastX;
        var dy = y - lastY;
        lastX = x;
        lastY = y;
        if (rightDown || (leftDown && ctrlDown))
        {
            SetBearing(CurrentBearing() + dx * DragRotateFactor, animated: false);
            SetPitch(CurrentPitch() - dy * DragPitchFactor, animated: false);
            renderRequested();
        }
        else if (leftDown)
        {
            map.MoveBy(dx, dy);
            renderRequested();
        }
    }

    private void OnMouseButton(
        WindowHandle* handle,
        MouseButton button,
        InputAction action,
        KeyModifiers mods
    )
    {
        _ = handle;
        ctrlDown = (mods & KeyModifiers.Control) != 0;
        if (button == MouseButton.Left)
        {
            leftDown =
                action == InputAction.Press ? true
                : action == InputAction.Release ? false
                : leftDown;
        }
        else if (button == MouseButton.Right)
        {
            rightDown =
                action == InputAction.Press ? true
                : action == InputAction.Release ? false
                : rightDown;
        }

        if (action == InputAction.Press)
        {
            window.Glfw.GetCursorPos(handle, out cursorX, out cursorY);
            lastX = cursorX;
            lastY = cursorY;
            map.CancelTransitions();
        }
    }

    private void OnScroll(WindowHandle* handle, double xOffset, double yOffset)
    {
        _ = handle;
        _ = xOffset;
        var scale = Math.Pow(2.0, yOffset * 0.25);
        map.ScaleBy(scale, new ScreenPoint(cursorX, cursorY));
        renderRequested();
    }

    private void OnKey(
        WindowHandle* handle,
        Keys key,
        int scanCode,
        InputAction action,
        KeyModifiers mods
    )
    {
        _ = handle;
        _ = scanCode;
        ctrlDown = (mods & KeyModifiers.Control) != 0;
        if (action != InputAction.Press && action != InputAction.Repeat)
        {
            return;
        }

        var changed = true;
        switch (key)
        {
            case Keys.Left:
            case Keys.A:
                map.MoveByAnimated(KeyboardPan, 0.0, KeyboardAnimation);
                break;
            case Keys.Right:
            case Keys.D:
                map.MoveByAnimated(-KeyboardPan, 0.0, KeyboardAnimation);
                break;
            case Keys.Up:
            case Keys.W:
                map.MoveByAnimated(0.0, KeyboardPan, KeyboardAnimation);
                break;
            case Keys.Down:
            case Keys.S:
                map.MoveByAnimated(0.0, -KeyboardPan, KeyboardAnimation);
                break;
            case Keys.Equal:
            case Keys.KeypadEqual:
                map.ScaleByAnimated(KeyboardZoom, null, KeyboardAnimation);
                break;
            case Keys.Minus:
                map.ScaleByAnimated(1.0 / KeyboardZoom, null, KeyboardAnimation);
                break;
            case Keys.Q:
                SetBearing(CurrentBearing() - KeyboardBearing, animated: true);
                break;
            case Keys.E:
                SetBearing(CurrentBearing() + KeyboardBearing, animated: true);
                break;
            case Keys.RightBracket:
                SetPitch(CurrentPitch() + KeyboardPitch, animated: true);
                break;
            case Keys.LeftBracket:
                SetPitch(CurrentPitch() - KeyboardPitch, animated: true);
                break;
            case Keys.Number0:
                map.EaseTo(new CameraOptions { Bearing = 0.0, Pitch = 0.0 }, ResetAnimation);
                break;
            default:
                changed = false;
                break;
        }

        if (changed)
        {
            renderRequested();
        }
    }

    private double CurrentBearing()
    {
        return map.GetCamera().Bearing ?? 0.0;
    }

    private double CurrentPitch()
    {
        return map.GetCamera().Pitch ?? 0.0;
    }

    private void SetBearing(double bearing, bool animated)
    {
        var camera = new CameraOptions { Bearing = bearing };
        if (animated)
        {
            map.EaseTo(camera, KeyboardAnimation);
        }
        else
        {
            map.JumpTo(camera);
        }
    }

    private void SetPitch(double pitch, bool animated)
    {
        var clamped = Math.Clamp(pitch, 0.0, 60.0);
        var camera = new CameraOptions { Pitch = clamped };
        if (animated)
        {
            map.EaseTo(camera, KeyboardAnimation);
        }
        else
        {
            map.JumpTo(camera);
        }
    }
}
