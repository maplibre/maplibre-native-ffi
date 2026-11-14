# Snippets for the docs site's Render to a texture page. They are type-checked
# with the tests, which never run them.


import maplibre_native_ffi as mln

session: mln.RenderSessionHandle


# The frame wake from the Render to a window page.
def on_frame_wake() -> None:
    pass


# The host's draw, which samples the image view.
def draw_map(image_view: int) -> None:
    pass


def destroy_textures(
    images: tuple[int, int], views: tuple[int, int]
) -> None:
    pass


def draw_frame() -> None:
    # #region acquire
    frame = session.acquire_frame()
    if frame is None:
        return  # No new frame yet.

    # Sample the texture, and wait for those reads to finish.
    with frame.vulkan_texture() as texture:
        draw_map(texture.image_view)

    frame.release(mln.GpuSync(kind=mln.GpuSyncKind.CPU_COMPLETE))
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
    # #region attach-options
    options = mln.RenderSessionAttachOptions(
        driver=mln.RenderDriverKind.CORE_WORKER,
        frame_wake=mln.Wake(on_frame_wake),
    )
    # #endregion attach-options

    # #region attach-borrowed
    # Your VkDevice and graphics queue.
    device = mln.GpuDeviceHandle.wrap_vulkan(
        mln.VulkanDeviceDescriptor(
            context=vulkan,
            # Only if your renderer also submits to the graphics
            # queue. The lock and unlock are native function pointers.
            queue_lock=mln.QueueLock.native(
                lock=lock_queue, unlock=unlock_queue
            ),
            # The device extensions that you enabled on your VkDevice.
            enabled_extensions=[
                "VK_KHR_swapchain",
                "VK_KHR_external_semaphore_fd",
            ],
        )
    )

    ring = mln.RenderTargetVulkanBorrowedTexturesVariant(
        mln.VulkanBorrowedTexturesTarget(
            # The map's size in logical pixels.
            logical_size=mln.LogicalSize(width=800, height=600),
            # The size of your textures in device pixels.
            physical_width=1600,
            physical_height=1200,
            # Your VkImage and VkImageView for each texture of the ring.
            textures=[
                mln.VulkanBorrowedTexture(
                    image=images[0], image_view=views[0]
                ),
                mln.VulkanBorrowedTexture(
                    image=images[1], image_view=views[1]
                ),
            ],
            format=37,  # VK_FORMAT_R8G8B8A8_UNORM
            final_layout=5,  # VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        )
    )

    session = map_handle.attach(ring, device, options)
    # The session keeps the device until it detaches.
    device.release()
    session.ready.result()
    # #endregion attach-borrowed

    # #region resize-borrowed
    resized = mln.RenderTargetVulkanBorrowedTexturesVariant(
        mln.VulkanBorrowedTexturesTarget(
            logical_size=mln.LogicalSize(width=1024, height=768),
            physical_width=2048,
            physical_height=1536,
            # Your new textures, at the new size.
            textures=[
                mln.VulkanBorrowedTexture(
                    image=new_images[0], image_view=new_views[0]
                ),
                mln.VulkanBorrowedTexture(
                    image=new_images[1], image_view=new_views[1]
                ),
            ],
            format=37,
            final_layout=5,
        )
    )

    session.set_target(resized).result()
    # vkDestroyImageView and vkDestroyImage
    destroy_textures(images, views)
    # #endregion resize-borrowed
