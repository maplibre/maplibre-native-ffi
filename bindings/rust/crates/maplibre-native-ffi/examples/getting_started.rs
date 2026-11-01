//! Snippets for the docs site's Getting started page. They compile with the
//! tests, which never run them.

use std::future::Future;
use std::pin::pin;
use std::sync::Arc;
use std::task::{Context, Poll, Wake, Waker};
use std::thread::{self, Thread};

use maplibre_native_ffi::*;

fn copy_pixels(_data: &[u8], _stride: u32) {}

async fn getting_started() -> Result<()> {
    // #region runtime
    let runtime = RuntimeHandle::create(&RuntimeOptions::default())?;
    // #endregion runtime

    // #region map
    let extent = LogicalExtent::new(512, 512, 1.0);
    let map_options = MapOptions {
        map_mode: MapMode::Static,
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

    // #region render
    map.request_still_image()?.await?;
    // #endregion render

    // #region render
    let image = session.read_texture()?.await?;
    // #endregion render

    // #region render
    copy_pixels(&image.data, image.info.stride);
    // #endregion render

    // #region close
    drop(session);
    drop(device);
    map.release().map_err(|(_, error)| error)?.await?;
    runtime.release().map_err(|(_, error)| error)?.await?;
    // #endregion close

    Ok(())
}

struct Unpark(Thread);

impl Wake for Unpark {
    fn wake(self: Arc<Self>) {
        self.0.unpark();
    }
}

fn block_on<F: Future>(future: F) -> F::Output {
    let waker = Waker::from(Arc::new(Unpark(thread::current())));
    let mut context = Context::from_waker(&waker);
    let mut future = pin!(future);
    loop {
        if let Poll::Ready(output) = future.as_mut().poll(&mut context) {
            return output;
        }
        thread::park();
    }
}

fn main() -> Result<()> {
    block_on(getting_started())
}
