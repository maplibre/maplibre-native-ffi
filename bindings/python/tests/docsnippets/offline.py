# Snippets for the docs site's Offline page. They are type-checked with the
# tests, which never run them.

import maplibre_native_ffi as mln


def record_progress(
    region_id: int, completed: int, required: int, complete: bool
) -> None:
    pass


def record_error(region_id: int, message: str) -> None:
    pass


def record_region(region_id: int, metadata: bytes) -> None:
    pass


def offline() -> None:
    # #region cache-file
    runtime = mln.RuntimeHandle.create(
        mln.RuntimeOptions(cache_path="maplibre-cache.db")
    )
    # #endregion cache-file

    # #region progress
    def on_events(events: tuple[mln.RuntimeEvent, ...]) -> None:
        for event in events:
            match event.payload:
                case mln.RuntimeEventOfflineRegionStatusVariant(value=update):
                    record_progress(
                        update.region_id,
                        update.status.completed_resource_count,
                        update.status.required_resource_count,
                        update.status.complete,
                    )
                case mln.RuntimeEventOfflineRegionResponseErrorVariant(value=error):
                    record_error(error.region_id, event.message)

    runtime.discarding.set_event_handler(mln.RuntimeEventHandler(on_events))
    # #endregion progress

    # #region ambient
    # 50 MiB
    runtime.set_maximum_ambient_cache_size(50 * 1024 * 1024).result()
    runtime.run_ambient_cache_operation(mln.AmbientCacheOperation.CLEAR).result()
    # #endregion ambient

    # #region download
    region = runtime.create_offline_region(
        mln.OfflineRegionDefinitionTilePyramidVariant(
            mln.OfflineTilePyramidRegionDefinition(
                style_url="https://demotiles.maplibre.org/style.json",
                bounds=mln.LatLngBounds(
                    southwest=mln.LatLng(latitude=47.5, longitude=-122.5),
                    northeast=mln.LatLng(latitude=47.8, longitude=-122.2),
                ),
                min_zoom=0,
                max_zoom=6,
                pixel_ratio=1,
            )
        ),
        b"Seattle",
    ).result()

    runtime.set_offline_region_download_state(
        region.id, mln.OfflineRegionDownloadState.ACTIVE
    ).result()
    # #endregion download

    # #region progress
    runtime.set_offline_region_observed(region.id, True).result()
    # #endregion progress

    # #region manage
    for stored in runtime.list_offline_regions().result():
        record_region(stored.id, stored.metadata)

    # Resume a download that stopped when the runtime closed.
    runtime.set_offline_region_download_state(
        region.id, mln.OfflineRegionDownloadState.ACTIVE
    ).result()

    runtime.invalidate_offline_region(region.id).result()

    runtime.delete_offline_region(region.id).result()
    # #endregion manage

    runtime.close().result()
