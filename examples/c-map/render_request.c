#include "render_request.h"

void render_request_init(render_request* request) {
  atomic_init(&request->value, false);
}

void render_request_set(render_request* request) {
  atomic_store_explicit(&request->value, true, memory_order_release);
}

bool render_request_consume(render_request* request) {
  return atomic_exchange_explicit(&request->value, false, memory_order_acq_rel);
}
