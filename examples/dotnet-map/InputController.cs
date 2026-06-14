namespace Maplibre.Native.Examples.DotnetMap;

internal sealed class InputController : IDisposable
{
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

    public void Dispose() { }
}
