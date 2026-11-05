// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.

using MaplibreNativeFfi;

namespace CreateAMap {
	void report_failure (GLib.Error error) {
	}

	// Attaches a render target, as the render target guides show.
	RenderSession attach_target (Map map) throws GLib.Error {
		assert_not_reached ();
	}

	public async void create_a_map () throws GLib.Error {
		// #region runtime
		var runtime = new Runtime.create (null);
		// #endregion runtime

		// #region map
		var options = new MapOptions () {
			initial_extent = { width: 800, height: 600, scale_factor: 2.0 },
			map_mode = MapMode.CONTINUOUS,
		};

		var map = runtime.create_map (options);
		yield map.ready_async ();
		// #endregion map

		// #region style
		map.set_style_url_async.begin (
			"https://demotiles.maplibre.org/style.json"
		);
		// #endregion style

		var camera = new CameraOptions () { zoom = 3 };
		var update = new CameraUpdate ();
		update.set_mode (CameraUpdateMode.JUMP);
		update.set_camera (camera);

		// #region wait
		yield map.update_camera_async (update);

		// Start it without a callback to ignore the result.
		map.update_camera_async.begin (update);
		// #endregion wait

		// #region errors
		try {
			yield map.update_camera_async (update);
		} catch (GLib.Error error) {
			// Its message describes the failure.
			report_failure (error);
		}
		// #endregion errors

		var session = attach_target (map);

		// #region close
		// Releasing the last reference disposes each handle without
		// waiting for it to retire. The session goes first, since it
		// keeps the map, and the map keeps the runtime.
		session = null;
		map = null;
		runtime = null;
		// #endregion close
	}
}
