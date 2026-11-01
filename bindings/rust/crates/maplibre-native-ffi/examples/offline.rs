//! Snippets for the docs site's Offline page. They compile with the tests,
//! which never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

fn record_progress(_region_id: i64, _completed: u64, _required: u64, _complete: bool) {}

fn record_error(_region_id: i64, _message: &str) {}

fn record_region(_region_id: i64, _metadata: &[u8]) {}

async fn offline() -> Result<()> {
    // #region cache-file
    let runtime = RuntimeHandle::create(&RuntimeOptions {
        cache_path: "maplibre-cache.db",
        ..Default::default()
    })?;
    // #endregion cache-file

    // #region progress
    runtime
        .discarding()
        .set_event_handler(RuntimeEventHandler::new(|events| {
            for event in events {
                match event.payload {
                    RuntimeEventPayload::OfflineRegionStatus(payload) => record_progress(
                        payload.region_id,
                        payload.status.completed_resource_count,
                        payload.status.required_resource_count,
                        payload.status.complete,
                    ),
                    RuntimeEventPayload::OfflineRegionResponseError(payload) => {
                        record_error(payload.region_id, &event.message);
                    }
                    _ => {}
                }
            }
        }))?;
    // #endregion progress

    // #region ambient
    // 50 MiB
    runtime
        .set_maximum_ambient_cache_size(50 * 1024 * 1024)?
        .await?;
    runtime
        .run_ambient_cache_operation(AmbientCacheOperation::Clear)?
        .await?;
    // #endregion ambient

    // #region download
    let definition = OfflineRegionDefinition::TilePyramid(OfflineTilePyramidRegionDefinition {
        style_url: "https://demotiles.maplibre.org/style.json".to_owned(),
        bounds: LatLngBounds {
            southwest: LatLng::new(47.5, -122.5),
            northeast: LatLng::new(47.8, -122.2),
        },
        min_zoom: 0.0,
        max_zoom: 6.0,
        pixel_ratio: 1.0,
        ..Default::default()
    });

    let region = runtime
        .create_offline_region(&definition, b"Seattle")?
        .await?;

    runtime
        .set_offline_region_download_state(region.id, OfflineRegionDownloadState::Active)?
        .await?;
    // #endregion download

    // #region progress
    runtime
        .set_offline_region_observed(region.id, true)?
        .await?;
    // #endregion progress

    // #region manage
    for stored in runtime.list_offline_regions()?.await? {
        record_region(stored.id, &stored.metadata);
    }

    // Resume a download that stopped when the runtime closed.
    runtime
        .set_offline_region_download_state(region.id, OfflineRegionDownloadState::Active)?
        .await?;

    runtime.invalidate_offline_region(region.id)?.await?;

    runtime.delete_offline_region(region.id)?.await?;
    // #endregion manage

    runtime.release().map_err(|(_, error)| error)?.await?;

    Ok(())
}

fn main() {}
