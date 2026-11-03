//! Snippets for the docs site's Create a map page. They compile with the
//! tests, which never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

fn report_failure(_error: &Error) {}

// Attaches a render target, as the render target guides show.
async fn attach_target(
    _map: &MapHandle,
) -> Result<RenderSessionHandle> {
    unimplemented!()
}

async fn create_a_map() -> Result<()> {
    // #region runtime
    let runtime = RuntimeHandle::create(None)?;
    // #endregion runtime

    // #region map
    let map_options = MapOptions {
        map_mode: MapMode::Continuous,
        ..MapOptions::new(LogicalExtent::new(800, 600, 2.0))
    };

    let (map, created) = runtime.create_map(&map_options)?;
    created.await?;
    // #endregion map

    // #region style
    map.discarding()
        .set_style_url("https://demotiles.maplibre.org/style.json")?;
    // #endregion style

    let update = CameraUpdate {
        mode: CameraUpdateMode::Jump,
        camera: CameraOptions {
            zoom: Some(3.0),
            ..Default::default()
        },
        ..Default::default()
    };

    // #region wait
    map.update_camera(&update)?.await?;

    // Use the discarding variant to ignore the result.
    map.discarding().update_camera(&update)?;
    // #endregion wait

    // #region errors
    match map.update_camera(&update) {
        // The call failed right away.
        Err(error) => report_failure(&error),
        Ok(completion) => {
            // The result reports a failure.
            if let Err(error) = completion.await {
                report_failure(&error);
            }
        }
    }
    // #endregion errors

    let session = attach_target(&map).await?;

    // #region close
    drop(session);
    drop(map);
    runtime.release().map_err(|(_, error)| error)?.await?;
    // #endregion close

    Ok(())
}

fn main() {}
