// A widget that shows a continuous map. The session renders into a ring of
// dmabufs that the library allocates, in formats that GTK can import. The
// widget shows each frame as a GdkDmabufTexture in a GtkGraphicsOffload, so the
// compositor can scan the buffer out. Nothing is copied, and the session needs
// no GL context of GTK's. The widget's frame clock paces the frames. Everything
// else matches the browser example in examples/browser.

using MaplibreNativeFfi;

namespace Example {
	const string STYLE_URL = "https://tiles.openfreemap.org/styles/liberty";

	// The DRM formats that the library renders.
	const uint32 DRM_FORMAT_ABGR8888 = 0x34324241;
	const uint32 DRM_FORMAT_ARGB8888 = 0x34325241;

	[CCode (cheader_filename = "dma-buf.h", cname = "example_dma_buf_dup")]
	extern int dma_buf_dup (int fd);

	[CCode (cheader_filename = "dma-buf.h", cname = "example_dma_buf_export_sync_file")]
	extern int dma_buf_export_sync_file (int fd);

	CameraOptions initial_camera () {
		var camera = new CameraOptions ();
		camera.set_center ({ latitude: 48.8584, longitude: 2.2945 });
		camera.set_zoom (15.5);
		camera.set_bearing (-30);
		camera.set_pitch (55);
		return camera;
	}

	// An acquired frame that a texture shows. The texture reads the buffer
	// through duplicates of the frame's file descriptors, which stay valid
	// after the frame goes back to the session.
	class HeldFrame : Object {
		public AcquiredFrame? frame;
		public int[] fds = {};
		// Whether the session takes a sync file when the frame goes back.
		public bool sync_file;
		// Whether GTK let go of the texture.
		public bool finished;

		public HeldFrame (AcquiredFrame frame, bool sync_file) {
			this.frame = frame;
			this.sync_file = sync_file;
		}

		// Gives the frame back to the session. The session renders into its
		// buffer again only once the buffer's readers that the kernel tracks,
		// such as the compositor's GPU, are done with it.
		public void release () {
			if (frame == null) {
				return;
			}
			GpuSync sync = { kind: GpuSyncKind.CPU_COMPLETE };
			var file = sync_file && fds.length > 0 ? dma_buf_export_sync_file (fds[0]) : -1;
			if (file >= 0) {
				sync = { kind: GpuSyncKind.SYNC_FD, handle: file };
			}
			try {
				frame.release (sync);
			} catch (GLib.Error error) {
				// The session takes the sync file only when the release
				// succeeds. A session that failed or went away has no use for
				// the frame, which the object discards when it goes.
				if (file >= 0) {
					Posix.close (file);
				}
			}
			frame = null;
		}

		public void close_fds () {
			foreach (var fd in fds) {
				Posix.close (fd);
			}
			fds = {};
		}
	}

	public class MapView : Gtk.Widget {
		// The map's progress, or an error once it fails, which later
		// progress doesn't replace. An empty message clears the status.
		public signal void status_changed (string message, bool error);

		private Gtk.GraphicsOffload? offload;
		private Gtk.Picture picture;
		private Gestures gestures;

		// What GTK reports.
		private bool started;
		private Gdk.Display? display;
		private Gdk.Surface? surface;
		private ulong scale_handler;
		private LogicalSize? size;
		private double surface_scale;

		// What the map and session have.
		private Runtime? runtime;
		private Map? maplibre_map;
		private RenderSession? session;
		private uint session_number;
		private double map_scale_factor;
		private LogicalSize session_size;
		private bool sync_file;
		private CameraOptions camera = initial_camera ();
		private bool failed;

		// The frames that textures still show.
		private GenericArray<HeldFrame> held = new GenericArray<HeldFrame> ();

		// Frame pacing.
		private uint tick_id;
		private bool redraw_pending;
		private bool force_pending;
		private int demands_in_flight;
		private bool resizing;
		private SourceFunc? demands_settled;

		// Changes to the map and session run one at a time, in order.
		private bool changing;
		private bool change_queued;
		private SourceFunc? stopped;

		construct {
			focusable = true;
			picture = new Gtk.Picture ();
			picture.content_fit = Gtk.ContentFit.FILL;
			picture.can_shrink = true;
			offload = new Gtk.GraphicsOffload (picture);
			offload.set_parent (this);
			gestures = new Gestures (this);
			gestures.delta.connect ((delta) => {
				if (maplibre_map != null) {
					maplibre_map.apply_camera_delta_async.begin (delta);
				}
			});
		}

		public override void dispose () {
			if (offload != null) {
				offload.unparent ();
				offload = null;
			}
			base.dispose ();
		}

		public void start () {
			started = true;
			update ();
		}

