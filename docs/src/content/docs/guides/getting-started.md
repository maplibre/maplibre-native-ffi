---
title: Getting started
description: Render a map and read its pixels.
sidebar:
  order: 1
# Scope: one short program, start to finish, that renders a map and reads its
# pixels, without a window. Each step names the choice it makes and links the
# page that covers the alternatives.
---

This guide renders a map into a texture and reads its pixels. It uses no window,
so it runs on every platform.

## Create a runtime

Create a runtime with the default options. To set a cache file, see
[Network](/maplibre-native-ffi/guides/network/).

```c title="getting-started.c"
mln_runtime_options runtime_options = {.size = sizeof(mln_runtime_options)};
mln_runtime runtime = MLN_HANDLE_NULL;
mln_runtime_create(&runtime_options, &runtime, NULL);
```

## Create a map

Create a map in static mode. Make it the size of the image you want. A static
map renders one image for each request. Wait for the map to be created. For the
other modes, see [Rendering](/maplibre-native-ffi/guides/rendering/).

```c title="getting-started.c"
mln_map_options map_options = {
  .size = sizeof(mln_map_options),
  .initial_extent = {.width = 512, .height = 512, .scale_factor = 1.0},
  .map_mode = MLN_MAP_MODE_STATIC,
};

mln_map map = MLN_HANDLE_NULL;
mln_runtime_create_map(runtime, &map_options, &map, &done, NULL);
```

## Load a style

Set the style URL. This guide uses the MapLibre demo tiles style. You don't need
to wait for the style to load. The image request waits for it. To load style
JSON or change the style, see [Style](/maplibre-native-ffi/guides/style/).

```c title="getting-started.c"
mln_map_set_style_url(
  map,
  mln_buffer_view_from_cstr("https://demotiles.maplibre.org/style.json"),
  NULL, NULL
);
```

## Attach a texture

Create a graphics device. The library creates the device for its graphics
backend, so you write no graphics code. Attach a texture on the device to the
map. Make the texture the same size as the map. The attach call returns a render
session. The session draws the map into the texture. Wait for the attach to
finish. To render to a window, or to a texture you own, see
[Rendering](/maplibre-native-ffi/guides/rendering/).

```c title="getting-started.c"
// NULL options select the default adapter.
mln_graphics_device device = MLN_HANDLE_NULL;
mln_graphics_device_create(NULL, &device, NULL);

mln_graphics_device_texture_descriptor texture = {
  .size = sizeof(mln_graphics_device_texture_descriptor),
  .extent = map_options.initial_extent,
};
mln_render_session_attach_options attach_options = {
  .size = sizeof(mln_render_session_attach_options),
  // The library renders on its own thread.
  .driver = MLN_RENDER_DRIVER_CORE_WORKER,
};

mln_render_session session = MLN_HANDLE_NULL;
mln_map_attach_graphics_device_texture(
  map, device, &texture, &attach_options, &session, &done, NULL
);
```

## Render an image

Request a still image. Wait for the request to finish.

```c title="getting-started.c"
mln_map_request_still_image(map, &done, NULL);
```

## Read the pixels

Read the texture's pixels. The result has RGBA pixels, row by row. The stride is
the number of bytes in each row.

```c title="getting-started.c"
const mln_completion read = {
  .size = sizeof(mln_completion), .callback = on_pixels
};

mln_render_session_read_texture(session, &read, NULL);
```

```c title="getting-started.c"
static void on_pixels(void* user_data, const mln_completion_result* result) {
  if (result->status != MLN_STATUS_OK) return;
  const mln_texture_readback_result* readback = result->value;
  // Copy readback->data here. It's valid only during the callback.
}
```

## Close

Close the render session, the graphics device, the map, and the runtime, in that
order. Wait for the map and the runtime to close.

```c title="getting-started.c"
mln_render_session_dispose(session, NULL, NULL);
mln_graphics_device_release(device);
mln_map_release(map, &done, NULL);
mln_runtime_release(runtime, &done, NULL);
```
