// The entry point that puts Kotlin on a thread it may block.
//
// -sPROXY_TO_PTHREAD runs main() on a pthread rather than on the agent that
// instantiated the module, so this thread parks in mln_runtime_pump() while the
// host agent keeps its event loop. The keepalive is what stops the thread from
// exiting once main() returns, as crt1_proxy_main.c does for a proxied main.
// mln_kotlin_boot_module() imports the Kotlin/Wasm module into this thread's
// realm, which is what makes every C call Kotlin issues a same-thread call.

#include <emscripten.h>
#include <emscripten/eventloop.h>

#include "mln_kotlin.h"

void mln_kotlin_boot_module(void);

int main(void) {
  emscripten_runtime_keepalive_push();
  mln_kotlin_boot_module();
  return 0;
}

EMSCRIPTEN_KEEPALIVE void mln_kotlin_exit(int status) {
  // The keepalive that leaves this thread running is what a host has to drop to
  // end the program, and a backend keepalive can outlive it, so the exit is
  // forced rather than waited for. Runners read the status; a host that never
  // calls this keeps the thread parked, which is what a map wants.
  emscripten_runtime_keepalive_pop();
  emscripten_force_exit(status);
}