		// Releases the session, the map, and the runtime.
		public async void stop () {
			started = false;
			update ();
			if (changing) {
				stopped = stop.callback;
				yield;
			}
		}

		// The map takes its size from the window, not the other way round.
		public override void measure (
			Gtk.Orientation orientation,
			int for_size,
			out int minimum,
			out int natural,
			out int minimum_baseline,
			out int natural_baseline
		) {
			minimum = natural = 0;
			minimum_baseline = natural_baseline = -1;
		}

		public override void size_allocate (int width, int height, int baseline) {
			int minimum, natural, minimum_baseline, natural_baseline;
			offload.measure (
				Gtk.Orientation.HORIZONTAL, -1,
				out minimum, out natural, out minimum_baseline, out natural_baseline
			);
			// An OpenGL build of the library renders the rows of its dmabufs
			// from the bottom up, so the widget shows them flipped.
			Gsk.Transform? transform = null;
			if (RenderBackendFlag.OPEN_GL in supported_render_backend_mask ()) {
				transform = new Gsk.Transform ()
					.translate (Graphene.Point () { x = 0, y = height })
					.scale (1, -1);
			}
			offload.allocate (width, height, baseline, transform);
			size = LogicalSize () {
				width = (uint32) int.max (1, width),
				height = (uint32) int.max (1, height),
			};
			update ();
		}

		public override void realize () {
			base.realize ();
			display = get_display ();
			surface = get_native ().get_surface ();
			surface_scale = surface.get_scale ();
			scale_handler = surface.notify["scale"].connect (() => {
				surface_scale = surface.get_scale ();
				update ();
			});
			update ();
		}

		// The session's buffers suit the display that the widget leaves, so
		// the widget releases the session, and keeps the map and its tiles
		// for when it shows again.
		public override void unrealize () {
			surface.disconnect (scale_handler);
			surface = null;
			display = null;
			stop_ticking ();
			update ();
			base.unrealize ();
		}

		// While the widget is hidden, it keeps the session and the frame it
		// shows, and demands no frames.
		public override void map () {
			base.map ();
			if (redraw_pending) {
				schedule_tick ();
			}
		}

		public override void unmap () {
			stop_ticking ();
			base.unmap ();
		}

		// Brings the map and session in line with what GTK last reported.
		private void update () {
			if (changing) {
				change_queued = true;
				return;
			}
			changing = true;
			run_changes.begin ();
		}

		private async void run_changes () {
			do {
				change_queued = false;
				try {
					yield apply_changes ();
				} catch (GLib.Error error) {
					fail (error.message);
				}
			} while (change_queued);
			changing = false;
			if (stopped != null) {
				Idle.add ((owned) stopped);
			}
		}

		private async void apply_changes () throws GLib.Error {
			if (!started) {
				yield release_session ();
				yield release_map ();
				var retired = runtime;
				runtime = null;
				if (retired != null) {
					yield retired.release_async ();
				}
				return;
			}
			// Without a display there is nothing to import frames into. This
			// runs even after a failure, since the session can still render.
			if (display == null || size == null) {
				yield release_session ();
				return;
			}
			if (failed) {
				return;
			}

			// A map renders at one scale factor for its life, so a move to a
			// display of another density takes a new map and session. The new
			// map keeps the camera.
			if (maplibre_map != null && map_scale_factor != surface_scale) {
				camera = maplibre_map.get_snapshot ().view.camera.to_options ();
				yield release_session ();
				yield release_map ();
			}

			if (maplibre_map == null) {
				yield create_map (size, surface_scale);
				return;
			}
			if (session == null) {
				yield attach (size);
			} else if (session_size.width != size.width || session_size.height != size.height) {
				yield resize_session (size);
			}
		}

		private async void create_map (LogicalSize size, double scale) throws GLib.Error {
			if (runtime == null) {
				start_runtime ();
			}
			var options = new MapOptions ();
			options.set_initial_extent (
				{ width: size.width, height: size.height, scale_factor: scale }
			);
			options.set_map_mode (MapMode.CONTINUOUS);
			options.set_event_mask (
				RuntimeEventFlag.MAP_STYLE_LOADED
				| RuntimeEventFlag.MAP_LOADING_FAILED
				| RuntimeEventFlag.MAP_RENDER_ERROR
			);
			maplibre_map = runtime.create_map (options);
			map_scale_factor = scale;
			yield maplibre_map.ready_async ();
			var update = new CameraUpdate ();
			update.set_mode (CameraUpdateMode.JUMP);
			update.set_camera (camera);
			maplibre_map.update_camera_async.begin (update);
			maplibre_map.set_style_url_async.begin (STYLE_URL);
			show_status ("Loading the style…");
			// The next pass attaches a session at the size GTK reports then.
			change_queued = true;
		}

