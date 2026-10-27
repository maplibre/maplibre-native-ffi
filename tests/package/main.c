#include <maplibre_native_c.h>

int main(void) {
  mln_runtime runtime = {0};
  const mln_runtime_options options = {.size = sizeof(mln_runtime_options)};
  if (mln_runtime_create(&options, &runtime, NULL) != MLN_STATUS_OK) return 1;
  return mln_runtime_dispose(runtime, NULL) != MLN_STATUS_OK;
}
