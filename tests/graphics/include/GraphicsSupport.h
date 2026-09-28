#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Driver objects only: Binding tests submit every MapLibre operation through
// the binding.
typedef struct mln_test_graphics {
  void* _Nullable loader;
  bool vulkan;
  void* _Nullable display;
  void* _Nullable config;
  void* _Nullable instance;
  void* _Nullable physical_device;
  void* _Nullable device;
  void* _Nullable queue;
  uint32_t queue_family;
  void* _Nullable get_instance_proc_addr;
  void* _Nullable get_device_proc_addr;
  void* _Nullable context;
  void* _Nullable surface;
} mln_test_graphics;

mln_test_graphics* _Nullable mln_test_graphics_create(bool vulkan);
bool mln_test_graphics_make_current(mln_test_graphics* _Nonnull graphics);
void mln_test_graphics_destroy(mln_test_graphics* _Nullable graphics);

#ifdef __cplusplus
}
#endif
