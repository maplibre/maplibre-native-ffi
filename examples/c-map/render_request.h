// The one-bit render request shared with notification callbacks.

#ifndef C_MAP_RENDER_REQUEST_H
#define C_MAP_RENDER_REQUEST_H

#include <stdatomic.h>

/// One-bit signal that a frame is worth drawing.
typedef struct render_request {
  atomic_bool value;
} render_request;

void render_request_init(render_request* request);
void render_request_set(render_request* request);
bool render_request_consume(render_request* request);

#endif  // C_MAP_RENDER_REQUEST_H
