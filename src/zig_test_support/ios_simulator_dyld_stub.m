#include <TargetConditionals.h>

struct mach_header;

// Zig's test runtime references this dyld helper, which the iOS and tvOS SDK
// link environments do not provide. The tests never inspect image headers.
const struct mach_header* _dyld_get_image_header_containing_address(
  const void* address
) {
  (void)address;
  return 0;
}
