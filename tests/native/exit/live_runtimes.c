// A process that exits with two runtimes live, each with a map loading a
// style, and with every callback it installed still installed: the log
// callback, each runtime's event wake, and each runtime's resource provider.
// See probe.h.

#include "probe.h"

int main(void) {
  probe_start();
  for (int index = 0; index < 2; index += 1) {
    (void)probe_create_map(probe_create_runtime());
  }
  return 0;
}
