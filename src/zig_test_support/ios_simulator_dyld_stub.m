#include <TargetConditionals.h>

#if TARGET_OS_SIMULATOR
struct mach_header;

const struct mach_header* _dyld_get_image_header_containing_address(
  const void* address
) {
  (void)address;
  return 0;
}
#endif
