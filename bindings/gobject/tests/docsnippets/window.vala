// Snippets for the docs site's Render to a window page. They compile with
// the tests, which never run them.

using MaplibreNativeFfi;

namespace Window {
	// Stand-ins for the host's display timer and graphics thread.
	void run_at_next_refresh (owned GLib.SourceFunc task) {
	}

	void run_on_graphics_thread (owned GLib.SourceFunc task) {
	}

	void replace_session () {
	}

	void destroy_surface () {
	}

	// #region attach
	RenderSession session;
	// #endregion attach

	// #region frame-loop
	// Draws the map if it changed since the last frame.
	bool demand_frame () {
		try {
			session.demand_frame ({});
		} catch (GLib.Error error) {
			warning ("%s", error.message);
		}
		return GLib.Source.REMOVE;
	}

	void drain_frame_results () throws GLib.Error {
		foreach (var result in session.drain_frame_results ()) {
			if (result.disposition == RenderResult.FAILED) {
				// Release the session and attach a new one.
				replace_session ();
				return;
			}
		}
	}
	// #endregion frame-loop

	// #region frame-wake
	// Both run on the main context of the thread that attached the
	// session.
	void on_redraw_wake () {
		run_at_next_refresh (demand_frame);
	}

	void on_frame_wake () {
		try {
			drain_frame_results ();
		} catch (GLib.Error error) {
			warning ("%s", error.message);
		}
	}
	// #endregion frame-wake

	// #region resize
	void on_window_resized (uint32 width, uint32 height) {
		// The window's new size in logical pixels.
		session.resize_async.begin ({ width, height });
	}
	// #endregion resize

	// #region service
	// Your graphics context is current on this thread.
	bool service_session () {
		try {
			session.service_driver_work ();
		} catch (GLib.Error error) {
			warning ("%s", error.message);
		}
		return GLib.Source.REMOVE;
	}

	// Runs on the main context of the thread that attached the
	// session.
	void on_driver_work_wake () {
		run_on_graphics_thread (service_session);
	}
	// #endregion service

	public async void surface_lost () throws GLib.Error {
		// #region lost-surface
		yield session.release_target_async ();
		// #endregion lost-surface
	}

	public async void surface_restored (
		uint64 new_surface
	) throws GLib.Error {
		// #region lost-surface
		yield session.set_vulkan_surface_target_async (
			{ logical_size: { width: 800, height: 600 }, surface: new_surface }
		);
		// #endregion lost-surface
	}

	Wake wake (owned WakeCallback callback) {
		var wake = new Wake ();
		wake.set_callback ((owned) callback);
		return wake;
	}

	public async void render_to_window (
		Map map, VulkanContextDescriptor vulkan, uint64 window_surface
	) throws GLib.Error {
		// #region attach
		var descriptor = new VulkanSurfaceDescriptor () {
			// The window's size in logical pixels. The map's scale
			// factor turns it into pixels.
			logical_size = { width: 800, height: 600 },
			// Your VkInstance, VkPhysicalDevice, VkDevice, and graphics
			// VkQueue.
			context = vulkan,
			// Your VkSurfaceKHR.
			surface = window_surface,
		};

		var options = new RenderSessionAttachOptions () {
			driver = RenderDriverKind.CORE_WORKER,
			redraw_wake = wake (on_redraw_wake),
			frame_wake = wake (on_frame_wake),
		};

		session = map.attach_vulkan_surface (descriptor, options);
		yield session.ready_async ();
		// #endregion attach

		on_window_resized (1024, 768);

		// #region service
		var graphics_thread_options = new RenderSessionAttachOptions () {
			driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
			redraw_wake = wake (on_redraw_wake),
			frame_wake = wake (on_frame_wake),
			driver_work_wake = wake (on_driver_work_wake),
		};
		// #endregion service
		assert (graphics_thread_options != null);

		yield session.release_async ();
		destroy_surface (); // For example, with vkDestroySurfaceKHR.
	}
}
