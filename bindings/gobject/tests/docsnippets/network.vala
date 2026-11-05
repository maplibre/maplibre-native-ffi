// Snippets for the docs site's Network page. They compile with the tests,
// which never run them.

using MaplibreNativeFfi;

namespace Network {
	string with_api_key (string url) {
		return url + (url.contains ("?") ? "&" : "?") + "key=my-api-key";
	}

	GLib.Bytes? find_tile (string url) {
		return null;
	}

	string geojson_for_tile (CanonicalTileId tile) {
		return "{\"type\":\"FeatureCollection\",\"features\":[]}";
	}

	public void network (Runtime runtime, Map map) throws GLib.Error {
		// #region headers
		// This library takes rules only, because it can't answer
		// on a library thread.
		var header = new HttpHeader () {
			name = "Authorization",
			value = "Bearer my-token",
		};
		var header_rule = new HttpHeaderRule () {
			flags = UrlMatchFlag.GLOB,
			url = "https://tiles.example.com/**",
		};
		header_rule.set_headers ({ header });
		var headers = new HttpHeaderTransform ();
		headers.set_rules ({ header_rule });
		runtime.set_http_header_transform_async.begin (headers);
		// #endregion headers

		// #region query-rules
		var parameter = new UrlQueryParameter () {
			name = "key",
			value = "my-api-key",
		};
		var query_rule = new ResourceTransformRule () {
			flags = UrlMatchFlag.GLOB,
			url = "https://tiles.example.com/**",
		};
		query_rule.set_parameters ({ parameter });
		var query = new ResourceTransform ();
		query.set_rules ({ query_rule });
		runtime.set_resource_transform_async.begin (query);
		// #endregion query-rules

		// #region rewrite
		var rewrite = new ResourceTransform ();
		rewrite.set_callback ((kind, url, request) => {
			try {
				request.complete (with_api_key (url));
			} catch (GLib.Error error) {
				warning ("%s", error.message);
			}
		});
		runtime.set_resource_transform_async.begin (rewrite);
		// #endregion rewrite

		// #region provider
		var route = new ResourceRoute () {
			kinds = ResourceKindFlag.TILE,
			flags = UrlMatchFlag.GLOB,
			url = "app://tiles/**",
		};
		var provider = new ResourceProvider ();
		provider.set_routes ({ route });
		provider.set_callback ((request, handle) => {
			var tile = find_tile (request.get_requested_url ());
			var response = new ResourceResponse () {
				status = ResourceResponseStatus.NO_CONTENT,
			};
			if (tile != null) {
				response.status = ResourceResponseStatus.OK;
				response.bytes = tile;
			}
			try {
				handle.complete (response);
			} catch (GLib.Error error) {
				warning ("%s", error.message);
			}
		});
		runtime.set_resource_provider_async.begin (provider);
		// #endregion provider

		// #region custom-source
		var source = new CustomGeometrySourceOptions ();
		source.set_fetch_tile ((source_map, source_id, tile) => {
			source_map.set_custom_geometry_source_tile_data_async.begin (
				source_id, tile, geojson_for_tile (tile)
			);
		});
		map.add_custom_geometry_source_async.begin ("app-data", source);
		// #endregion custom-source

		// #region status
		set_network_status (NetworkStatus.OFFLINE);

		// Later, when the connection returns:
		set_network_status (NetworkStatus.ONLINE);
		// #endregion status
	}
}
