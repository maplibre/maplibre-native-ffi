// Snippets for the docs site's Offline page. They compile with the tests,
// which never run them.

using MaplibreNativeFfi;

namespace Offline {
	void record_progress (
		int64 region_id, uint64 completed, uint64 required, bool complete
	) {
	}

	void record_error (int64 region_id, string message) {
	}

	void record_region (int64 region_id, GLib.Bytes? metadata) {
	}

	public async void offline () throws GLib.Error {
		// #region cache-file
		var options = new RuntimeOptions () {
			cache_path = "maplibre-cache.db",
		};
		var runtime = new Runtime.create (options);
		// #endregion cache-file

		// #region progress
		var handler = new RuntimeEventHandler ();
		handler.set_callback ((events) => {
			foreach (var event in events) {
				var payload = event.get_payload ();
				if (payload.is_offline_region_status ()) {
					var update = payload.get_offline_region_status ();
					record_progress (
						update.region_id,
						update.status.completed_resource_count,
						update.status.required_resource_count,
						update.status.complete
					);
				} else if (payload.is_offline_region_response_error ()) {
					var error =
						payload.get_offline_region_response_error ();
					record_error (error.region_id, event.get_message ());
				}
			}
		});
		runtime.set_event_handler_async.begin (handler);
		// #endregion progress

		// #region ambient
		// 50 MiB
		yield runtime.set_maximum_ambient_cache_size_async (
			50 * 1024 * 1024
		);
		yield runtime.run_cache_database_operation_async (
			CacheDatabaseOperation.CLEAR
		);
		// #endregion ambient

		// #region download
		var pyramid = new OfflineTilePyramidRegionDefinition () {
			style_url = "https://demotiles.maplibre.org/style.json",
			bounds = {
				{ latitude: 47.5, longitude: -122.5 },
				{ latitude: 47.8, longitude: -122.2 },
			},
			min_zoom = 0,
			max_zoom = 6,
			pixel_ratio = 1,
		};
		var region = yield runtime.create_offline_region_async (
			new OfflineRegionDefinition.tile_pyramid (pyramid),
			new GLib.Bytes ("Seattle".data)
		);

		yield runtime.set_offline_region_download_state_async (
			region.get_id (), OfflineRegionDownloadState.ACTIVE
		);
		// #endregion download

		// #region progress
		yield runtime.set_offline_region_observed_async (
			region.get_id (), RuntimeEventFlag.ALL_RUNTIME_EVENTS
		);
		// #endregion progress

		// #region manage
		var stored = yield runtime.list_offline_regions_async ();
		foreach (var info in stored) {
			record_region (info.get_id (), info.get_metadata ());
		}

		// Resume a download that stopped when the runtime was
		// released.
		yield runtime.set_offline_region_download_state_async (
			region.get_id (), OfflineRegionDownloadState.ACTIVE
		);

		yield runtime.invalidate_offline_region_async (region.get_id ());

		yield runtime.delete_offline_region_async (region.get_id ());
		// #endregion manage

		yield runtime.release_async ();
	}
}
