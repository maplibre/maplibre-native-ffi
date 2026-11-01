//! Snippets for the docs site's Create a map page. They compile with the
//! tests, which never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

// Attaches a render target, as the Rendering guides show.
async fn attach_target(_map: &MapHandle) -> Result<RenderSessionHandle> {
    unimplemented!()
}

async fn create_a_map() -> Result<()> {
    // #region runtime
    let runtime = RuntimeHandle::create(&RuntimeOptions::default())?;
    // #endregion runtime

    // #region map
    let map_options = MapOptions {
        map_mode: MapMode::Continuous,
        ..MapOptions::new(LogicalExtent::new(800, 600, 2.0))
    };

    let (map, created) = runtime.create_map(&map_options)?;
    created.await?;
    // #endregion map

    let session = attach_target(&map).await?;

    // #region close
    drop(session);
    map.release().map_err(|(_, error)| error)?.await?;
    runtime.release().map_err(|(_, error)| error)?.await?;
    // #endregion close

    Ok(())
}

fn main() {}
