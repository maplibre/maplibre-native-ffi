// Snippets for the docs site's Camera page. They compile with the tests,
// which never run them.

using MaplibreNativeFfi;

namespace CameraSnippets {
	void use_camera (Camera camera) {
	}

	void fly_ended (bool completed) {
	}

	// #region gesture
	GestureRecognizer create_recognizer (Map map) throws GLib.Error {
		return map.gesture_recognizer_create (null);
	}

	void schedule_tick (GestureResult result) {
		// On the next frame, or at result.get_next_tick_ms ().
	}

	void pointer_down (
		GestureRecognizer recognizer,
		uint32 id,
		ScreenPoint position,
		double time_ms
	) throws GLib.Error {
		var input = new GestureInput.pointer_down ({
			id: id, kind: PointerKind.TOUCH, position: position,
		});
		schedule_tick (recognizer.handle (input, time_ms));
	}

	void pointer_move (
		GestureRecognizer recognizer,
		uint32 id,
		ScreenPoint position,
		double time_ms
	) throws GLib.Error {
		var input = new GestureInput.pointer_move ({
			id: id, position: position,
		});
		schedule_tick (recognizer.handle (input, time_ms));
	}

	void tick (GestureRecognizer recognizer, double time_ms) throws GLib.Error {
		schedule_tick (recognizer.tick (time_ms));
	}
	// #endregion gesture

	public async void camera (Map map) throws GLib.Error {
		// #region read
		var snapshot = map.get_snapshot ();
		use_camera (snapshot.view.camera);

		var result = yield map.get_camera_async ();
		use_camera (result.camera);
		// #endregion read

		// #region jump
		var paris = new CameraOptions () {
			center = { latitude: 48.8566, longitude: 2.3522 },
			zoom = 12,
		};
		var jump = new CameraUpdate () {
			mode = CameraUpdateMode.JUMP,
			camera = paris,
		};
		map.update_camera_async.begin (jump);
		// #endregion jump

		// #region animate
		var end_handler = new CameraTransitionHandler ();
		// Runs on the main context of the thread that started the
		// update.
		end_handler.set_callback ((end) => {
			fly_ended (
				end.outcome == CameraTransitionOutcome.COMPLETED
			);
		});
		var animation = new AnimationOptions () {
			duration_ms = 3000,
			end_handler = end_handler,
		};

		var new_york = new CameraOptions () {
			center = { latitude: 40.7128, longitude: -74.006 },
			zoom = 10,
		};
		var fly = new CameraUpdate () {
			mode = CameraUpdateMode.FLY,
			camera = new_york,
			animation = animation,
		};
		map.update_camera_async.begin (fly);
		// #endregion animate

		// #region fit
		LatLngBounds bounds = {
			{ latitude: 45.8, longitude: 5.9 },
			{ latitude: 47.8, longitude: 10.5 },
		};
		var ease = new AnimationOptions () { duration_ms = 500 };
		var fit = new CameraFitUpdate () {
			mode = CameraUpdateMode.EASE,
			animation = ease,
		};
		map.fit_camera_to_lat_lng_bounds_async.begin (bounds, fit);
		// #endregion fit

		// #region limit
		var limits = new BoundOptions () {
			bounds = bounds,
			min_zoom = 5,
			max_zoom = 16,
		};
		map.set_bounds_async.begin (limits);
		// #endregion limit

		var recognizer = create_recognizer (map);
		pointer_down (recognizer, 1, { x: 100, y: 100 }, 0);
		pointer_move (recognizer, 1, { x: 112, y: 96 }, 16);
		tick (recognizer, 32);
		recognizer.release ();

		// #region convert
		var projection = new MapProjection.create (
			map.get_snapshot ().view
		);
		var coordinate = projection.lat_lng_for_pixel ({ x: 100, y: 200 });
		var point = projection.pixel_for_lat_lng (coordinate);
		// #endregion convert
		assert (point != null);
	}
}
