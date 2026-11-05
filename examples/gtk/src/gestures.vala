// Gestures, as camera deltas. A drag pans, a right or Ctrl drag rotates and
// pitches, two fingers or a touchpad pinch zoom and rotate, a mouse wheel
// zooms about the pointer, a touchpad scroll pans, and a double click zooms
// in, or out with Shift. The arrow keys pan, and plus and minus zoom.

using MaplibreNativeFfi;

namespace Example {
	public class Gestures : Object {
		public signal void delta (CameraDelta delta);

		// A Vala closure that uses a local holds the instance, which would
		// keep the gestures, and the view, alive from the handlers of the
		// controllers that the widget keeps. Handlers that use only fields
		// hold the instance weakly.
		private unowned Gtk.Widget widget;
		private Gtk.GestureDrag drag;
		private Gtk.GestureZoom zoom;
		private Gtk.GestureRotate rotate;
		private Gtk.EventControllerScroll scroll;
		private Gtk.GestureClick click;
		private double previous_x;
		private double previous_y;
		private bool rotating;
		private double previous_scale;
		private double previous_angle;
		private double pointer_x;
		private double pointer_y;

		public Gestures (Gtk.Widget widget) {
			this.widget = widget;

			drag = new Gtk.GestureDrag ();
			drag.button = 0;
			drag.drag_begin.connect ((x, y) => {
				this.widget.grab_focus ();
				previous_x = 0;
				previous_y = 0;
				var state = drag.get_current_event_state ();
				rotating = drag.get_current_button () == Gdk.BUTTON_SECONDARY
					|| Gdk.ModifierType.CONTROL_MASK in state;
				apply (phase (GesturePhase.BEGIN));
			});
			drag.drag_update.connect ((offset_x, offset_y) => {
				var dx = offset_x - previous_x;
				var dy = offset_y - previous_y;
				previous_x = offset_x;
				previous_y = offset_y;
				var delta = phase (GesturePhase.UPDATE);
				if (rotating) {
					delta.set_bearing (dx * 0.5);
					delta.set_pitch (-dy * 0.5);
				} else {
					delta.set_offset ({ x: dx, y: dy });
				}
				apply (delta);
			});
			drag.drag_end.connect (() => apply (phase (GesturePhase.END)));
			drag.cancel.connect (() => apply (phase (GesturePhase.CANCEL)));
			widget.add_controller (drag);

			// The span between two touches scales and turns about their
			// midpoint.
			zoom = new Gtk.GestureZoom ();
			zoom.begin.connect (() => {
				previous_scale = 1;
				apply (phase (GesturePhase.BEGIN));
			});
			zoom.scale_changed.connect ((scale) => {
				var delta = phase (GesturePhase.UPDATE);
				delta.set_scale (scale / previous_scale);
				previous_scale = scale;
				delta.set_anchor (center (zoom));
				apply (delta);
			});
			zoom.end.connect (() => apply (phase (GesturePhase.END)));
			widget.add_controller (zoom);

			rotate = new Gtk.GestureRotate ();
			rotate.begin.connect (() => previous_angle = 0);
			// The delta is the turn since the gesture began.
			rotate.angle_changed.connect ((angle, angle_delta) => {
				var turn = (angle_delta - previous_angle) * 180 / Math.PI;
				previous_angle = angle_delta;
				var delta = phase (GesturePhase.UPDATE);
				delta.set_bearing (-wrap_degrees (turn));
				delta.set_anchor (center (rotate));
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

			scroll = new Gtk.EventControllerScroll (
				Gtk.EventControllerScrollFlags.BOTH_AXES
			);
			scroll.scroll_begin.connect (() => apply (phase (GesturePhase.BEGIN)));
			scroll.scroll_end.connect (() => apply (phase (GesturePhase.END)));
			scroll.scroll.connect ((dx, dy) => {
				var delta = new CameraDelta ();
				if (scroll.get_unit () == Gdk.ScrollUnit.WHEEL) {
					// A wheel zooms about the pointer, by steps of 40 pixels.
					delta.set_scale (Math.pow (2, -dy * 40 / 300));
					delta.set_anchor ({ x: pointer_x, y: pointer_y });
				} else {
					// A touchpad scroll pans.
					delta.set_offset ({ x: -dx, y: -dy });
					delta.set_gesture_phase (GesturePhase.UPDATE);
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
					delta.set_offset ({ x: STEP, y: 0 });
					break;
				case Gdk.Key.Right:
					delta.set_offset ({ x: -STEP, y: 0 });
					break;
				case Gdk.Key.Up:
					delta.set_offset ({ x: 0, y: STEP });
					break;
				case Gdk.Key.Down:
					delta.set_offset ({ x: 0, y: -STEP });
					break;
				case Gdk.Key.plus:
				case Gdk.Key.equal:
				case Gdk.Key.KP_Add:
					delta.set_scale (2);
					break;
				case Gdk.Key.minus:
				case Gdk.Key.KP_Subtract:
					delta.set_scale (0.5);
					break;
				default:
					return false;
				}
				delta.set_animation (animation ());
				apply (delta);
				return true;
			});
			widget.add_controller (keys);
		}

		private void zoom_by (double scale, ScreenPoint anchor) {
			var delta = new CameraDelta ();
			delta.set_scale (scale);
			delta.set_anchor (anchor);
			delta.set_animation (animation ());
			apply (delta);
		}

		private void apply (CameraDelta delta) {
			this.delta (delta);
		}

		private static CameraDelta phase (GesturePhase phase) {
			var delta = new CameraDelta ();
			delta.set_gesture_phase (phase);
			return delta;
		}

		private static AnimationOptions animation () {
			var animation = new AnimationOptions ();
			animation.set_duration_ms (300);
			return animation;
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
	}
}