		private void start_runtime () throws GLib.Error {
			runtime = new Runtime.create (null);
			runtime.set_event_handler_async.begin (event_handler (this));
		}

		// A Vala closure in an instance method holds the instance, so the
		// callbacks that the library keeps are made in static methods, where
		// a weak reference is all they hold.
		private static RuntimeEventHandler event_handler (MapView view) {
			var self = WeakRef (view);
			var handler = new RuntimeEventHandler ();
			// The handler runs on this thread's main context.
			handler.set_callback ((events) => {
				var target = self.get () as MapView;
				target?.handle_events (events);
			});
			return handler;
		}

		private void handle_events (GenericArray<RuntimeEvent> events) {
			foreach (var event in events) {
				if (maplibre_map == null || !maplibre_map.is_event_source (event)) {
					continue;
				}
				switch (event.get_event_type ()) {
				case RuntimeEventType.MAP_STYLE_LOADED:
					show_status ("");
					break;
				case RuntimeEventType.MAP_LOADING_FAILED:
				case RuntimeEventType.MAP_RENDER_ERROR:
					fail (event.get_message ());
					break;
				default:
					break;
				}
			}
		}

		// Attaches a session that renders into a ring of dmabufs in formats
		// that this display imports.
		private async void attach (LogicalSize size) throws GLib.Error {
			var descriptor = new DmaBufRingDescriptor ();
			descriptor.set_logical_size (size);
			// The session renders into one buffer while GTK holds two: the
			// frame it shows, and the one the compositor hasn't let go of yet.
			descriptor.set_buffer_count (3);
			descriptor.set_formats (dma_buf_formats (display));

			var number = ++session_number;
			var options = new RenderSessionAttachOptions ();
			options.set_driver (RenderDriverKind.CORE_WORKER);
			// GdkDmabufTextureBuilder takes no sync file, so each frame
			// finishes rendering before the session hands it over.
			options.set_producer_sync_kinds (GpuSyncKindFlag.CPU_COMPLETE);
			options.set_redraw_wake (session_wake (this, number, false));
			options.set_frame_wake (session_wake (this, number, true));

			session = maplibre_map.attach_dma_buf_ring (descriptor, null, options);
			session_size = size;
			yield session.ready_async ();
			var capabilities = session.get_capabilities ();
			sync_file = GpuSyncKindFlag.SYNC_FD in capabilities.consumer_sync_kinds;

			// The first frame draws whether or not the map changed.
			force_pending = true;
			on_redraw_wake ();
		}

		private static DmaBufFormat[] dma_buf_formats (Gdk.Display display) {
			var formats = display.get_dmabuf_formats ();
			DmaBufFormat[] result = {};
			for (size_t index = 0; index < formats.get_n_formats (); index++) {
				uint32 fourcc;
				uint64 modifier;
				formats.get_format (index, out fourcc, out modifier);
				if (fourcc == DRM_FORMAT_ABGR8888 || fourcc == DRM_FORMAT_ARGB8888) {
					result += DmaBufFormat () { fourcc = fourcc, modifier = modifier };
				}
			}
			return result;
		}

		// The wakes run on this thread's main context. A wake of a session
		// that the widget released finds no session to serve.
		private static Wake session_wake (MapView view, uint number, bool frame) {
			var self = WeakRef (view);
			var wake = new Wake ();
			wake.set_callback (() => {
				var target = self.get () as MapView;
				if (target == null || target.session_number != number) {
					return;
				}
				if (frame) {
					target.on_frame_wake ();
				} else {
					target.on_redraw_wake ();
				}
			});
			return wake;
		}

		// The session resizes only while it lends no frames. GTK may show a
		// frame until the compositor lets go of it, so the frames go back
		// before then, once the session renders nothing into the ring. The
		// resize gives the ring new buffers, and the textures keep showing
		// the old ones until a frame of the new size replaces them.
		private async void resize_session (LogicalSize size) throws GLib.Error {
			resizing = true;
			stop_ticking ();
			try {
				while (demands_in_flight > 0) {
					demands_settled = resize_session.callback;
					yield;
				}
				release_held_frames ();
				yield session.resize_async (size);
				session_size = size;
			} finally {
				resizing = false;
			}
			force_pending = true;
			on_redraw_wake ();
		}

		private async void release_session () throws GLib.Error {
			if (session == null) {
				return;
			}
			var retired = session;
			session = null;
			session_number++;
			stop_ticking ();
			redraw_pending = false;
			demands_in_flight = 0;
			release_held_frames ();
			yield retired.release_async ();
		}

