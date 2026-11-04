#define _POSIX_C_SOURCE 200809L

#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "wayland_compositor.h"

#include "unity.h"
#include "wait.h"

// libwayland's types stay opaque, since the suite builds without its headers.
// The opcodes are the core protocol's, from wayland.xml.
struct wl_client;
struct wl_display;
struct wl_event_loop;
struct wl_event_source;
struct wl_global;
struct wl_proxy;
struct wl_resource;
struct wl_interface;
struct wl_shm_buffer;

typedef void (*bind_function)(
  struct wl_client* client, void* data, uint32_t version, uint32_t id
);
typedef void (*resource_destroy_function)(struct wl_resource* resource);
typedef int (*fd_function)(int fd, uint32_t mask, void* data);

enum {
  // WL_EVENT_READABLE
  event_readable = 0x01,
  // WL_MARSHAL_FLAG_DESTROY
  marshal_destroy = 1U << 0U,
  // The newest wl_compositor whose requests the compositor implements.
  compositor_version = 4,
  surface_capacity = 8,
  callback_capacity = 16,
};

enum {
  display_get_registry = 1,
  registry_bind = 0,
  compositor_create_surface = 0,
  surface_destroy = 0,
  callback_done = 0,
  buffer_release = 0,
};

// WL_SHM_FORMAT_ARGB8888. The other format that wl_shm always offers is
// XRGB8888.
enum { shm_format_argb8888 = 0 };

typedef struct server_api {
  struct wl_display* (*display_create)(void);
  void (*display_destroy)(struct wl_display*);
  struct wl_event_loop* (*display_get_event_loop)(struct wl_display*);
  void (*display_flush_clients)(struct wl_display*);
  int (*display_init_shm)(struct wl_display*);
  int (*event_loop_dispatch)(struct wl_event_loop*, int);
  struct wl_event_source* (*event_loop_add_fd)(
    struct wl_event_loop*, int, uint32_t, fd_function, void*
  );
  struct wl_client* (*client_create)(struct wl_display*, int);
  struct wl_global* (*global_create)(
    struct wl_display*, const struct wl_interface*, int, void*, bind_function
  );
  struct wl_resource* (*resource_create)(
    struct wl_client*, const struct wl_interface*, int, uint32_t
  );
  void (*resource_set_implementation)(
    struct wl_resource*, const void*, void*, resource_destroy_function
  );
  void* (*resource_get_user_data)(struct wl_resource*);
  int (*resource_get_version)(struct wl_resource*);
  void (*resource_post_event)(struct wl_resource*, uint32_t, ...);
  void (*resource_destroy)(struct wl_resource*);
  struct wl_shm_buffer* (*shm_buffer_get)(struct wl_resource*);
  void* (*shm_buffer_get_data)(struct wl_shm_buffer*);
  int32_t (*shm_buffer_get_stride)(struct wl_shm_buffer*);
  uint32_t (*shm_buffer_get_format)(struct wl_shm_buffer*);
  int32_t (*shm_buffer_get_width)(struct wl_shm_buffer*);
  int32_t (*shm_buffer_get_height)(struct wl_shm_buffer*);
  void (*shm_buffer_begin_access)(struct wl_shm_buffer*);
  void (*shm_buffer_end_access)(struct wl_shm_buffer*);
  const struct wl_interface* compositor_interface;
  const struct wl_interface* surface_interface;
  const struct wl_interface* region_interface;
  const struct wl_interface* callback_interface;
} server_api;

typedef struct client_api {
  struct wl_display* (*display_connect_to_fd)(int);
  void (*display_disconnect)(struct wl_display*);
  int (*display_roundtrip)(struct wl_display*);
  struct wl_proxy* (*proxy_marshal_flags)(
    struct wl_proxy*, uint32_t, const struct wl_interface*, uint32_t, uint32_t,
    ...
  );
  uint32_t (*proxy_get_version)(struct wl_proxy*);
  uint32_t (*proxy_get_id)(struct wl_proxy*);
  int (*proxy_add_listener)(struct wl_proxy*, void (**)(void), void*);
  void (*proxy_destroy)(struct wl_proxy*);
  const struct wl_interface* registry_interface;
  const struct wl_interface* compositor_interface;
  const struct wl_interface* surface_interface;
} client_api;

