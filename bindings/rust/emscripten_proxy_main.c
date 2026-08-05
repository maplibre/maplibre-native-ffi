// Bridges Rust's entry point to Emscripten's pthread proxy CRT.
#include <emscripten.h>

extern int mln_rust_main(int argc, char** argv) __asm__("main");

int __main_argc_argv(int argc, char** argv) {
  const int status = mln_rust_main(argc, argv);

  // Do not let backend keepalives hide libtest's completed status.
  emscripten_force_exit(status);
  return status;
}
