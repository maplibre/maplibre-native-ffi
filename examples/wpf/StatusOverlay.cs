// The status, over the map. WPF can't draw over the map's child window, so the
// status is a window of its own. The main window owns it, so it stays above the
// main window and minimizes with it, and it follows the map's top-left corner.
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace Maplibre.NativeFfi.Example.Wpf;

internal sealed class StatusOverlay : Window
{
    private readonly FrameworkElement anchor;
    private readonly TextBlock text = new()
    {
        Margin = new Thickness(12, 8, 12, 8),
        MaxWidth = 480,
        TextWrapping = TextWrapping.Wrap,
    };

    public StatusOverlay(Window owner, FrameworkElement anchor, MapController controller)
    {
        this.anchor = anchor;
        Owner = owner;
        WindowStyle = WindowStyle.None;
        ResizeMode = ResizeMode.NoResize;
        SizeToContent = SizeToContent.WidthAndHeight;
        ShowInTaskbar = false;
        ShowActivated = false;
        Focusable = false;
        Content = text;
        owner.LocationChanged += (_, _) => Follow();
        owner.SizeChanged += (_, _) => Follow();
        controller.StatusChanged += Update;
        Update(controller.Status);
    }

    private void Update(Status status)
    {
        if (status.Message.Length == 0)
        {
            Hide();
            return;
        }
        text.Text = status.Message;
        Background = status.Error ? Brushes.DarkRed : Brushes.White;
        text.Foreground = status.Error ? Brushes.White : Brushes.Black;
        Follow();
        Show();
    }

    private void Follow()
    {
        if (PresentationSource.FromVisual(anchor) is not { CompositionTarget: { } target })
            return;
        // The screen point is in pixels, and a window's position in logical
        // pixels.
        var corner = target.TransformFromDevice.Transform(anchor.PointToScreen(new Point(12, 12)));
        Left = corner.X;
        Top = corner.Y;
    }
}
