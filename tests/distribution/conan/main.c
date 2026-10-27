#include <maplibre_native_c.h>
#include <stdio.h>

int main(void) {
  printf("C API version: %u\n", mln_c_version());
  return mln_supported_render_backend_mask() == 0;
}
