// The entry point that puts Kotlin on a thread it may block.
//
// -sPROXY_TO_PTHREAD runs main() on a pthread rather than on the agent that
// instantiated the module, so this thread parks in mln_runtime_pump() while the
// host agent keeps its event loop. The keepalive is what stops the thread from
// exiting once main() returns, as crt1_proxy_main.c does for a proxied main.
// mln_kotlin_boot_module() imports the Kotlin/Wasm module into this thread's
// realm, which is what makes every C call Kotlin issues a same-thread call.

#include <emscripten/eventloop.h>

void mln_kotlin_boot_module(void);

int main(void) {
  emscripten_runtime_keepalive_push();
  mln_kotlin_boot_module();
  return 0;
}
