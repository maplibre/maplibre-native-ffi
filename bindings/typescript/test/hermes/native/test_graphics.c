// Give the test module a separate private registration entry.
#include "symbols.h"
#undef napi_register_module_v1
#define napi_register_module_v1 mln_hermes_test_graphics_init
#include "graphics_addon.c"
