#include "c_api/autorelease_pool.hpp"

namespace mln::c_api {

auto run_in_autorelease_pool(void* context, mln_status (*body)(void*))
  -> mln_status {
  @autoreleasepool {
    return body(context);
  }
}

}  // namespace mln::c_api
