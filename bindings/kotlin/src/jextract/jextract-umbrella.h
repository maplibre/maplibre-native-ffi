// jextract parses one root header. plugin.h stays outside maplibre_native_c.h
// for native consumers, so the JVM parse roots at this umbrella.
#include <maplibre_native_c.h>
#include <maplibre_native_c/plugin.h>