// A surface as the compositor holds it. The case's thread reads it under the
// compositor's mutex.
typedef struct surface_record {
  bool used;
  // The surface's object ID, which the client's proxy shares.
  uint32_t id;
  struct wl_resource* attached;
  bool has_attached;
  struct wl_resource* callbacks[callback_capacity];
  int callback_count;
  // Whether the compositor withholds the surface's frame callbacks, as one
  // does for a surface that isn't shown.
  bool hidden;
  uint64_t committed;
  uint64_t seen;
  mln_test_wayland_frame frame;
} surface_record;

struct mln_test_wayland {
  server_api server;
  client_api client;
  struct wl_display* server_display;
  struct wl_display* client_display;
  struct wl_proxy* compositor;
  uint32_t compositor_name;
  pthread_t thread;
  bool thread_started;
  atomic_bool stopping;
  // A byte on stop_pipe wakes the compositor's thread to stop.
  int stop_pipe[2];
  pthread_mutex_t mutex;
  surface_record surfaces[surface_capacity];
};

// libwayland-server runs a request's handler without saying which compositor
// the resource belongs to, so the handlers reach the one that runs through
// this.
static mln_test_wayland* running;

#define LOAD(library, table, member, symbol)                 \
  do {                                                       \
    *(void**)(&(table).member) = dlsym((library), (symbol)); \
    if ((table).member == NULL) {                            \
      TEST_FAIL_MESSAGE("libwayland lacks " symbol);         \
    }                                                        \
  } while (0)

// The graphics drivers keep both libraries loaded, so neither closes.
static void load_server(server_api* api) {
  void* library = dlopen("libwayland-server.so.0", RTLD_NOW | RTLD_LOCAL);
  TEST_ASSERT_NOT_NULL_MESSAGE(library, "libwayland-server.so.0 is missing");
  LOAD(library, *api, display_create, "wl_display_create");
  LOAD(library, *api, display_destroy, "wl_display_destroy");
  LOAD(library, *api, display_get_event_loop, "wl_display_get_event_loop");
  LOAD(library, *api, display_flush_clients, "wl_display_flush_clients");
  LOAD(library, *api, display_init_shm, "wl_display_init_shm");
  LOAD(library, *api, event_loop_dispatch, "wl_event_loop_dispatch");
  LOAD(library, *api, event_loop_add_fd, "wl_event_loop_add_fd");
  LOAD(library, *api, client_create, "wl_client_create");
  LOAD(library, *api, global_create, "wl_global_create");
  LOAD(library, *api, resource_create, "wl_resource_create");
  LOAD(
    library, *api, resource_set_implementation, "wl_resource_set_implementation"
  );
  LOAD(library, *api, resource_get_user_data, "wl_resource_get_user_data");
  LOAD(library, *api, resource_get_version, "wl_resource_get_version");
  LOAD(library, *api, resource_post_event, "wl_resource_post_event");
  LOAD(library, *api, resource_destroy, "wl_resource_destroy");
  LOAD(library, *api, shm_buffer_get, "wl_shm_buffer_get");
  LOAD(library, *api, shm_buffer_get_data, "wl_shm_buffer_get_data");
  LOAD(library, *api, shm_buffer_get_stride, "wl_shm_buffer_get_stride");
  LOAD(library, *api, shm_buffer_get_format, "wl_shm_buffer_get_format");
  LOAD(library, *api, shm_buffer_get_width, "wl_shm_buffer_get_width");
  LOAD(library, *api, shm_buffer_get_height, "wl_shm_buffer_get_height");
  LOAD(library, *api, shm_buffer_begin_access, "wl_shm_buffer_begin_access");
  LOAD(library, *api, shm_buffer_end_access, "wl_shm_buffer_end_access");
  LOAD(library, *api, compositor_interface, "wl_compositor_interface");
  LOAD(library, *api, surface_interface, "wl_surface_interface");
  LOAD(library, *api, region_interface, "wl_region_interface");
  LOAD(library, *api, callback_interface, "wl_callback_interface");
}

