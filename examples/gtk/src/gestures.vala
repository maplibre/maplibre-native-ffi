// Gestures, as camera deltas. A drag pans, a right or Ctrl drag rotates and
// pitches, two fingers or a touchpad pinch pan, zoom, and rotate, a mouse wheel
// zooms about the pointer, a touchpad scroll pans, and a double click zooms
// in, or out with Shift. The arrow keys pan, and plus and minus zoom. A drag,
// a pinch, or a touchpad scroll keeps moving after release, until a press or
// a touch on the touchpad stops it.

using MaplibreNativeFfi;

namespace Example {
	public class Gestures : Object {
		public signal void delta (CameraDelta delta);

		// A Vala closure that uses a local holds the instance, which would
		// keep the gestures, and the view, alive from the handlers of the
		// controllers that the widget keeps. Handlers that use only fields
		// hold the instance weakly.
		private unowned Gtk.Widget widget;
		private Gtk.GestureSwipe drag;
		private Gtk.GestureZoom zoom;
		private Gtk.GestureRotate rotate;
		private Gtk.EventControllerScroll scroll;
		private Gtk.GestureClick click;
		private bool cancelled;
		private double previous_x;
		private double previous_y;
		private bool rotating;
		private ScreenPoint previous_center;
		private double previous_scale;
		private double previous_angle;
		private Rate pan_x_rate = new Rate ();
		private Rate pan_y_rate = new Rate ();
		private Rate zoom_rate = new Rate ();
		private Rate bearing_rate = new Rate ();
		private double pointer_x;
		private double pointer_y;

		public Gestures (Gtk.Widget widget) {
			this.widget = widget;

			// A swipe is a drag that reports its velocity at release. GTK
			// ends a cancelled gesture as well, so the end checks for that.
			drag = new Gtk.GestureSwipe ();
			drag.button = 0;
			drag.begin.connect ((sequence) => {
				this.widget.grab_focus ();
				cancelled = false;
				drag.get_point (sequence, out previous_x, out previous_y);
				var state = drag.get_current_event_state ();
				rotating = drag.get_current_button () == Gdk.BUTTON_SECONDARY
					|| Gdk.ModifierType.CONTROL_MASK in state;
				apply (phase (GesturePhase.BEGIN));
			});
			drag.update.connect ((sequence) => {
				double x, y;
				drag.get_point (sequence, out x, out y);
				var delta = phase (GesturePhase.UPDATE);
				if (rotating) {
					delta.bearing = (x - previous_x) * 0.5;
					delta.pitch = -(y - previous_y) * 0.5;
				} else {
					delta.offset = { x: x - previous_x, y: y - previous_y };
				}
				previous_x = x;
				previous_y = y;
				apply (delta);
			});
			drag.cancel.connect (() => cancelled = true);
			drag.swipe.connect ((velocity_x, velocity_y) => {
				var momentum = new GestureMomentum ();
				if (rotating) {
					momentum.bearing.velocity = velocity_x * 0.5;
					momentum.pitch.velocity = -velocity_y * 0.5;
				} else {
					momentum.pan.velocity = { x: velocity_x, y: velocity_y };
				}
				apply (cancelled ? phase (GesturePhase.CANCEL) : end (momentum));
			});
			widget.add_controller (drag);

			// The span between two touches scales and turns about their
			// midpoint, which pans as it moves. The pinch takes over both
			// touches from the drag.
			zoom = new Gtk.GestureZoom ();
			zoom.begin.connect (() => {
				drag.set_state (Gtk.EventSequenceState.DENIED);
				cancelled = false;
				previous_center = center (zoom);
				previous_scale = 1;
				var time = zoom.get_current_event_time ();
				pan_x_rate.start (time);
				pan_y_rate.start (time);
				zoom_rate.start (time);
				apply (phase (GesturePhase.BEGIN));
			});
			zoom.scale_changed.connect ((scale) => {
				var anchor = center (zoom);
				var dx = anchor.x - previous_center.x;
				var dy = anchor.y - previous_center.y;
				var time = zoom.get_current_event_time ();
				pan_x_rate.add (dx, time);
				pan_y_rate.add (dy, time);
				zoom_rate.add (Math.log2 (scale / previous_scale), time);
				var delta = phase (GesturePhase.UPDATE);
				delta.offset = { x: dx, y: dy };
				delta.scale = scale / previous_scale;
				delta.anchor = anchor;
				previous_center = anchor;
				previous_scale = scale;
				apply (delta);
			});
			zoom.cancel.connect (() => cancelled = true);
			zoom.end.connect (() => {
				var time = zoom.get_current_event_time ();
				var momentum = new GestureMomentum ();
				momentum.pan.velocity = {
					x: pan_x_rate.at (time),
					y: pan_y_rate.at (time),
				};
				momentum.zoom.velocity = zoom_rate.at (time);
				momentum.zoom.anchor = previous_center;
				momentum.bearing.velocity = bearing_rate.at (time);
				momentum.bearing.anchor = previous_center;
				apply (cancelled ? phase (GesturePhase.CANCEL) : end (momentum));
			});
			widget.add_controller (zoom);

			rotate = new Gtk.GestureRotate ();
			rotate.begin.connect (() => {
				previous_angle = 0;
				bearing_rate.start (rotate.get_current_event_time ());
			});
			// The delta is the turn since the gesture began.
			rotate.angle_changed.connect ((angle, angle_delta) => {
				var turn = (angle_delta - previous_angle) * 180 / Math.PI;
				previous_angle = angle_delta;
				var delta = phase (GesturePhase.UPDATE);
				delta.bearing = -wrap_degrees (turn);
				delta.anchor = center (rotate);
				bearing_rate.add (delta.bearing, rotate.get_current_event_time ());
				apply (delta);
			});
			widget.add_controller (rotate);
			rotate.group (zoom);

			var motion = new Gtk.EventControllerMotion ();
			motion.motion.connect ((x, y) => {
				pointer_x = x;
				pointer_y = y;
			});
			widget.add_controller (motion);

			// A touchpad scroll, or a touch on the touchpad, begins a
			// gesture, and its release velocity follows its end.
			scroll = new Gtk.EventControllerScroll (
				Gtk.EventControllerScrollFlags.BOTH_AXES
				| Gtk.EventControllerScrollFlags.KINETIC
			);
			scroll.scroll_begin.connect (() => apply (phase (GesturePhase.BEGIN)));
			scroll.decelerate.connect ((velocity_x, velocity_y) => {
				var momentum = new GestureMomentum ();
				momentum.pan.velocity = { x: -velocity_x, y: -velocity_y };
				apply (end (momentum));
			});
			scroll.scroll.connect ((dx, dy) => {
				var delta = new CameraDelta ();
				if (scroll.get_unit () == Gdk.ScrollUnit.WHEEL) {
					// A wheel zooms about the pointer, by steps of 40 pixels.
					delta.scale = Math.pow (2, -dy * 40 / 300);
					delta.anchor = { x: pointer_x, y: pointer_y };
				} else {
					// A touchpad scroll pans.
					delta.offset = { x: -dx, y: -dy };
					delta.gesture_phase = GesturePhase.UPDATE;
				}
				apply (delta);
				return true;
			});
			widget.add_controller (scroll);

			click = new Gtk.GestureClick ();
			click.pressed.connect ((presses, x, y) => {
				if (presses != 2) {
					return;
				}
				var state = click.get_current_event_state ();
				zoom_by (Gdk.ModifierType.SHIFT_MASK in state ? 0.5 : 2, { x: x, y: y });
			});
			widget.add_controller (click);

			var keys = new Gtk.EventControllerKey ();
			keys.key_pressed.connect ((keyval, keycode, state) => {
				const double STEP = 100;
				var delta = new CameraDelta ();
				switch (keyval) {
				case Gdk.Key.Left:
					delta.offset = { x: STEP, y: 0 };
					break;
				case Gdk.Key.Right:
					delta.offset = { x: -STEP, y: 0 };
					break;
				case Gdk.Key.Up:
					delta.offset = { x: 0, y: STEP };
					break;
				case Gdk.Key.Down:
					delta.offset = { x: 0, y: -STEP };
					break;
				case Gdk.Key.plus:
				case Gdk.Key.equal:
				case Gdk.Key.KP_Add:
					delta.scale = 2;
					break;
				case Gdk.Key.minus:
				case Gdk.Key.KP_Subtract:
					delta.scale = 0.5;
					break;
				default:
					return false;
				}
				delta.animation = animation ();
				apply (delta);
				return true;
			});
			widget.add_controller (keys);
		}

