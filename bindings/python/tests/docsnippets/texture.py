# Snippets for the docs site's Render to a texture page. They are type-checked
# with the tests, which never run them.

import dataclasses

import maplibre_native_ffi as mln

session: mln.RenderSessionHandle


# The frame wake from the Render to a window page.
def on_frame_wake() -> None:
    pass


# The host's draw, which samples the image view.
def draw_map(image_view: int) -> None:
    pass


def destroy_textures(images: tuple[int, int], views: tuple[int, int]) -> None:
    pass


def draw_frame() -> None:
    # #region acquire
    frame = session.acquire_frame()
    if frame is None:
        return  # No new frame yet.

    # Sample the texture, and wait for those reads to finish.
    with frame.vulkan_texture() as texture:
        draw_map(texture.image_view)

    frame.close()
    # #endregion acquire


def render_to_texture(
    map_handle: mln.MapHandle,
    vulkan: mln.VulkanContextDescriptor,
    lock_queue: int,
    unlock_queue: int,
    images: tuple[int, int],
    views: tuple[int, int],
    new_images: tuple[int, int],
    new_views: tuple[int, int],
) -> None:
    global session
    # #region attach-owned
    texture = mln.VulkanOwnedTextureDescriptor(
        # The texture's size in logical pixels, and its scale factor.
        extent=mln.LogicalExtent(width=800, height=600, scale_factor=2.0),
        # Your VkInstance, VkPhysicalDevice, VkDevice, and graphics VkQueue.
        context=vulkan,
    )
    options = mln.RenderSessionAttachOptions(
        driver=mln.RenderDriverKind.CORE_WORKER,
        frame_wake=mln.Wake(on_frame_wake),
        # Only if your renderer also submits to the graphics queue. The lock and
        # unlock are native function pointers.
        queue_lock=mln.QueueLock.native(lock=lock_queue, unlock=unlock_queue),
    )

    session, ready = map_handle.attach_vulkan_owned_texture(texture, options)
    ready.result()
    # #endregion attach-owned

    draw_frame()
    session.close().result()

    # #region attach-borrowed
    ring = mln.VulkanBorrowedTextureDescriptor(
        extent=mln.LogicalExtent(width=800, height=600, scale_factor=2.0),
        # The size of your textures in device pixels.
        physical_width=1600,
        physical_height=1200,
        context=vulkan,
        # Your VkImage and VkImageView for each texture of the ring.
        textures=[
            mln.VulkanBorrowedTexture(image=images[0], image_view=views[0]),
            mln.VulkanBorrowedTexture(image=images[1], image_view=views[1]),
        ],
        format=37,  # VK_FORMAT_R8G8B8A8_UNORM
        final_layout=5,  # VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    )

    # The same attach options as for an owned ring. A queue lock serves one
    # attach, so pass a new one.
    session, ready = map_handle.attach_vulkan_borrowed_texture(
        ring,
        dataclasses.replace(
            options,
            queue_lock=mln.QueueLock.native(lock=lock_queue, unlock=unlock_queue),
        ),
    )
    ready.result()
    # #endregion attach-borrowed

    # #region resize-borrowed
    ring = dataclasses.replace(
        ring,
        extent=mln.LogicalExtent(width=1024, height=768, scale_factor=2.0),
        physical_width=2048,
        physical_height=1536,
        # Your new textures, at the new size.
        textures=[
            mln.VulkanBorrowedTexture(image=new_images[0], image_view=new_views[0]),
            mln.VulkanBorrowedTexture(image=new_images[1], image_view=new_views[1]),
        ],
    )

    session.set_vulkan_borrowed_texture_target(ring).result()
    destroy_textures(images, views)  # vkDestroyImageView and vkDestroyImage
    # #endregion resize-borrowed
