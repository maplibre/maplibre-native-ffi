// Feeds the widget's raw input to a gesture recognizer, which moves the map's
// camera. A drag pans, a right or Ctrl drag rotates and pitches, touches pan,
// pinch, twist, and tilt, a touchpad pinch zooms and rotates, a mouse wheel
// zooms about the pointer, a touchpad scroll pans, and a double click zooms
// in, or out with Shift. The arrow keys pan, plus and minus zoom, and Escape
// drops a drag. A release keeps the map gliding.

using MaplibreNativeFfi;

namespace Example {
	public class Gestures : Object {
		private unowned Gtk.Widget widget;
		private GestureRecognizer? recognizer;
		// The pointers that went down and haven't lifted: the mouse's button,
		// and the touches, by their sequences.
		private uint button;
		private GenericSet<void*> touches = new GenericSet<void*> (direct_hash, direct_equal);
		// The touchpad pinch's scale so far.
		private double pinch_scale;
		// When the recognizer next needs a tick.
		private uint tick_id;
		private uint timeout_id;
		private bool every_frame;

		public Gestures (Gtk.Widget widget) {
			this.widget = widget;

			var events = new Gtk.EventControllerLegacy ();
			events.event.connect (on_event);
			widget.add_controller (events);

			var keys = new Gtk.EventControllerKey ();
			keys.key_pressed.connect (on_key);
			keys.modifiers.connect (on_modifiers);
			widget.add_controller (keys);

			// Drags and keys that the widget can't see the end of end here.
			var focus = new Gtk.EventControllerFocus ();
			focus.leave.connect (cancel);
			widget.add_controller (focus);
		}

		// Moves `map`'s camera from now on, or nothing's.
		public void set_map (Map? map) throws GLib.Error {
			cancel ();
			if (recognizer != null) {
				recognizer.release ();
				recognizer = null;
			}
			if (map != null) {
				recognizer = map.gesture_recognizer_create (null);
			}
		}

		private bool on_event (Gdk.Event event) {
			if (event.get_pointer_emulated ()) {
				return false;
			}
			switch (event.get_event_type ()) {
			case Gdk.EventType.BUTTON_PRESS:
				// The primary button drags, the secondary one rotates and
				// tilts, and the others mean nothing to the map.
				var pressed = ((Gdk.ButtonEvent) event).get_button ();
				if (button != 0
					|| (pressed != Gdk.BUTTON_PRIMARY && pressed != Gdk.BUTTON_SECONDARY)) {
					return false;
				}
				widget.grab_focus ();
				button = pressed;
				var kind = event.get_device ().source == Gdk.InputSource.PEN
					? PointerKind.PEN : PointerKind.MOUSE;
				return handle (new GestureInput.pointer_down ({
					id: 0,
					kind: kind,
					position: position (event),
					modes: pressed == Gdk.BUTTON_SECONDARY ? GestureMode.ROTATE : (GestureMode) 0,
				}));
			case Gdk.EventType.MOTION_NOTIFY:
				return handle (new GestureInput.pointer_move ({ id: 0, position: position (event) }));
			case Gdk.EventType.BUTTON_RELEASE:
				if (((Gdk.ButtonEvent) event).get_button () != button) {
					return false;
				}
				button = 0;
				return handle (new GestureInput.pointer_up ({ id: 0, position: position (event) }));
			case Gdk.EventType.TOUCH_BEGIN:
				widget.grab_focus ();
				touches.add (event.get_event_sequence ());
				return handle (new GestureInput.pointer_down ({
					id: touch_id (event.get_event_sequence ()),
					kind: PointerKind.TOUCH,
					position: position (event),
				}));
			case Gdk.EventType.TOUCH_UPDATE:
				return touches.contains (event.get_event_sequence ())
					&& handle (new GestureInput.pointer_move ({
						id: touch_id (event.get_event_sequence ()),
						position: position (event),
					}));
			case Gdk.EventType.TOUCH_END:
				return touches.remove (event.get_event_sequence ())
					&& handle (new GestureInput.pointer_up ({
						id: touch_id (event.get_event_sequence ()),
						position: position (event),
					}));
			case Gdk.EventType.TOUCH_CANCEL:
				return touches.remove (event.get_event_sequence ())
					&& handle (new GestureInput.pointer_cancel ({
						id: touch_id (event.get_event_sequence ()),
					}));
			case Gdk.EventType.SCROLL:
				return on_scroll ((Gdk.ScrollEvent) event);
			case Gdk.EventType.TOUCHPAD_PINCH:
				return on_pinch ((Gdk.TouchpadEvent) event);
			default:
				return false;
			}
		}

		// A wheel's detents zoom a third of a level each, eased, and a
		// touchpad's scrolling pans.
		private bool on_scroll (Gdk.ScrollEvent event) {
			double dx = 0, dy = 0;
			switch (event.get_direction ()) {
			case Gdk.ScrollDirection.UP:
				dy = -1;
				break;
			case Gdk.ScrollDirection.DOWN:
				dy = 1;
				break;
			case Gdk.ScrollDirection.SMOOTH:
				event.get_deltas (out dx, out dy);
				break;
			default:
				return false;
			}
			if (dx == 0 && dy == 0) {
				return false;
			}
			GestureDelta delta = { position: position (event) };
			if (event.get_unit () == Gdk.ScrollUnit.WHEEL) {
				delta.discrete = true;
				delta.zoom = -dy / 3;
			} else {
				delta.pan = { x: -dx, y: -dy };
			}
			return handle (new GestureInput.delta (delta));
		}

