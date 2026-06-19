#include <TargetConditionals.h>

#if TARGET_OS_SIMULATOR
struct mach_header;

// Zig's test runtime references this dyld helper, but the iOS simulator SDK
// link environment used here does not provide it. The tests do not depend on
// image-header lookup behavior, so a null fallback is sufficient.
const struct mach_header* _dyld_get_image_header_containing_address(
  const void* address
) {
  (void)address;
  return 0;
}
#endif
