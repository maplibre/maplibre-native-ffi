// Snippets for the docs site's Style page. They compile with the tests,
// which never run them.

using MaplibreNativeFfi;

namespace Style {
	PremultipliedRgba8Image draw_image (string image_id) {
		return new PremultipliedRgba8Image () {
			width = 1,
			height = 1,
			stride = 4,
			pixels = new GLib.Bytes ({ 255, 0, 0, 255 }),
		};
	}

	// pixels holds 16 x 16 premultiplied RGBA pixels.
	public async void style (Map map, GLib.Bytes pixels) throws GLib.Error {
		// #region load
		yield map.set_style_url_async (
			"https://demotiles.maplibre.org/style.json"
		);
		// #endregion load

		// #region add
		map.add_style_source_json_async.begin (
			"satellite",
			"""{"type": "raster", "tileSize": 256,
			"tiles": ["https://tiles.example.com/{z}/{x}/{y}.png"]}"""
		);

		// Draw the layer under the "countries-label" layer.
		map.add_style_layer_json_async.begin (
			"""{"id": "satellite", "type": "raster",
			"source": "satellite"}""",
			"countries-label"
		);
		// #endregion add

		// #region geojson
		map.add_geo_json_source_async.begin ("points", null, null);
		map.set_geo_json_source_data_async.begin (
			"points", """{"type": "Point", "coordinates": [-122.4, 37.8]}"""
		);
		// #endregion geojson

		// #region set-property
		// Show both changes in the same frame.
		map.begin_command_group_async.begin ();
		map.set_style_layer_property_async.begin (
			"satellite", "visibility", "\"none\""
		);
		map.set_style_layer_property_async.begin (
			"countries-boundary",
			"line-color",
			"""["interpolate", ["linear"], ["zoom"],
			5, "#9cf", 12, "#06c"]"""
		);
		map.end_command_group_async.begin ();
		// #endregion set-property

		// #region image
		var marker = new PremultipliedRgba8Image () {
			width = 16,
			height = 16,
			stride = 16 * 4,
			pixels = pixels,
		};
		map.set_style_image_async.begin ("marker", marker, null);

		// An empty layer ID draws the layer on top.
		map.add_style_layer_json_async.begin (
			"""{"id": "markers", "type": "symbol", "source": "points",
			"layout": {"icon-image": "marker"}}""",
			null
		);
		// #endregion image
	}

	public void style_events (Runtime runtime, Map map) {
		// #region missing-image
		var handler = new RuntimeEventHandler ();
		// Runs on the main context of the thread that set it.
		handler.set_callback ((events) => {
			foreach (var event in events) {
				var payload = event.get_payload ();
				if (!map.is_event_source (event)
					|| !payload.is_style_image ()) {
					continue;
				}
				var image_id = payload.get_style_image ().get_image_id ();
				map.set_style_image_async.begin (
					image_id, draw_image (image_id), null
				);
			}
		});
		runtime.set_event_handler_async.begin (handler);
		// #endregion missing-image
	}
}
