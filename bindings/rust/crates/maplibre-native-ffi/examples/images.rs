//! Snippets for the docs site's Render images page. They compile with the
//! tests, which never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

fn copy_pixels(_data: &[u8], _stride: u32) {}

async fn images(runtime: &RuntimeHandle) -> Result<()> {
    // #region map
    let map_options = MapOptions {
        map_mode: MapMode::Static,
        ..MapOptions::new(LogicalExtent::new(800, 600, 2.0))
    };

    let (map, created) = runtime.create_map(&map_options)?;
    created.await?;
    // #endregion map

    // #region style
    map.discarding()
        .set_style_url("https://demotiles.maplibre.org/style.json")?;
    // #endregion style

    // #region attach
    let offscreen =
        OffscreenDescriptor::new(LogicalSize::new(800, 600));
    // The library renders on its own thread.
    let attach_options =
        RenderSessionAttachOptions::new(RenderDriverKind::CoreWorker);

    // With no device, the session creates its own.
    let (session, attached) =
        map.attach_offscreen(&offscreen, &attach_options, None)?;
    attached.await?;
    // #endregion attach

    // #region image
    map.discarding().update_camera(&CameraUpdate {
        mode: CameraUpdateMode::Jump,
        camera: CameraOptions {
            center: Some(LatLng::new(48.8566, 2.3522)),
            zoom: Some(4.0),
            ..Default::default()
        },
        ..Default::default()
    })?;

    map.request_still_image()?.await?;
    // #endregion image

    // #region read
    let image = session.read_texture()?.await?;
    copy_pixels(&image.data, image.info.stride);
    // #endregion read

    drop(session);
    map.release()?.await?;

    Ok(())
}

async fn images_tile(
    runtime: &RuntimeHandle,
    z: u32,
    x: u32,
    y: u32,
) -> Result<()> {
    // #region tile
    let map_options = MapOptions {
        map_mode: MapMode::Tile,
        ..MapOptions::new(LogicalExtent::new(512, 512, 1.0))
    };
    let (map, created) = runtime.create_map(&map_options)?;
    created.await?;

    map.discarding().update_camera(&CameraUpdate {
        mode: CameraUpdateMode::Jump,
        camera: camera_for_tile(
            CanonicalTileId::new(z, x, y),
            LogicalSize::new(512, 512),
        )?,
        ..Default::default()
    })?;
    // #endregion tile

    map.release()?.await?;

    Ok(())
}

fn main() {}
