# GTK example

A GTK 4 app in Vala that shows a continuous map with the GObject library, on
Linux. The library renders each frame into a dmabuf, which GTK shows without a
copy.

## Run it

From the repository root, build the native library, the GObject library, and the
app, and run it:

```bash
mise run //examples/gtk:run
```

The task builds this machine's Vulkan preset. Pass another Linux preset, such as
`linux-gnu-x64-egl`, to run an OpenGL build. The app needs GTK 4.14 or later,
and Vala, from the system's packages.

## How it fits together

The app builds with meson against the prefix that `//bindings/gobject:build`
installs into: the GObject library's pkg-config file, and the VAPI that vapigen
generates from its introspection data. An app outside this repository builds the
same way against an installed library.

`src/map-view.vala` is a widget that holds the map. It attaches a render session
to a ring of three dmabufs that the library allocates, with
`attach_dma_buf_ring`, in the formats that `gdk_display_get_dmabuf_formats()`
reports. The session renders on a library thread, in the EGL and Vulkan builds
alike, and needs no GL context of GTK's. A redraw wake asks for a frame, and the
widget's frame clock demands it at the next tick. A frame wake drains the frame
results and acquires the newest frame, which the widget wraps in a texture with
`GdkDmabufTextureBuilder` and shows in a `GtkPicture` inside a
`GtkGraphicsOffload`. When nothing clips the map, as in a maximized window, the
compositor shows the buffer itself.

`GdkDmabufTextureBuilder` takes no sync file, so the session attaches without
`SYNC_FD` in its producer sync kinds, and each frame finishes rendering before
the session hands it over. The texture keeps the frame until GTK lets go of it:
once the compositor releases the buffer, or GTK draws without it. Then the
widget gives the frame back with a sync file from
`DMA_BUF_IOCTL_EXPORT_SYNC_FILE`, so the session renders into the buffer again
only after the GPU reads that the kernel tracks on it, such as the compositor's,
finish.

The session resizes only while it lends no frames, so a resize takes the frames
back early, once no demand is pending, and the textures keep showing the old
buffers through duplicated file descriptors until a frame of the new size
arrives. While the widget is hidden, it keeps its session and demands no frames.
When it leaves its window, it releases the session and keeps the map, and
attaches a new session when it shows again. A move to a display of another scale
takes a new map, as the map renders at one scale factor for its life. An OpenGL
build renders the rows of each buffer from the bottom up, so the widget shows
them flipped. `src/gestures.vala` turns pointer, touch, and key input into
camera changes. Everything else matches the browser example in
`examples/browser`.
