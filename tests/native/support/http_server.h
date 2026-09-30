#ifndef MLN_NATIVE_TESTS_HTTP_SERVER_H
#define MLN_NATIVE_TESTS_HTTP_SERVER_H

// A scripted HTTP/1.1 server on the loopback interface, for the cases that
// exercise the platform transport and the header transform. It never reaches
// past 127.0.0.1, answers each request from a fixed route table, closes every
// connection after one response, and logs each request's path and headers for
// a case to inspect.
//
// The browser has no sockets, so the browser build leaves this file out; its
// transport runs against the runner's routes instead.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mln_test_http_route {
  // The request path to answer, without its query, matched exactly.
  const char* path;
  // The status to answer with. 0 means 200.
  int status;
  // Extra response header lines, each ending in "\r\n", or null.
  const char* headers;
  const void* body;
  size_t body_size;
  // When set, the response carries this ETag, and a request whose
  // If-None-Match names it is answered 304 with no body.
  const char* etag;
  // Answers a request with a `Range: bytes=first-last` header with 206 and
  // that slice of the body.
  bool serves_ranges;
  // Holds the response until mln_test_http_server_release_held().
  bool held;
} mln_test_http_route;

typedef struct mln_test_http_server mln_test_http_server;

// Starts a server on an ephemeral 127.0.0.1 port. The routes need to outlive
// the server. A path with no route is answered 404. Fails the case when the
// server cannot start.
mln_test_http_server* mln_test_http_server_start(
  const mln_test_http_route* routes, size_t route_count
);
// Releases every held response, closes open connections, and joins the
// server's threads.
void mln_test_http_server_stop(mln_test_http_server* server);

// Writes `http://127.0.0.1:<port><path>`. Fails the case when it does not fit.
void mln_test_http_server_url(
  const mln_test_http_server* server, const char* path, char* out,
  size_t capacity
);

// How many requests for `path` arrived, counting held ones.
int mln_test_http_server_requests(
  mln_test_http_server* server, const char* path
);
// Waits within the default deadline until `count` requests for `path` have
// arrived.
bool mln_test_http_server_wait_for_requests(
  mln_test_http_server* server, const char* path, int count
);
// Copies the value of header `name`, matched case-insensitively, from the
// `index`th request for `path`. Returns false when that request or header does
// not exist, leaving `out` empty.
bool mln_test_http_server_request_header(
  mln_test_http_server* server, const char* path, int index, const char* name,
  char* out, size_t capacity
);
// Answers every held request, and every later one for a held route.
void mln_test_http_server_release_held(mln_test_http_server* server);

#ifdef __cplusplus
}
#endif

#endif
