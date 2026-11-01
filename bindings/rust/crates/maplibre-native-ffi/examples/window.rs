//! Snippets for the docs site's Render to a window page. They compile with the
//! tests, which never run them.

#![allow(dead_code)]

use maplibre_native_ffi::*;

// Stand-ins for the host's event loop, display timer, and graphics thread,
// which run each task with the window's state.
type Task = fn(&mut MapWindow) -> Result<()>;

fn run_on_event_loop(_task: Task) {}

fn run_at_next_refresh(_task: Task) {}

fn run_on_graphics_thread(_task: Task) {}

fn destroy_surface() {}

impl MapWindow {
    fn replace_session(&mut self) {}
}

// #region attach
struct MapWindow {
    session: RenderSessionHandle,
    // Set while the window has no surface, and when it closes.
    paused: bool,
}
// #endregion attach

// #region frame-loop
impl MapWindow {
    fn request_frame(&mut self) -> Result<()> {
        self.session.request_frame(&FrameDemand {
            flags: FrameDemandFlag::PRESENT | FrameDemandFlag::WAIT_FOR_UPDATE,
            ..Default::default()
        })
    }

    fn drain_frame_results(&mut self) -> Result<()> {
        let results = self.session.drain_frame_results()?;
        if results.is_empty() {
            return Ok(());
        }
        if results
            .iter()
            .any(|result| result.disposition == RenderResult::Failed)
        {
            self.replace_session(); // Close the session and attach a new one.
        } else if !self.paused {
            run_at_next_refresh(MapWindow::request_frame);
        }
        Ok(())
    }
}
// #endregion frame-loop

// #region frame-wake
// Runs on a library thread.
fn on_frame_wake() {
    run_on_event_loop(MapWindow::drain_frame_results);
}
// #endregion frame-wake

// #region resize
impl MapWindow {
    fn on_window_resized(&mut self, width: u32, height: u32) -> Result<()> {
        // The scale factor the session has now.
        let extent = LogicalExtent::new(width, height, 2.0);
        self.session.discarding().resize(extent)
    }
}
// #endregion resize

// #region service
impl MapWindow {
    // Your graphics context is current on this thread.
    fn service_session(&mut self) -> Result<()> {
        self.session.service_driver_work()
    }
}

// Runs on a library thread.
fn on_driver_work_wake() {
    run_on_graphics_thread(MapWindow::service_session);
}
// #endregion service

async fn surface_lost(window: &mut MapWindow) -> Result<()> {
    // #region lost-surface
    window.paused = true;
    window.session.release_target()?.await?;
    // #endregion lost-surface

    Ok(())
}

#[cfg(any(
    target_os = "android",
    target_os = "freebsd",
    target_os = "linux",
    target_os = "macos",
    target_os = "windows"
))]
async fn surface_restored(
    window: &mut MapWindow,
    vulkan: VulkanContextDescriptor,
    new_surface: u64,
) -> Result<()> {
    // #region lost-surface
    let extent = LogicalExtent::new(800, 600, 2.0);
    let surface = VulkanSurfaceDescriptor::new(extent, vulkan, new_surface);
    // SAFETY: the surface and the Vulkan objects outlive the session.
    unsafe { window.session.set_vulkan_surface_target(&surface)? }.await?;

    window.paused = false;
    window.request_frame()?;
    // #endregion lost-surface

    Ok(())
}

#[cfg(any(
    target_os = "android",
    target_os = "freebsd",
    target_os = "linux",
    target_os = "macos",
    target_os = "windows"
))]
async fn render_to_window(
    map: &MapHandle,
    vulkan: VulkanContextDescriptor,
    window_surface: u64,
) -> Result<()> {
    // #region attach
    let surface = VulkanSurfaceDescriptor {
        // The window's size in logical pixels, and its scale factor.
        extent: LogicalExtent::new(800, 600, 2.0),
        // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
        context: vulkan,
        // Your VkSurfaceKHR.
        surface: window_surface,
    };
    let options = RenderSessionAttachOptions {
        frame_wake: Wake::new(on_frame_wake),
        ..RenderSessionAttachOptions::new(RenderDriverKind::CoreWorker)
    };

    // SAFETY: the surface and the Vulkan objects outlive the session.
    let (session, attached) = unsafe { map.attach_vulkan_surface(&surface, &options)? };
    attached.await?;

    let mut window = MapWindow {
        session,
        paused: false,
    };
    // #endregion attach

    window.request_frame()?;
    window.on_window_resized(1024, 768)?;

    // #region service
    let graphics_thread_options = RenderSessionAttachOptions {
        frame_wake: Wake::new(on_frame_wake),
        driver_work_wake: Wake::new(on_driver_work_wake),
        ..RenderSessionAttachOptions::new(RenderDriverKind::CallerGraphicsThread)
    };
    // #endregion service
    drop(graphics_thread_options);

    // #region close
    window.paused = true;
    window
        .session
        .release()
        .map_err(|(_, error)| error)?
        .await?;
    destroy_surface(); // For example, with vkDestroySurfaceKHR.
    // #endregion close

    Ok(())
}

fn main() {}
