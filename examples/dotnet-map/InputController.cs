using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Geo;
using Silk.NET.GLFW;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

/// <summary>Decodes host input into camera commands.</summary>
/// <remarks>
/// This runs on the render loop, which does not own the map, so it only queues commands for the
/// runtime loop. GLFW reports pointer positions in the map's logical coordinates already.
/// </remarks>
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
    private readonly CommandQueue commands;
    private readonly RenderRequest renderRequest;
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

    public InputController(GlfwWindow window, CommandQueue commands, RenderRequest renderRequest)
    {
        ArgumentNullException.ThrowIfNull(window);
        this.window = window;
        this.commands = commands;
        this.renderRequest = renderRequest;
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
            commands.Push(new AdjustBearingCommand(dx * DragRotateFactor, null));
            commands.Push(new AdjustPitchCommand(-dy * DragPitchFactor, null));
            renderRequest.Set();
        }
        else if (leftDown)
        {
            commands.Push(new MoveByCommand(dx, dy, null));
            renderRequest.Set();
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
        var wasDragging = Dragging;
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

            // Queued ahead of the drag's own commands, so the transition stops before the first
            // delta lands.
            commands.Push(new CancelTransitionsCommand());
        }

        if (Dragging != wasDragging)
        {
            commands.Push(new SetGestureInProgressCommand(Dragging));
        }
    }

    private bool Dragging => leftDown || rightDown;

    private void OnScroll(WindowHandle* handle, double xOffset, double yOffset)
    {
        _ = handle;
        _ = xOffset;
        var scale = Math.Pow(2.0, yOffset * 0.25);
        commands.Push(new ScaleByCommand(scale, new ScreenPoint(cursorX, cursorY), null));
        renderRequest.Set();
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
                commands.Push(new MoveByCommand(KeyboardPan, 0.0, KeyboardAnimation));
                break;
            case Keys.Right:
            case Keys.D:
                commands.Push(new MoveByCommand(-KeyboardPan, 0.0, KeyboardAnimation));
                break;
            case Keys.Up:
            case Keys.W:
                commands.Push(new MoveByCommand(0.0, KeyboardPan, KeyboardAnimation));
                break;
            case Keys.Down:
            case Keys.S:
                commands.Push(new MoveByCommand(0.0, -KeyboardPan, KeyboardAnimation));
                break;
            case Keys.Equal:
            case Keys.KeypadEqual:
                commands.Push(new ScaleByCommand(KeyboardZoom, null, KeyboardAnimation));
                break;
            case Keys.Minus:
                commands.Push(new ScaleByCommand(1.0 / KeyboardZoom, null, KeyboardAnimation));
                break;
            case Keys.Q:
                commands.Push(new AdjustBearingCommand(-KeyboardBearing, KeyboardAnimation));
                break;
            case Keys.E:
                commands.Push(new AdjustBearingCommand(KeyboardBearing, KeyboardAnimation));
                break;
            case Keys.RightBracket:
                commands.Push(new AdjustPitchCommand(KeyboardPitch, KeyboardAnimation));
                break;
            case Keys.LeftBracket:
                commands.Push(new AdjustPitchCommand(-KeyboardPitch, KeyboardAnimation));
                break;
            case Keys.Number0:
                commands.Push(new ResetOrientationCommand(ResetAnimation));
                break;
            default:
                changed = false;
                break;
        }

        if (changed)
        {
            renderRequest.Set();
        }
    }
}
