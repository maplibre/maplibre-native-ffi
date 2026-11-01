//! Snippets for the docs site's Render images page. They compile with the
//! tests, which never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

// The center of tile z/x/y. The snippet leaves the tile math to the reader.
fn tile_center(_z: u32, _x: u32, _y: u32) -> LatLng {
    LatLng::default()
}

fn copy_pixels(_data: &[u8], _stride: u32) {}

async fn images(runtime: &RuntimeHandle, z: u32, x: u32, y: u32) -> Result<()> {
    // #region map
    let extent = LogicalExtent::new(512, 512, 1.0);
    let map_options = MapOptions {
        map_mode: MapMode::Tile,
        ..MapOptions::new(extent)
    };

    let (map, created) = runtime.create_map(&map_options)?;
    created.await?;
    // #endregion map

    // #region style
    map.discarding()
        .set_style_url("https://demotiles.maplibre.org/style.json")?;
    // #endregion style

    // #region attach
    // None selects the default adapter.
    let device = GraphicsDeviceHandle::create(None)?;

    let texture = GraphicsDeviceTextureDescriptor::new(extent);
    // The library renders on its own thread.
    let attach_options = RenderSessionAttachOptions::new(RenderDriverKind::CoreWorker);

    let (session, attached) =
        map.attach_graphics_device_texture(&device, &texture, &attach_options)?;
    attached.await?;
    // #endregion attach

    // #region image
    map.discarding().update_camera(&CameraUpdate {
        mode: CameraUpdateMode::Jump,
        camera: CameraOptions {
            center: Some(tile_center(z, x, y)),
            zoom: Some(f64::from(z)),
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

    // #region close
    drop(session);
    drop(device);
    map.release().map_err(|(_, error)| error)?.await?;
    // #endregion close

    Ok(())
}

fn main() {}
