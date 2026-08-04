// Lets a Rust browser module run its entry point on a pthread.
//
// Emscripten's -sPROXY_TO_PTHREAD moves main off the thread that hosts the
// module, which is what leaves that thread servicing JavaScript. That thread is
// the one emscripten starts new pthread workers from, recycles finished ones
// on, and lets the browser HTTP transport start on, so a module whose main
// blocks instead -- libtest is synchronous, so a test suite's does -- loses all
// three.
//
// The option links a crt that calls __main_argc_argv, the name clang gives a C
// `main`. rustc emits a plain `main`, and a Rust definition of the missing name
// lands in an rlib with hidden visibility where emcc cannot export it, so the
// bridge is this C translation unit: an asm label names the Rust symbol without
// declaring a function called `main`, which clang would rename in turn.
#include <emscripten.h>

extern int mln_rust_main(int argc, char** argv) __asm__("main");

int __main_argc_argv(int argc, char** argv) {
  const int status = mln_rust_main(argc, argv);

  // Forced, because a suite that has reported its result has nothing left to
  // wait for. An ordinary exit is refused while any runtime keepalive is
  // outstanding, and a fixture can leave one behind through no fault of its
  // own: emdawnwebgpu takes a keepalive per device and returns it when
  // device.lost settles, which needs the JavaScript job queue this thread stops
  // serving the moment main returns. The module would then never exit and the
  // runner would report a timeout instead of the status libtest just produced.
  emscripten_force_exit(status);
  return status;
}
