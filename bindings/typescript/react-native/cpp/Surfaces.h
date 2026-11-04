#pragma once

#include <cstdint>
#include <memory>

#include <jsi/jsi.h>

namespace mln::react_native {

// A native surface that a view lends to JavaScript.
class Surface {
 public:
  virtual ~Surface() = default;
  // The handles that JavaScript passes to the binding. Runs on the JavaScript
  // thread each time JavaScript acquires the surface.
  virtual facebook::jsi::Object describe(facebook::jsi::Runtime& runtime) = 0;
};

// The registry that connects native views to JavaScript. A view adds its
// surface when the platform provides one, and withdraws it when the platform
// takes it away. JavaScript acquires a surface by its ID and releases it once
// the session stops using it. The registry frees a surface once both sides are
// done with it, so the surface outlives a session that still renders to it.
namespace surfaces {

// Any thread. Returns the surface's ID, which the view reports to JavaScript.
int32_t add(std::shared_ptr<Surface> surface);
// Any thread.
void withdraw(int32_t id);
// Installs `__mlnReactNativeSurfaces` with `acquire(id)` and `release(id)`.
void install(facebook::jsi::Runtime& runtime);
// Releases every surface that JavaScript holds, when its runtime goes away.
void releaseAll();

}  // namespace surfaces

}  // namespace mln::react_native
