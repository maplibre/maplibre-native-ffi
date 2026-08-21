#pragma once

namespace mln::core {

// Backend swap() skips host present while an internal render is in progress.
inline thread_local bool discard_renderable_present = false;

}  // namespace mln::core
