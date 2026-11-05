// Snippets for the docs site's Events and logs page. They compile with the
// tests, which never run them.

using MaplibreNativeFfi;

namespace EventsAndLogs {
	void on_style_loaded () {
	}

	void on_loading_failed (string message) {
	}

	void write_log (LogSeverity severity, string message) {
	}

	public async void events_and_logs (Runtime runtime) throws GLib.Error {
		Map? map = null;

		// #region handler
		var handler = new RuntimeEventHandler ();
		// The handler runs on the main context of the thread that
		// set it.
		handler.set_callback ((events) => {
			foreach (var event in events) {
				if (map == null || !map.is_event_source (event)) {
					continue;
				}
				switch (event.get_event_type ()) {
				case RuntimeEventType.MAP_STYLE_LOADED:
					on_style_loaded ();
					break;
				case RuntimeEventType.MAP_LOADING_FAILED:
					on_loading_failed (event.get_message ());
					break;
				default:
					break;
				}
			}
		});
		runtime.set_event_handler_async.begin (handler);
		// #endregion handler

		// #region mask
		var options = new MapOptions ();
		options.set_initial_extent (
			{ width: 512, height: 512, scale_factor: 1.0 }
		);
		options.set_event_mask (
			RuntimeEventFlag.MAP_STYLE_LOADED
			| RuntimeEventFlag.MAP_LOADING_FAILED
		);
		map = runtime.create_map (options);
		// #endregion mask
		yield map.ready_async ();

		// #region logs
		var logs = new LogHandler ();
		// The callback runs on the main context of the thread that
		// set it.
		logs.set_callback ((severity, event, code, message) => {
			write_log (severity, message);
		});
		set_log_callback (logs);
		// #endregion logs
	}
}
