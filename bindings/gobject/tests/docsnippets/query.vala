// Snippets for the docs site's Query page. They compile with the tests,
// which never run them.

using MaplibreNativeFfi;

namespace Query {
	void use_feature (string feature) {
	}

	public async void query (
		RenderSession session, ScreenPoint point
	) throws GLib.Error {
		// #region rendered
		ScreenBox box = {
			{ point.x - 5, point.y - 5 }, { point.x + 5, point.y + 5 }
		};
		var options = new RenderedFeatureQueryOptions ();
		options.set_layer_ids ({ "poi" });
		var hits = yield session.query_rendered_features_async (
			new RenderedQueryGeometry.box (box), options
		);
		foreach (var hit in hits) {
			// The feature is GeoJSON text.
			use_feature (hit.get_feature ());
		}
		// #endregion rendered

		// #region source
		var filter = new SourceFeatureQueryOptions ();
		filter.set_source_layer_ids ({ "transportation" });
		// A filter from the MapLibre style spec.
		filter.set_filter ("[\"==\", [\"get\", \"class\"], \"motorway\"]");
		var features = yield session.query_source_features_async (
			"openmaptiles", filter
		);
		foreach (var feature in features) {
			use_feature (feature.get_feature ());
		}
		// #endregion source
	}
}