		private void zoom_by (double scale, ScreenPoint anchor) {
			apply (new CameraDelta () {
				scale = scale,
				anchor = anchor,
				animation = animation (),
			});
		}

		private static CameraDelta end (GestureMomentum momentum) {
			var delta = phase (GesturePhase.END);
			delta.momentum = momentum;
			return delta;
		}

		private void apply (CameraDelta delta) {
			this.delta (delta);
		}

		private static CameraDelta phase (GesturePhase phase) {
			return new CameraDelta () { gesture_phase = phase };
		}

		private static AnimationOptions animation () {
			return new AnimationOptions () { duration_ms = 300 };
		}

		private static ScreenPoint center (Gtk.Gesture gesture) {
			double x, y;
			gesture.get_bounding_box_center (out x, out y);
			return { x: x, y: y };
		}

		// An angle in degrees, wrapped to [-180, 180).
		private static double wrap_degrees (double degrees) {
			return Math.fmod (Math.fmod (degrees + 180, 360) + 360, 360) - 180;
		}

		// How fast a pinch changes, per second, which Gtk.GestureZoom and
		// Gtk.GestureRotate don't report. Each change's rate averages with the
		// earlier ones, and a pause before the release stops it.
		private class Rate {
			private double per_second;
			private uint32 time;

			public void start (uint32 time) {
				per_second = 0;
				this.time = time;
			}

			public void add (double change, uint32 time) {
				if (time > this.time) {
					per_second = (per_second + change * 1000 / (time - this.time)) / 2;
					this.time = time;
				}
			}

			public double at (uint32 time) {
				return time - this.time < 50 ? per_second : 0;
			}
		}
	}
}