		private async void release_map () throws GLib.Error {
			var retired = maplibre_map;
			maplibre_map = null;
			if (retired != null) {
				yield retired.release_async ();
			}
		}

		private void release_held_frames () {
			foreach (var frame in held) {
				frame.release ();
			}
			forget_finished_frames ();
		}

		private void forget_finished_frames () {
			for (var index = held.length; index > 0; index--) {
				if (held[index - 1].finished) {
					held.remove_index_fast (index - 1);
				}
			}
		}

		// The session has something new to draw. The next tick of the
		// widget's frame clock demands it.
		private void on_redraw_wake () {
			redraw_pending = true;
			schedule_tick ();
		}

		private void schedule_tick () {
			if (tick_id == 0 && get_mapped () && !resizing && session != null) {
				tick_id = add_tick_callback (on_tick);
			}
		}

		private void stop_ticking () {
			if (tick_id != 0) {
				remove_tick_callback (tick_id);
				tick_id = 0;
			}
		}

		private bool on_tick (Gtk.Widget widget, Gdk.FrameClock clock) {
			tick_id = 0;
			if (session == null || resizing || !get_mapped ()) {
				return Source.REMOVE;
			}
			var flags = force_pending ? FrameDemandFlag.FORCE : (FrameDemandFlag) 0;
			redraw_pending = false;
			force_pending = false;
			try {
				session.demand_frame ({ flags: flags });
				demands_in_flight++;
			} catch (GLib.Error error) {
				fail (error.message);
			}
			return Source.REMOVE;
		}

		// Frame results are waiting. Shows the newest frame, if the session
		// rendered one.
		private void on_frame_wake () {
			try {
				var results = session.drain_frame_results ();
				demands_in_flight -= (int) results.length;
				foreach (unowned var result in results) {
					if (result.disposition == RenderResult.FAILED) {
						fail (session.get_failure ());
					}
				}
				if (demands_in_flight <= 0 && demands_settled != null) {
					Idle.add ((owned) demands_settled);
					demands_settled = null;
				}
				if (!resizing && get_mapped ()) {
					show_newest_frame ();
				}
			} catch (GLib.Error error) {
				fail (error.message);
			}
		}

		private void show_newest_frame () throws GLib.Error {
			var frame = session.acquire_frame ();
			if (frame == null) {
				return;
			}
			var held_frame = new HeldFrame (frame, sync_file);
			try {
				picture.paintable = wrap_frame (held_frame);
			} catch (GLib.Error error) {
				held_frame.release ();
				held_frame.close_fds ();
				throw error;
			}
		}

		// Wraps the frame's dmabuf in a texture, which gives the frame back
		// when GTK lets go of it.
		private Gdk.Texture wrap_frame (HeldFrame held_frame) throws GLib.Error {
			var dma_buf = held_frame.frame.get_dma_buf ();
			var builder = new Gdk.DmabufTextureBuilder ();
			builder.display = display;
			builder.width = dma_buf.get_width ();
			builder.height = dma_buf.get_height ();
			builder.fourcc = dma_buf.get_fourcc ();
			builder.modifier = dma_buf.get_modifier ();
			unowned var planes = dma_buf.get_planes ();
			builder.n_planes = planes.length;
			for (uint index = 0; index < planes.length; index++) {
				var fd = dma_buf_dup (planes[index].fd);
				if (fd < 0) {
					throw new IOError.FAILED ("%s", strerror (errno));
				}
				held_frame.fds += fd;
				builder.set_fd (index, fd);
				builder.set_offset (index, planes[index].offset);
				builder.set_stride (index, planes[index].stride);
			}
			held_frame.ref ();
			Gdk.Texture texture;
			try {
				texture = builder.build (on_texture_destroyed, held_frame);
			} catch (GLib.Error error) {
				held_frame.unref ();
				throw error;
			}
			forget_finished_frames ();
			held.add (held_frame);
			return texture;
		}

		// GTK lets go of a texture once nothing shows it: the compositor has
		// released the buffer of an offloaded texture, or GTK has drawn a
		// frame without it. GPU work that still reads the buffer may be
		// running, which the frame's release waits for.
		private static void on_texture_destroyed (void* data) {
			var held_frame = (HeldFrame) data;
			held_frame.unref ();
			MainContext.@default ().invoke (() => {
				held_frame.release ();
				held_frame.close_fds ();
				held_frame.finished = true;
				return Source.REMOVE;
			});
		}

		private void show_status (string message) {
			if (!failed) {
				status_changed (message, false);
			}
		}

		private void fail (string message) {
			printerr ("Map error: %s\n", message);
			if (!failed) {
				failed = true;
				status_changed (message, true);
			}
		}
	}
}
