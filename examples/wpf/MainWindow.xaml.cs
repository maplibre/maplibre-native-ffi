using System.ComponentModel;
using System.Windows;
using System.Windows.Media;

namespace Maplibre.NativeFfi.Example.Wpf;

public partial class MainWindow : Window
{
    private readonly MapController controller;
    private bool closing;
    private bool closed;

    public MainWindow()
    {
        InitializeComponent();
        controller = new MapController(Dispatcher);
        Map.Controller = controller;
    }

    // The map starts once the window knows its display's scale.
    protected override void OnSourceInitialized(EventArgs e)
    {
        base.OnSourceInitialized(e);
        controller.Start(VisualTreeHelper.GetDpi(this).DpiScaleX);
    }

    // A window can own the overlay only once it has been shown.
    protected override void OnContentRendered(EventArgs e)
    {
        base.OnContentRendered(e);
        _ = new StatusOverlay(this, Map, controller);
    }

    // The session presents to the map's window, which goes when this one does,
    // so closing waits for the map to release the session first.
    protected override async void OnClosing(CancelEventArgs e)
    {
        base.OnClosing(e);
        if (closed)
            return;
        e.Cancel = true;
        if (closing)
            return;
        closing = true;
        await controller.DisposeAsync();
        closed = true;
        Close();
    }
}
