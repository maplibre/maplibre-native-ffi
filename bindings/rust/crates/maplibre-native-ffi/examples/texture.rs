//! Snippets for the docs site's Render to a texture page. They compile with
//! the tests, which never run them.

// Only the stand-ins compile for targets without Vulkan.
#![allow(dead_code, unused_imports)]

use maplibre_native_ffi::*;

// The frame wake from the Render to a window page.
fn on_frame_wake() {}

// Stand-ins for the host's mutex on its graphics queue.
fn lock_queue() {}

fn unlock_queue() {}

// The host's draw, which samples the image view.
fn draw_map(_image_view: u64) {}

fn destroy_textures(_images: [u64; 2], _views: [u64; 2]) {}

#[cfg(any(
    target_os = "android",
    target_os = "freebsd",
    target_os = "linux",
    target_os = "macos",
    target_os = "windows"
))]
fn draw_frame(session: &RenderSessionHandle) -> Result<()> {
    // #region acquire
    let Some(frame) = session.acquire_frame()? else {
        return Ok(()); // No new frame yet.
    };

    // Sample the texture, and wait for those reads to finish.
    draw_map(frame.view()?.get_vulkan_texture()?.image_view);

    frame.release(None).map_err(|(_, error)| error)?;
    // #endregion acquire

    Ok(())
}

#[cfg(any(
    target_os = "android",
    target_os = "freebsd",
    target_os = "linux",
    target_os = "macos",
    target_os = "windows"
))]
async fn render_to_texture(
    map: &MapHandle,
    vulkan: VulkanContextDescriptor,
    images: [u64; 2],
    views: [u64; 2],
    new_images: [u64; 2],
    new_views: [u64; 2],
) -> Result<()> {
    // #region attach-owned
    let texture = VulkanOwnedTextureDescriptor {
        // The texture's size in logical pixels, and its scale factor.
        extent: LogicalExtent::new(800, 600, 2.0),
        // Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
        context: vulkan,
    };
    let options = RenderSessionAttachOptions {
        frame_wake: Wake::new(on_frame_wake),
        // Only if your renderer also submits to the graphics queue.
        queue_lock: QueueLock::new(lock_queue).with_unlock(unlock_queue),
        ..RenderSessionAttachOptions::new(RenderDriverKind::CoreWorker)
    };

    // SAFETY: the Vulkan objects outlive the session.
    let (session, attached) = unsafe { map.attach_vulkan_owned_texture(&texture, &options)? };
    attached.await?;
    // #endregion attach-owned

    draw_frame(&session)?;
    session.release().map_err(|(_, error)| error)?.await?;

    // #region attach-borrowed
    let ring = VulkanBorrowedTextureDescriptor {
        extent: LogicalExtent::new(800, 600, 2.0),
        // The size of your textures in device pixels.
        physical_width: 1600,
        physical_height: 1200,
        context: vulkan,
        // Your VkImage and VkImageView for each texture of the ring.
        textures: vec![
            VulkanBorrowedTexture::new(images[0], views[0]),
            VulkanBorrowedTexture::new(images[1], views[1]),
        ],
        format: 37,      // VK_FORMAT_R8G8B8A8_UNORM
        final_layout: 5, // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        owner: VulkanImageOwner::GraphicsQueue,
    };

    // The same attach options as for an owned ring.
    // SAFETY: the textures and the Vulkan objects outlive the session.
    let (session, attached) = unsafe { map.attach_vulkan_borrowed_texture(&ring, &options)? };
    attached.await?;
    // #endregion attach-borrowed

    // #region resize-borrowed
    let ring = VulkanBorrowedTextureDescriptor {
        extent: LogicalExtent::new(1024, 768, 2.0),
        physical_width: 2048,
        physical_height: 1536,
        // Your new textures, at the new size.
        textures: vec![
            VulkanBorrowedTexture::new(new_images[0], new_views[0]),
            VulkanBorrowedTexture::new(new_images[1], new_views[1]),
        ],
        ..ring
    };

    // SAFETY: the new textures outlive the session.
    unsafe { session.set_vulkan_borrowed_texture_target(&ring)? }.await?;
    destroy_textures(images, views); // vkDestroyImageView and vkDestroyImage
    // #endregion resize-borrowed

    Ok(())
}

fn main() {}