static void load_client(client_api* api) {
  void* library = dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_LOCAL);
  TEST_ASSERT_NOT_NULL_MESSAGE(library, "libwayland-client.so.0 is missing");
  LOAD(library, *api, display_connect_to_fd, "wl_display_connect_to_fd");
  LOAD(library, *api, display_disconnect, "wl_display_disconnect");
  LOAD(library, *api, display_roundtrip, "wl_display_roundtrip");
  LOAD(library, *api, proxy_marshal_flags, "wl_proxy_marshal_flags");
  LOAD(library, *api, proxy_get_version, "wl_proxy_get_version");
  LOAD(library, *api, proxy_get_id, "wl_proxy_get_id");
  LOAD(library, *api, proxy_add_listener, "wl_proxy_add_listener");
  LOAD(library, *api, proxy_destroy, "wl_proxy_destroy");
  LOAD(library, *api, registry_interface, "wl_registry_interface");
  LOAD(library, *api, compositor_interface, "wl_compositor_interface");
  LOAD(library, *api, surface_interface, "wl_surface_interface");
}

#undef LOAD

// The compositor's side, which runs on its own thread, where Unity can't fail
// the case.

static void give_up(const char* message) {
  (void)fprintf(stderr, "%s\n", message);
  abort();
}

static void ignore_rectangle(
  struct wl_client* client, struct wl_resource* resource, int32_t x, int32_t y,
  int32_t width, int32_t height
) {
  (void)client;
  (void)resource;
  (void)x;
  (void)y;
  (void)width;
  (void)height;
}

static void ignore_region(
  struct wl_client* client, struct wl_resource* resource,
  struct wl_resource* region
) {
  (void)client;
  (void)resource;
  (void)region;
}

static void ignore_int(
  struct wl_client* client, struct wl_resource* resource, int32_t value
) {
  (void)client;
  (void)resource;
  (void)value;
}

static void destroy_resource(
  struct wl_client* client, struct wl_resource* resource
) {
  (void)client;
  running->server.resource_destroy(resource);
}

static surface_record* resource_surface(struct wl_resource* resource) {
  return running->server.resource_get_user_data(resource);
}

static void surface_attach(
  struct wl_client* client, struct wl_resource* resource,
  struct wl_resource* buffer, int32_t x, int32_t y
) {
  (void)client;
  (void)x;
  (void)y;
  surface_record* surface = resource_surface(resource);
  surface->attached = buffer;
  surface->has_attached = true;
}

static bool surface_hidden(const surface_record* surface) {
  pthread_mutex_lock(&running->mutex);
  const bool hidden = surface->hidden;
  pthread_mutex_unlock(&running->mutex);
  return hidden;
}

static void surface_frame(
  struct wl_client* client, struct wl_resource* resource, uint32_t id
) {
  surface_record* surface = resource_surface(resource);
  const server_api* api = &running->server;
  struct wl_resource* callback =
    api->resource_create(client, api->callback_interface, 1, id);
  if (callback == NULL) give_up("the test compositor ran out of memory");
  api->resource_set_implementation(callback, NULL, NULL, NULL);
  if (surface->callback_count < callback_capacity) {
    surface->callbacks[surface->callback_count++] = callback;
  } else if (!surface_hidden(surface)) {
    give_up("the test compositor ran out of frame callbacks");
  }
  // A hidden surface's callbacks past the capacity are never answered either.
}

// Reads a wl_shm buffer's size and center pixel. Its formats describe 32-bit
// little-endian words.
static mln_test_wayland_frame read_shm_frame(struct wl_shm_buffer* shm) {
  const server_api* api = &running->server;
  mln_test_wayland_frame frame = {
    .width = api->shm_buffer_get_width(shm),
    .height = api->shm_buffer_get_height(shm),
  };
  const size_t offset =
    (size_t)(frame.height / 2) * (size_t)api->shm_buffer_get_stride(shm) +
    (size_t)(frame.width / 2) * sizeof(uint32_t);
  uint32_t word = 0;
  api->shm_buffer_begin_access(shm);
  memcpy(
    &word, (const uint8_t*)api->shm_buffer_get_data(shm) + offset, sizeof(word)
  );
  api->shm_buffer_end_access(shm);
  frame.center[0] = (uint8_t)(word >> 16U);
  frame.center[1] = (uint8_t)(word >> 8U);
  frame.center[2] = (uint8_t)word;
  frame.center[3] = api->shm_buffer_get_format(shm) == shm_format_argb8888
                      ? (uint8_t)(word >> 24U)
                      : 0xff;
  return frame;
}

