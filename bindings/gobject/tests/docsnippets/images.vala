// Snippets for the docs site's Render images page. They compile with the
// tests, which never run them.

using MaplibreNativeFfi;

namespace Images {
	void copy_pixels (GLib.Bytes data, uint32 stride) {
	}

	public async void images (Runtime runtime) throws GLib.Error {
		// #region map
		var options = new MapOptions ();
		options.set_initial_extent (
			{ width: 800, height: 600, scale_factor: 2.0 }
		);
		options.set_map_mode (MapMode.STATIC);
		var map = runtime.create_map (options);
		yield map.ready_async ();
		// #endregion map

		// #region style
		map.set_style_url_async.begin (
			"https://demotiles.maplibre.org/style.json"
		);
		// #endregion style

		// #region attach
		var attach = new RenderSessionAttachOptions ();
		// The library renders on its own thread.
		attach.set_driver (RenderDriverKind.CORE_WORKER);
		var session = map.attach_offscreen (
			{ logical_size: { width: 800, height: 600 } }, attach, null
		);
		yield session.ready_async ();
		// #endregion attach

		// #region image
		var camera = new CameraOptions ();
		camera.set_center ({ latitude: 48.8566, longitude: 2.3522 });
		camera.set_zoom (4);
		var update = new CameraUpdate ();
		update.set_mode (CameraUpdateMode.JUMP);
		update.set_camera (camera);
		map.update_camera_async.begin (update);

		yield map.request_still_image_async ();
		// #endregion image

		// #region read
		var image = yield session.read_texture_async ();
		copy_pixels (image.get_data (), image.get_info ().stride);
		// #endregion read

		session.dispose ();
		yield map.release_async ();
	}

	public async void images_tile (
		Runtime runtime, uint8 z, uint32 x, uint32 y
	) throws GLib.Error {
		// #region tile
		var options = new MapOptions ();
		options.set_initial_extent (
			{ width: 512, height: 512, scale_factor: 1.0 }
		);
		options.set_map_mode (MapMode.TILE);
		var map = runtime.create_map (options);
		yield map.ready_async ();

		var update = new CameraUpdate ();
		update.set_mode (CameraUpdateMode.JUMP);
		update.set_camera (
			Camera.for_tile ({ z, x, y }, { width: 512, height: 512 })
		);
		map.update_camera_async.begin (update);
		// #endregion tile

		yield map.release_async ();
	}
}