		// GTK reports the pinch's scale since it began, and its turn since
		// the last event, clockwise in radians.
		private bool on_pinch (Gdk.TouchpadEvent event) {
			if (event.get_gesture_phase () == Gdk.TouchpadGesturePhase.BEGIN) {
				pinch_scale = 1;
			}
			GesturePinch pinch = {
				center: position (event),
				scale: event.get_pinch_scale () / pinch_scale,
				rotation: event.get_pinch_angle_delta () * 180 / Math.PI,
			};
			pinch_scale = event.get_pinch_scale ();
			switch (event.get_gesture_phase ()) {
			case Gdk.TouchpadGesturePhase.BEGIN:
				pinch.phase = PinchPhase.BEGIN;
				break;
			case Gdk.TouchpadGesturePhase.UPDATE:
				pinch.phase = PinchPhase.UPDATE;
				break;
			case Gdk.TouchpadGesturePhase.END:
				pinch.phase = PinchPhase.END;
				break;
			default:
				pinch.phase = PinchPhase.CANCEL;
				break;
			}
			return handle (new GestureInput.pinch (pinch));
		}

		private bool on_key (uint keyval, uint keycode, Gdk.ModifierType state) {
			GestureDelta delta = { position: center (), discrete: true };
			switch (keyval) {
			case Gdk.Key.Left:
				delta.pan = { x: 100, y: 0 };
				break;
			case Gdk.Key.Right:
				delta.pan = { x: -100, y: 0 };
				break;
			case Gdk.Key.Up:
				delta.pan = { x: 0, y: 100 };
				break;
			case Gdk.Key.Down:
				delta.pan = { x: 0, y: -100 };
				break;
			case Gdk.Key.plus:
			case Gdk.Key.equal:
			case Gdk.Key.KP_Add:
				delta.zoom = 1;
				break;
			case Gdk.Key.minus:
			case Gdk.Key.KP_Subtract:
				delta.zoom = -1;
				break;
			case Gdk.Key.Escape:
				if (button == 0) {
					return false;
				}
				button = 0;
				return handle (new GestureInput.pointer_cancel ({ id: 0 }));
			default:
				return false;
			}
			return handle (new GestureInput.delta (delta));
		}

		// Ctrl rotates and tilts on a drag, and Shift zooms out on a double
		// click.
		private bool on_modifiers (Gdk.ModifierType state) {
			GestureMode modes = 0;
			if (Gdk.ModifierType.CONTROL_MASK in state) {
				modes |= GestureMode.ROTATE;
			}
			if (Gdk.ModifierType.SHIFT_MASK in state) {
				modes |= GestureMode.ZOOM_OUT;
			}
			handle (new GestureInput.modes (modes));
			return false;
		}

		// Cancels the pointers that went down, and lets go of the keys.
		private void cancel () {
			if (button != 0) {
				button = 0;
				handle (new GestureInput.pointer_cancel ({ id: 0 }));
			}
			foreach (var touch in touches) {
				handle (new GestureInput.pointer_cancel ({ id: touch_id (touch) }));
			}
			touches.remove_all ();
			handle (new GestureInput.modes (0));
		}

		// Whether the recognizer used the input.
		private bool handle (GestureInput input) {
			if (recognizer == null) {
				return false;
			}
			try {
				var result = recognizer.handle (input, now ());
				schedule_tick (result);
				return result.get_handled ();
			} catch (GLib.Error error) {
				warning ("%s", error.message);
				return false;
			}
		}

		// Ticks the recognizer when its last result asks: at every frame of
		// the widget's frame clock while something moves, or at a time.
		private void schedule_tick (GestureResult result) {
			if (timeout_id != 0) {
				Source.remove (timeout_id);
				timeout_id = 0;
			}
			every_frame = result.get_next_tick () == GestureNextTick.FRAME;
			if (every_frame && tick_id == 0) {
				tick_id = widget.add_tick_callback (on_frame);
			} else if (result.get_next_tick () == GestureNextTick.AT) {
				var delay = Math.ceil (result.get_next_tick_ms () - now ());
				timeout_id = Timeout.add ((uint) double.max (0, delay), on_timeout);
			}
		}

		private bool on_frame (Gtk.Widget widget, Gdk.FrameClock clock) {
			tick ();
			if (every_frame) {
				return Source.CONTINUE;
			}
			tick_id = 0;
			return Source.REMOVE;
		}

		private bool on_timeout () {
			timeout_id = 0;
			tick ();
			return Source.REMOVE;
		}

		private void tick () {
			if (recognizer == null) {
				every_frame = false;
				return;
			}
			try {
				schedule_tick (recognizer.tick (now ()));
			} catch (GLib.Error error) {
				warning ("%s", error.message);
			}
		}

		// Inputs and ticks share one clock, in milliseconds.
		private static double now () {
			return get_monotonic_time () / 1000.0;
		}

		// GDK numbers touches with small integers.
		private static uint32 touch_id (void* sequence) {
			return (uint32) (size_t) sequence;
		}

		// An event's position in the widget. GDK reports it in the
		// surface, which may also hold the window's shadow.
		private ScreenPoint position (Gdk.Event event) {
			double x, y, offset_x, offset_y;
			event.get_position (out x, out y);
			var native = widget.get_native ();
			native.get_surface_transform (out offset_x, out offset_y);
			Graphene.Point point;
			((Gtk.Widget) native).compute_point (
				widget, { (float) (x - offset_x), (float) (y - offset_y) }, out point
			);
			return { x: point.x, y: point.y };
		}

		private ScreenPoint center () {
			return { x: widget.get_width () / 2.0, y: widget.get_height () / 2.0 };
		}
	}
}
