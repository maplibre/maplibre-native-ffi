# Snippets for the docs site's Events and logs page. They are type-checked with
# the tests, which never run them.

import maplibre_native_ffi as mln


def on_style_loaded() -> None:
    pass


def on_loading_failed(message: str) -> None:
    pass


def write_log(severity: mln.LogSeverity, message: str) -> None:
    pass


def events_and_logs(runtime: mln.RuntimeHandle) -> None:
    map_handle: mln.MapHandle | None = None

    # #region handler
    def on_events(events: tuple[mln.RuntimeEvent, ...]) -> None:
        for event in events:
            if map_handle is None or event.source != map_handle.id:
                continue

            match event.type:
                case mln.RuntimeEventType.MAP_STYLE_LOADED:
                    on_style_loaded()
                case mln.RuntimeEventType.MAP_LOADING_FAILED:
                    on_loading_failed(event.message)

    # The handler runs on the binding's dispatcher thread.
    runtime.discarding.set_event_handler(mln.RuntimeEventHandler(on_events))
    # #endregion handler

    # #region mask
    map_handle, ready = runtime.create_map(
        mln.MapOptions(
            initial_extent=mln.LogicalExtent(width=512, height=512, scale_factor=1.0),
            event_mask=mln.RuntimeEventFlag.MAP_STYLE_LOADED
            | mln.RuntimeEventFlag.MAP_LOADING_FAILED,
        )
    )
    # #endregion mask
    ready.result()

    # #region logs
    # The callback runs on the binding's dispatcher thread.
    def on_log(
        severity: mln.LogSeverity, event: mln.LogEvent, code: int, message: str
    ) -> None:
        write_log(severity, message)

    mln.set_log_callback(mln.LogHandler(on_log))
    # #endregion logs
