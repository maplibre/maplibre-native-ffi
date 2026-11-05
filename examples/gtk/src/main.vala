// A window that the map fills, with a status line over it while the map loads
// or after it fails.

namespace Example {
	const string CSS = """
		window {
			background-color: #f2f0e8;
		}
		.status {
			margin: 0 20px 48px;
			padding: 8px 14px;
			border-radius: 8px;
			font-size: 15px;
			color: #212121;
			background-color: rgba(255, 255, 255, 0.92);
		}
		.status.error {
			color: #a1261c;
			background-color: #fdedeb;
		}
	""";

	// GTK deprecated GtkStyleContext but not this function, which Vala's
	// bindings deprecate with the class.
	[CCode (cname = "gtk_style_context_add_provider_for_display")]
	extern void add_provider_for_display (
		Gdk.Display display, Gtk.StyleProvider provider, uint priority
	);

	public class Application : Gtk.Application {
		public Application () {
			Object (application_id: "org.maplibre.nativeffi.example.gtk");
		}

		public override void activate () {
			var css = new Gtk.CssProvider ();
			css.load_from_string (CSS);
			add_provider_for_display (
				Gdk.Display.get_default (), css, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
			);

			var view = new MapView ();
			var status = new Gtk.Label ("");
			status.add_css_class ("status");
			status.halign = Gtk.Align.CENTER;
			status.valign = Gtk.Align.END;
			status.wrap = true;
			status.can_target = false;
			status.visible = false;
			view.status_changed.connect ((message, error) => {
				status.label = message;
				status.visible = message != "";
				if (error) {
					status.add_css_class ("error");
				}
			});

			var overlay = new Gtk.Overlay ();
			overlay.child = view;
			overlay.add_overlay (status);

			var window = new Gtk.ApplicationWindow (this);
			window.title = "MapLibre Native";
			window.set_default_size (1000, 700);
			window.child = overlay;

			// The window closes once the map's teardown finishes.
			var stopping = false;
			var stopped = false;
			window.close_request.connect (() => {
				if (stopped) {
					return false;
				}
				if (stopping) {
					return true;
				}
				stopping = true;
				view.stop.begin ((_, result) => {
					view.stop.end (result);
					stopped = true;
					window.close ();
				});
				return true;
			});

			view.start ();
			window.present ();
		}
	}

	int main (string[] args) {
		return new Application ().run (args);
	}
}