// Keeps the committed buffer's frame, and answers the buffer at once. Unless
// the surface is hidden, it answers the frame callbacks at once too, as a
// compositor that shows each frame right away would.
static void surface_commit(
  struct wl_client* client, struct wl_resource* resource
) {
  (void)client;
  surface_record* surface = resource_surface(resource);
  const server_api* api = &running->server;
  struct wl_resource* buffer = surface->has_attached ? surface->attached : NULL;
  surface->attached = NULL;
  surface->has_attached = false;
  struct wl_shm_buffer* shm =
    buffer == NULL ? NULL : api->shm_buffer_get(buffer);
  if (shm != NULL) {
    const mln_test_wayland_frame frame = read_shm_frame(shm);
    pthread_mutex_lock(&running->mutex);
    surface->frame = frame;
    ++surface->committed;
    pthread_mutex_unlock(&running->mutex);
    mln_test_pulse();
  }
  if (buffer != NULL) api->resource_post_event(buffer, buffer_release);
  if (surface_hidden(surface)) return;
  for (int index = 0; index < surface->callback_count; ++index) {
    api->resource_post_event(surface->callbacks[index], callback_done, 0U);
    api->resource_destroy(surface->callbacks[index]);
  }
  surface->callback_count = 0;
}

static void surface_destroyed(struct wl_resource* resource) {
  surface_record* surface = resource_surface(resource);
  pthread_mutex_lock(&running->mutex);
  *surface = (surface_record){0};
  pthread_mutex_unlock(&running->mutex);
}

// wl_surface's requests through version 4, in opcode order.
static const struct {
  void (*destroy)(struct wl_client*, struct wl_resource*);
  void (*attach)(
    struct wl_client*, struct wl_resource*, struct wl_resource*, int32_t,
    int32_t
  );
  void (*damage)(
    struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t
  );
  void (*frame)(struct wl_client*, struct wl_resource*, uint32_t);
  void (*set_opaque_region)(
    struct wl_client*, struct wl_resource*, struct wl_resource*
  );
  void (*set_input_region)(
    struct wl_client*, struct wl_resource*, struct wl_resource*
  );
  void (*commit)(struct wl_client*, struct wl_resource*);
  void (*set_buffer_transform)(struct wl_client*, struct wl_resource*, int32_t);
  void (*set_buffer_scale)(struct wl_client*, struct wl_resource*, int32_t);
  void (*damage_buffer)(
    struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t
  );
} surface_requests = {
  .destroy = destroy_resource,
  .attach = surface_attach,
  .damage = ignore_rectangle,
  .frame = surface_frame,
  .set_opaque_region = ignore_region,
  .set_input_region = ignore_region,
  .commit = surface_commit,
  .set_buffer_transform = ignore_int,
  .set_buffer_scale = ignore_int,
  .damage_buffer = ignore_rectangle,
};

// wl_region's requests, in opcode order.
static const struct {
  void (*destroy)(struct wl_client*, struct wl_resource*);
  void (*add)(
    struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t
  );
  void (*subtract)(
    struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t
  );
} region_requests = {
  .destroy = destroy_resource,
  .add = ignore_rectangle,
  .subtract = ignore_rectangle,
};

static void create_surface(
  struct wl_client* client, struct wl_resource* resource, uint32_t id
) {
  const server_api* api = &running->server;
  struct wl_resource* created = api->resource_create(
    client, api->surface_interface, api->resource_get_version(resource), id
  );
  surface_record* surface = NULL;
  pthread_mutex_lock(&running->mutex);
  for (int index = 0; index < surface_capacity; ++index) {
    if (!running->surfaces[index].used) {
      surface = &running->surfaces[index];
      *surface = (surface_record){.used = true, .id = id};
      break;
    }
  }
  pthread_mutex_unlock(&running->mutex);
  if (created == NULL || surface == NULL) {
    give_up("the test compositor ran out of surfaces");
  }
  api->resource_set_implementation(
    created, &surface_requests, surface, surface_destroyed
  );
}

static void create_region(
  struct wl_client* client, struct wl_resource* resource, uint32_t id
) {
  const server_api* api = &running->server;
  struct wl_resource* created = api->resource_create(
    client, api->region_interface, api->resource_get_version(resource), id
  );
  if (created == NULL) {
    give_up("the test compositor ran out of memory");
  }
  api->resource_set_implementation(created, &region_requests, NULL, NULL);
}

// wl_compositor's requests, in opcode order.
static const struct {
  void (*create_surface)(struct wl_client*, struct wl_resource*, uint32_t);
  void (*create_region)(struct wl_client*, struct wl_resource*, uint32_t);
} compositor_requests = {
  .create_surface = create_surface,
  .create_region = create_region,
};

