// Snippets for the docs site's Render to a texture page. They compile with
// the tests, which never run them.

using MaplibreNativeFfi;

namespace Texture {
	RenderSession session;

	// The frame wake from the Render to a window page.
	void on_frame_wake () {
	}

	// The host's draw, which samples the image view.
	void draw_map (uint64 image_view) {
	}

	void destroy_textures (uint64[] images, uint64[] views) {
	}

	public void draw_frame () throws GLib.Error {
		// #region acquire
		var frame = session.acquire_frame ();
		if (frame == null) {
			return; // No new frame yet.
		}

		// Sample the texture, and wait for those reads to finish.
		var texture = frame.get_vulkan_texture ();
		draw_map (texture.get_image_view ());

		frame.release ({ kind: GpuSyncKind.CPU_COMPLETE });
		// #endregion acquire
	}

	public async void render_to_texture (
		Map map,
		VulkanContextDescriptor vulkan,
		uint64[] images,
		uint64[] views,
		uint64[] new_images,
		uint64[] new_views
	) throws GLib.Error {
		// #region attach-options
		var frame_wake = new Wake ();
		frame_wake.set_callback (on_frame_wake);
		var options = new RenderSessionAttachOptions ();
		options.set_driver (RenderDriverKind.CORE_WORKER);
		options.set_frame_wake (frame_wake);
		// A queue lock, which your renderer needs only if it also
		// submits to the graphics queue, takes native callbacks that
		// only C code can set.
		// #endregion attach-options

		// #region attach-borrowed
		var ring = new VulkanBorrowedTextureDescriptor ();
		// The map's size in logical pixels.
		ring.set_logical_size ({ width: 800, height: 600 });
		// The size of your textures in device pixels.
		ring.set_physical_width (1600);
		ring.set_physical_height (1200);
		ring.set_context (vulkan);
		// Your VkImage and VkImageView for each texture of the ring.
		ring.set_textures ({
			{ image: images[0], image_view: views[0] },
			{ image: images[1], image_view: views[1] },
		});
		ring.set_format (37); // VK_FORMAT_R8G8B8A8_UNORM
		// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		ring.set_final_layout (5);

		session = map.attach_vulkan_borrowed_texture (ring, options);
		yield session.ready_async ();
		// #endregion attach-borrowed

		// #region resize-borrowed
		var resized = new VulkanBorrowedTextureReplacement ();
		resized.set_logical_size ({ width: 1024, height: 768 });
		resized.set_physical_width (2048);
		resized.set_physical_height (1536);
		// Your new textures, at the new size.
		resized.set_textures ({
			{ image: new_images[0], image_view: new_views[0] },
			{ image: new_images[1], image_view: new_views[1] },
		});

		yield session.set_vulkan_borrowed_texture_target_async (resized);
		// vkDestroyImageView and vkDestroyImage
		destroy_textures (images, views);
		// #endregion resize-borrowed
	}
}
