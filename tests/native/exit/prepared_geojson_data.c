// A process that prepares GeoJSON data and exits at once, never creating a
// runtime. The check is the exit itself: preparation starts the library's
// GeoJSON worker threads, and none may still be starting when static
// destruction runs. The program returns from main straight after its calls,
// so no harness teardown gives a starting thread time to finish.

#include <stdio.h>
#include <string.h>

#include "maplibre_native_c.h"

int main(void) {
  static const char json[] =
    "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
    "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"
    "\"properties\":{}}]}";
  // Preparation spreads datasets over ten sequenced workers, so ten
  // preparations start every one of them.
  for (int index = 0; index < 10; index += 1) {
    mln_geojson_source_data data = MLN_HANDLE_NULL;
    mln_diagnostic diagnostic = {.size = sizeof(mln_diagnostic)};
    const mln_status status = mln_geojson_source_data_create(
      (mln_buffer_view){.data = json, .size = strlen(json)}, NULL, &data,
      &diagnostic
    );
    if (status != MLN_STATUS_OK) {
      (void)fprintf(stderr, "preparation failed: %s\n", diagnostic.message);
      return 1;
    }
    mln_geojson_source_data_destroy(data);
  }
  return 0;
}