static void bind_compositor(
  struct wl_client* client, void* data, uint32_t version, uint32_t id
) {
  (void)data;
  const server_api* api = &running->server;
  struct wl_resource* resource =
    api->resource_create(client, api->compositor_interface, (int)version, id);
  if (resource == NULL) {
    give_up("the test compositor ran out of memory");
  }
  api->resource_set_implementation(resource, &compositor_requests, NULL, NULL);
}

static int on_stop(int fd, uint32_t mask, void* data) {
  (void)fd;
  (void)mask;
  (void)data;
  return 0;
}

static void* serve(void* context) {
  mln_test_wayland* wayland = context;
  struct wl_event_loop* loop =
    wayland->server.display_get_event_loop(wayland->server_display);
  while (!atomic_load(&wayland->stopping)) {
    wayland->server.event_loop_dispatch(loop, -1);
    wayland->server.display_flush_clients(wayland->server_display);
  }
  return NULL;
}

// The client's side, which runs on the case's thread.

static void on_global(
  void* data, struct wl_proxy* registry, uint32_t name, const char* interface,
  uint32_t version
) {
  (void)registry;
  (void)version;
  mln_test_wayland* wayland = data;
  if (strcmp(interface, "wl_compositor") == 0) wayland->compositor_name = name;
}

static void on_global_remove(
  void* data, struct wl_proxy* registry, uint32_t name
) {
  (void)data;
  (void)registry;
  (void)name;
}

static const struct {
  void (*global)(void*, struct wl_proxy*, uint32_t, const char*, uint32_t);
  void (*global_remove)(void*, struct wl_proxy*, uint32_t);
} registry_events = {.global = on_global, .global_remove = on_global_remove};

static void connect_client(mln_test_wayland* wayland, int fd) {
  const client_api* api = &wayland->client;
  wayland->client_display = api->display_connect_to_fd(fd);
  if (wayland->client_display == NULL) {
    close(fd);
    TEST_FAIL_MESSAGE("the client could not connect to the test compositor");
  }
  struct wl_proxy* display = (struct wl_proxy*)wayland->client_display;
  struct wl_proxy* registry = api->proxy_marshal_flags(
    display, display_get_registry, api->registry_interface,
    api->proxy_get_version(display), 0, NULL
  );
  TEST_ASSERT_NOT_NULL(registry);
  // libwayland-client takes the listener as an array of function pointers.
  // NOLINTNEXTLINE(bugprone-casting-through-void)
  api->proxy_add_listener(registry, (void (**)(void))&registry_events, wayland);
  const int roundtrip = api->display_roundtrip(wayland->client_display);
  if (roundtrip >= 0 && wayland->compositor_name != 0) {
    wayland->compositor = api->proxy_marshal_flags(
      registry, registry_bind, api->compositor_interface, compositor_version, 0,
      wayland->compositor_name, "wl_compositor", (uint32_t)compositor_version,
      NULL
    );
  }
  api->proxy_destroy(registry);
  TEST_ASSERT_NOT_NULL_MESSAGE(
    wayland->compositor, "the client could not bind wl_compositor"
  );
}

