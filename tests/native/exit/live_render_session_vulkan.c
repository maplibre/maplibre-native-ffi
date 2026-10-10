// A process that exits after abandoning two Vulkan sessions mid-frame, one on a
// core worker and one on a host graphics thread, with its runtime, maps,
// session handles, resource provider, wakes, and log callback all live. See
// render_probe.h.
//
// Each session shares its queue with the host under a host queue lock, so each
// of its queue calls calls into the host. Abandon ends those calls, so the
// lock must see none after the sessions are abandoned, through the exit.

#include "render_probe.h"

// The host's lock on both sessions' queues, one mutex for both.
typedef struct probe_queue_lock {
#ifdef _WIN32
  CRITICAL_SECTION mutex;
#else
  pthread_mutex_t mutex;
#endif
  // Set under the mutex once both sessions are abandoned.
  bool closed;
  // Written under the mutex by the session's release.
  int releases;
} probe_queue_lock;

static probe_queue_lock queue_lock;

static void probe_queue_lock_take(probe_queue_lock* lock) {
#ifdef _WIN32
  EnterCriticalSection(&lock->mutex);
#else
  pthread_mutex_lock(&lock->mutex);
#endif
}

static void probe_queue_lock_give(probe_queue_lock* lock) {
#ifdef _WIN32
  LeaveCriticalSection(&lock->mutex);
#else
  pthread_mutex_unlock(&lock->mutex);
#endif
}

static void session_locks_queue(void* user_data) {
  probe_queue_lock* lock = user_data;
  probe_queue_lock_take(lock);
  if (lock->closed) {
    (void)fprintf(stderr, "a session took the queue lock after abandon\n");
    _Exit(1);
  }
}

static void session_unlocks_queue(void* user_data) {
  probe_queue_lock_give(user_data);
}

static void session_releases_queue_lock(void* user_data) {
  probe_queue_lock* lock = user_data;
  probe_queue_lock_take(lock);
  lock->releases += 1;
  probe_queue_lock_give(lock);
}

static mln_status attach(
  mln_map map, const mln_test_graphics_context* context,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) {
  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
  descriptor.extent = probe_extent;
  descriptor.context.instance = context->vulkan_instance;
  descriptor.context.physical_device = context->vulkan_physical_device;
  descriptor.context.device = context->vulkan_device;
  descriptor.context.graphics_queue = context->vulkan_queue;
  descriptor.context.graphics_queue_family_index =
    context->vulkan_queue_family_index;
  descriptor.context.get_instance_proc_addr =
    context->vulkan_get_instance_proc_addr;
  descriptor.context.get_device_proc_addr =
    context->vulkan_get_device_proc_addr;
  mln_render_session_attach_options locked = *options;
  locked.queue_lock = (mln_queue_lock){
    .lock = session_locks_queue,
    .unlock = session_unlocks_queue,
    .user_data = &queue_lock,
    .release_user_data = session_releases_queue_lock,
  };
  return mln_map_attach_vulkan_owned_texture(
    map, &descriptor, &locked, out_session, completion, NULL
  );
}

int main(void) {
#ifdef _WIN32
  InitializeCriticalSection(&queue_lock.mutex);
#else
  pthread_mutex_init(&queue_lock.mutex, NULL);
#endif
  const int status =
    probe_exit_after_abandoning(MLN_TEST_GRAPHICS_BACKEND_VULKAN, attach);
  probe_queue_lock_take(&queue_lock);
  queue_lock.closed = true;
  const int releases = queue_lock.releases;
  probe_queue_lock_give(&queue_lock);
  // Abandon lets go of each session's lock once it can no longer be called.
  if (releases != 2) {
    (void)fprintf(stderr, "abandon released %d queue locks\n", releases);
    return 1;
  }
  return status;
}