static void start(mln_test_wayland* wayland) {
  load_server(&wayland->server);
  load_client(&wayland->client);
  const server_api* api = &wayland->server;
  wayland->server_display = api->display_create();
  TEST_ASSERT_NOT_NULL(wayland->server_display);
  TEST_ASSERT_EQUAL_INT(0, api->display_init_shm(wayland->server_display));
  TEST_ASSERT_NOT_NULL(api->global_create(
    wayland->server_display, api->compositor_interface, compositor_version,
    NULL, bind_compositor
  ));
  TEST_ASSERT_EQUAL_INT(0, pipe(wayland->stop_pipe));
  TEST_ASSERT_NOT_NULL(api->event_loop_add_fd(
    api->display_get_event_loop(wayland->server_display), wayland->stop_pipe[0],
    event_readable, on_stop, NULL
  ));
  int fds[2] = {-1, -1};
  TEST_ASSERT_EQUAL_INT(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
  // The compositor owns fds[0] from here, even when it fails.
  if (api->client_create(wayland->server_display, fds[0]) == NULL) {
    close(fds[1]);
    TEST_FAIL_MESSAGE("the test compositor could not accept its client");
  }
  TEST_ASSERT_EQUAL_INT(
    0, pthread_create(&wayland->thread, NULL, serve, wayland)
  );
  wayland->thread_started = true;
  connect_client(wayland, fds[1]);
}

void mln_test_wayland_start(mln_test_wayland** out_wayland) {
  TEST_ASSERT_NULL_MESSAGE(running, "a test compositor is already running");
  mln_test_wayland* wayland = calloc(1, sizeof(*wayland));
  TEST_ASSERT_NOT_NULL(wayland);
  wayland->stop_pipe[0] = -1;
  wayland->stop_pipe[1] = -1;
  pthread_mutex_init(&wayland->mutex, NULL);
  running = wayland;
  *out_wayland = wayland;
  start(wayland);
}

void mln_test_wayland_stop(mln_test_wayland* wayland) {
  if (wayland == NULL) return;
  if (wayland->compositor != NULL) {
    wayland->client.proxy_destroy(wayland->compositor);
  }
  if (wayland->client_display != NULL) {
    wayland->client.display_disconnect(wayland->client_display);
  }
  if (wayland->thread_started) {
    atomic_store(&wayland->stopping, true);
    const char wake = 0;
    (void)!write(wayland->stop_pipe[1], &wake, 1);
    pthread_join(wayland->thread, NULL);
  }
  if (wayland->server_display != NULL) {
    wayland->server.display_destroy(wayland->server_display);
  }
  if (wayland->stop_pipe[0] >= 0) close(wayland->stop_pipe[0]);
  if (wayland->stop_pipe[1] >= 0) close(wayland->stop_pipe[1]);
  pthread_mutex_destroy(&wayland->mutex);
  if (running == wayland) running = NULL;
  free(wayland);
}

void* mln_test_wayland_display(const mln_test_wayland* wayland) {
  return wayland->client_display;
}

void* mln_test_wayland_create_surface(mln_test_wayland* wayland) {
  const client_api* api = &wayland->client;
  struct wl_proxy* surface = api->proxy_marshal_flags(
    wayland->compositor, compositor_create_surface, api->surface_interface,
    api->proxy_get_version(wayland->compositor), 0, NULL
  );
  TEST_ASSERT_NOT_NULL(surface);
  // The compositor records the surface before it answers.
  TEST_ASSERT_TRUE(api->display_roundtrip(wayland->client_display) >= 0);
  return surface;
}

void mln_test_wayland_hide_surface(mln_test_wayland* wayland, void* surface) {
  const uint32_t id = wayland->client.proxy_get_id(surface);
  bool found = false;
  pthread_mutex_lock(&wayland->mutex);
  for (int index = 0; index < surface_capacity; ++index) {
    surface_record* record = &wayland->surfaces[index];
    if (record->used && record->id == id) {
      record->hidden = true;
      found = true;
    }
  }
  pthread_mutex_unlock(&wayland->mutex);
  TEST_ASSERT_TRUE(found);
}

void mln_test_wayland_destroy_surface(
  mln_test_wayland* wayland, void* surface
) {
  if (surface == NULL) return;
  const client_api* api = &wayland->client;
  struct wl_proxy* proxy = surface;
  (void)api->proxy_marshal_flags(
    proxy, surface_destroy, NULL, api->proxy_get_version(proxy), marshal_destroy
  );
  (void)api->display_roundtrip(wayland->client_display);
}

typedef struct frame_wait {
  mln_test_wayland* wayland;
  uint32_t id;
  bool committed;
  mln_test_wayland_frame frame;
} frame_wait;

static bool frame_committed(void* context) {
  frame_wait* wait = context;
  mln_test_wayland* wayland = wait->wayland;
  pthread_mutex_lock(&wayland->mutex);
  for (int index = 0; index < surface_capacity; ++index) {
    surface_record* surface = &wayland->surfaces[index];
    if (!surface->used || surface->id != wait->id) continue;
    if (surface->committed > surface->seen) {
      surface->seen = surface->committed;
      wait->frame = surface->frame;
      wait->committed = true;
    }
  }
  pthread_mutex_unlock(&wayland->mutex);
  return wait->committed;
}

bool mln_test_wayland_wait_for_frame(
  mln_test_wayland* wayland, void* surface, mln_test_wayland_frame* out_frame
) {
  frame_wait wait = {
    .wayland = wayland,
    .id = wayland->client.proxy_get_id(surface),
  };
  const bool committed = mln_test_await(
    frame_committed, &wait, mln_test_deadline_default(),
    "a frame on the test compositor"
  );
  if (committed) *out_frame = wait.frame;
  return committed;
}
