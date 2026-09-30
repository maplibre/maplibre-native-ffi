// A scripted loopback HTTP/1.1 server; see http_server.h.
//
// One thread accepts connections and hands each to a thread of its own, which
// reads one request head, logs it, answers from the route table, and closes
// the connection. Stopping wakes the accept thread with a connection of its
// own and shuts down every open connection, so no thread stays blocked.

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_server.h"

#include "unity.h"
#include "wait.h"

#if defined(_WIN32)
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_handle;
#define INVALID_SOCKET_HANDLE INVALID_SOCKET
#define close_socket closesocket
#define SHUTDOWN_BOTH SD_BOTH
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int socket_handle;
#define INVALID_SOCKET_HANDLE (-1)
#define close_socket close
#define SHUTDOWN_BOTH SHUT_RDWR
#endif

enum {
  request_head_capacity = 16384,
  logged_request_capacity = 256,
  open_connection_capacity = 64,
};

typedef struct logged_request {
  char path[512];
  char head[request_head_capacity];
} logged_request;

#if defined(_WIN32)
typedef CRITICAL_SECTION server_mutex;
typedef CONDITION_VARIABLE server_condition;
// Windows names its condition wait after sleeping, which it is not: it blocks
// until a broadcast.
#define wait_on_condition SleepConditionVariableCS
#else
typedef pthread_mutex_t server_mutex;
typedef pthread_cond_t server_condition;
#endif

struct mln_test_http_server {
  const mln_test_http_route* routes;
  size_t route_count;
  socket_handle listener;
  uint16_t port;
  atomic_bool stopping;

  server_mutex mutex;
  // Signalled when a held response is released or a connection thread exits.
  server_condition changed;
  bool held_released;
  size_t open_connections;
  socket_handle connections[open_connection_capacity];
  logged_request* requests;
  size_t request_count;

#if defined(_WIN32)
  HANDLE accept_thread;
#else
  pthread_t accept_thread;
#endif
};

static void lock_server(mln_test_http_server* server) {
#if defined(_WIN32)
  EnterCriticalSection(&server->mutex);
#else
  pthread_mutex_lock(&server->mutex);
#endif
}

static void unlock_server(mln_test_http_server* server) {
#if defined(_WIN32)
  LeaveCriticalSection(&server->mutex);
#else
  pthread_mutex_unlock(&server->mutex);
#endif
}

static void wait_server(mln_test_http_server* server) {
#if defined(_WIN32)
  wait_on_condition(&server->changed, &server->mutex, INFINITE);
#else
  pthread_cond_wait(&server->changed, &server->mutex);
#endif
}

static void broadcast_server(mln_test_http_server* server) {
#if defined(_WIN32)
  WakeAllConditionVariable(&server->changed);
#else
  pthread_cond_broadcast(&server->changed);
#endif
}

typedef struct connection_start {
  mln_test_http_server* server;
  socket_handle socket;
} connection_start;

static bool send_all(socket_handle socket, const void* bytes, size_t size) {
  const char* cursor = bytes;
  while (size > 0) {
    const int chunk = size > 65536 ? 65536 : (int)size;
#if defined(_WIN32)
    const int sent = send(socket, cursor, chunk, 0);
#elif defined(MSG_NOSIGNAL)
    // A client that hung up must not raise SIGPIPE in the suite.
    const ssize_t sent = send(socket, cursor, (size_t)chunk, MSG_NOSIGNAL);
#else
    const ssize_t sent = send(socket, cursor, (size_t)chunk, 0);
#endif
    if (sent <= 0) {
      return false;
    }
    cursor += sent;
    size -= (size_t)sent;
  }
  return true;
}

// Reads until the blank line that ends a request head, and returns its length,
// or 0 when the client closed or sent more than the buffer holds.
static size_t read_request_head(socket_handle socket, char* head) {
  size_t size = 0;
  while (size + 1 < request_head_capacity) {
    const int received = (int)recv(
      socket, head + size, (int)(request_head_capacity - 1 - size), 0
    );
    if (received <= 0) {
      return 0;
    }
    size += (size_t)received;
    head[size] = '\0';
    if (strstr(head, "\r\n\r\n") != NULL) {
      return size;
    }
  }
  return 0;
}

static bool header_name_matches(
  const char* line, size_t name_size, const char* name
) {
  if (strlen(name) != name_size) {
    return false;
  }
  for (size_t index = 0; index < name_size; index += 1) {
    if (
      tolower((unsigned char)line[index]) != tolower((unsigned char)name[index])
    ) {
      return false;
    }
  }
  return true;
}

// Copies the value of header `name` out of a request head.
static bool find_header(
  const char* head, const char* name, char* out, size_t capacity
) {
  if (capacity > 0) {
    out[0] = '\0';
  }
  const char* line = strstr(head, "\r\n");
  while (line != NULL) {
    line += 2;
    const char* end = strstr(line, "\r\n");
    if (end == NULL || end == line) {
      return false;
    }
    const char* colon = memchr(line, ':', (size_t)(end - line));
    if (
      colon != NULL && header_name_matches(line, (size_t)(colon - line), name)
    ) {
      const char* value = colon + 1;
      while (value < end && (*value == ' ' || *value == '\t')) {
        value += 1;
      }
      const size_t size = (size_t)(end - value);
      if (size >= capacity) {
        return false;
      }
      memcpy(out, value, size);
      out[size] = '\0';
      return true;
    }
    line = end;
  }
  return false;
}

// Extracts the request target's path, without its query.
static bool request_path(const char* head, char* out, size_t capacity) {
  const char* target = strchr(head, ' ');
  if (target == NULL) {
    return false;
  }
  target += 1;
  size_t size = strcspn(target, " ?#\r\n");
  if (size >= capacity) {
    return false;
  }
  memcpy(out, target, size);
  out[size] = '\0';
  return true;
}

static const mln_test_http_route* find_route(
  const mln_test_http_server* server, const char* path
) {
  for (size_t index = 0; index < server->route_count; index += 1) {
    if (strcmp(server->routes[index].path, path) == 0) {
      return &server->routes[index];
    }
  }
  return NULL;
}

static const char* reason_phrase(int status) {
  switch (status) {
    case 200:
      return "OK";
    case 204:
      return "No Content";
    case 206:
      return "Partial Content";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 304:
      return "Not Modified";
    case 307:
      return "Temporary Redirect";
    case 404:
      return "Not Found";
    case 416:
      return "Range Not Satisfiable";
    case 500:
      return "Internal Server Error";
    case 503:
      return "Service Unavailable";
    default:
      return "Status";
  }
}

static void respond(
  socket_handle socket, int status, const char* extra_headers, const void* body,
  size_t body_size
) {
  char head[2048];
  const int written = snprintf(
    head, sizeof(head),
    "HTTP/1.1 %d %s\r\nContent-Length: %zu\r\nConnection: close\r\n%s\r\n",
    status, reason_phrase(status), body_size,
    extra_headers == NULL ? "" : extra_headers
  );
  if (written <= 0 || (size_t)written >= sizeof(head)) {
    return;
  }
  if (send_all(socket, head, (size_t)written) && body_size > 0) {
    (void)send_all(socket, body, body_size);
  }
}

static void answer(
  mln_test_http_server* server, socket_handle socket, const char* head,
  const char* path
) {
  const mln_test_http_route* route = find_route(server, path);
  if (route == NULL) {
    respond(socket, 404, NULL, NULL, 0);
    return;
  }
  if (route->held) {
    lock_server(server);
    while (!server->held_released && !atomic_load(&server->stopping)) {
      wait_server(server);
    }
    unlock_server(server);
    if (atomic_load(&server->stopping)) {
      return;
    }
  }

  char headers[1024] = {0};
  if (route->headers != NULL) {
    (void)snprintf(headers, sizeof(headers), "%s", route->headers);
  }
  if (route->etag != NULL) {
    const size_t used = strlen(headers);
    (void)snprintf(
      headers + used, sizeof(headers) - used, "ETag: %s\r\n", route->etag
    );
    char if_none_match[256];
    if (
      find_header(
        head, "If-None-Match", if_none_match, sizeof(if_none_match)
      ) &&
      strcmp(if_none_match, route->etag) == 0
    ) {
      respond(socket, 304, headers, NULL, 0);
      return;
    }
  }

  char range[128];
  unsigned long long first = 0;
  unsigned long long last = 0;
  if (
    route->serves_ranges && find_header(head, "Range", range, sizeof(range)) &&
    sscanf(range, "bytes=%llu-%llu", &first, &last) == 2
  ) {
    if (first >= route->body_size || last < first) {
      respond(socket, 416, headers, NULL, 0);
      return;
    }
    if (last >= route->body_size) {
      last = route->body_size - 1;
    }
    const size_t used = strlen(headers);
    (void)snprintf(
      headers + used, sizeof(headers) - used,
      "Content-Range: bytes %llu-%llu/%zu\r\n", first, last, route->body_size
    );
    respond(
      socket, 206, headers, (const char*)route->body + first,
      (size_t)(last - first + 1)
    );
    return;
  }

  respond(
    socket, route->status == 0 ? 200 : route->status, headers, route->body,
    route->body_size
  );
}

static void log_request(
  mln_test_http_server* server, const char* path, const char* head
) {
  lock_server(server);
  if (server->request_count < logged_request_capacity) {
    logged_request* entry = &server->requests[server->request_count];
    (void)snprintf(entry->path, sizeof(entry->path), "%s", path);
    (void)snprintf(entry->head, sizeof(entry->head), "%s", head);
    server->request_count += 1;
  }
  unlock_server(server);
  mln_test_pulse();
}

static void forget_connection(
  mln_test_http_server* server, socket_handle socket
) {
  lock_server(server);
  for (size_t index = 0; index < open_connection_capacity; index += 1) {
    if (server->connections[index] == socket) {
      server->connections[index] = INVALID_SOCKET_HANDLE;
      break;
    }
  }
  server->open_connections -= 1;
  broadcast_server(server);
  unlock_server(server);
}

static void serve_connection(connection_start* start) {
  mln_test_http_server* server = start->server;
  const socket_handle socket = start->socket;
  free(start);

  char* head = malloc(request_head_capacity);
  char path[512];
  if (
    head != NULL && read_request_head(socket, head) != 0 &&
    request_path(head, path, sizeof(path))
  ) {
    log_request(server, path, head);
    answer(server, socket, head, path);
  }
  free(head);
  (void)shutdown(socket, SHUTDOWN_BOTH);
  close_socket(socket);
  forget_connection(server, socket);
}

#if defined(_WIN32)
static DWORD WINAPI connection_thread(LPVOID argument) {
  serve_connection(argument);
  return 0;
}
#else
static void* connection_thread(void* argument) {
  serve_connection(argument);
  return NULL;
}
#endif

static bool start_connection(
  mln_test_http_server* server, socket_handle socket
) {
  connection_start* start = malloc(sizeof(*start));
  if (start == NULL) {
    return false;
  }
  start->server = server;
  start->socket = socket;
  lock_server(server);
  bool tracked = false;
  for (size_t index = 0; index < open_connection_capacity; index += 1) {
    if (server->connections[index] == INVALID_SOCKET_HANDLE) {
      server->connections[index] = socket;
      tracked = true;
      break;
    }
  }
  if (tracked) {
    server->open_connections += 1;
  }
  unlock_server(server);
  if (!tracked) {
    free(start);
    return false;
  }
#if defined(_WIN32)
  HANDLE thread = CreateThread(NULL, 0, connection_thread, start, 0, NULL);
  if (thread != NULL) {
    CloseHandle(thread);
    return true;
  }
#else
  pthread_t thread;
  if (pthread_create(&thread, NULL, connection_thread, start) == 0) {
    pthread_detach(thread);
    return true;
  }
#endif
  free(start);
  forget_connection(server, socket);
  return false;
}

static void accept_connections(mln_test_http_server* server) {
  while (!atomic_load(&server->stopping)) {
    const socket_handle socket = accept(server->listener, NULL, NULL);
    if (socket == INVALID_SOCKET_HANDLE) {
      continue;
    }
#if defined(SO_NOSIGPIPE)
    // Apple platforms have no MSG_NOSIGNAL, so the socket opts out instead.
    const int no_sigpipe = 1;
    (void)setsockopt(
      socket, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe)
    );
#endif
    if (atomic_load(&server->stopping) || !start_connection(server, socket)) {
      close_socket(socket);
    }
  }
}

#if defined(_WIN32)
static DWORD WINAPI accept_thread(LPVOID argument) {
  accept_connections(argument);
  return 0;
}
#else
static void* accept_thread(void* argument) {
  accept_connections(argument);
  return NULL;
}
#endif

static struct sockaddr_in loopback_address(uint16_t port) {
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  (void)inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
  address.sin_port = htons(port);
  return address;
}

mln_test_http_server* mln_test_http_server_start(
  const mln_test_http_route* routes, size_t route_count
) {
#if defined(_WIN32)
  static atomic_bool winsock_started;
  if (!atomic_exchange(&winsock_started, true)) {
    WSADATA data;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      0, WSAStartup(MAKEWORD(2, 2), &data), "WSAStartup failed"
    );
  }
#endif
  mln_test_http_server* server = calloc(1, sizeof(*server));
  TEST_ASSERT_NOT_NULL(server);
  server->requests = calloc(logged_request_capacity, sizeof(logged_request));
  TEST_ASSERT_NOT_NULL(server->requests);
  server->routes = routes;
  server->route_count = route_count;
  atomic_init(&server->stopping, false);
  for (size_t index = 0; index < open_connection_capacity; index += 1) {
    server->connections[index] = INVALID_SOCKET_HANDLE;
  }
#if defined(_WIN32)
  InitializeCriticalSection(&server->mutex);
  InitializeConditionVariable(&server->changed);
#else
  pthread_mutex_init(&server->mutex, NULL);
  pthread_cond_init(&server->changed, NULL);
#endif

  server->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  TEST_ASSERT_TRUE_MESSAGE(
    server->listener != INVALID_SOCKET_HANDLE, "the test server has no socket"
  );
  struct sockaddr_in address = loopback_address(0);
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    0, bind(server->listener, (struct sockaddr*)&address, sizeof(address)),
    "the test server could not bind 127.0.0.1"
  );
  TEST_ASSERT_EQUAL_INT(0, listen(server->listener, 64));
  socklen_t address_size = sizeof(address);
  TEST_ASSERT_EQUAL_INT(
    0, getsockname(server->listener, (struct sockaddr*)&address, &address_size)
  );
  server->port = ntohs(address.sin_port);

#if defined(_WIN32)
  server->accept_thread = CreateThread(NULL, 0, accept_thread, server, 0, NULL);
  TEST_ASSERT_NOT_NULL(server->accept_thread);
#else
  TEST_ASSERT_EQUAL_INT(
    0, pthread_create(&server->accept_thread, NULL, accept_thread, server)
  );
#endif
  return server;
}

void mln_test_http_server_stop(mln_test_http_server* server) {
  if (server == NULL) {
    return;
  }
  atomic_store(&server->stopping, true);
  // A connection of its own wakes the accept thread, which then sees the flag.
  const socket_handle waker = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (waker != INVALID_SOCKET_HANDLE) {
    struct sockaddr_in address = loopback_address(server->port);
    (void)connect(waker, (struct sockaddr*)&address, sizeof(address));
  }
#if defined(_WIN32)
  WaitForSingleObject(server->accept_thread, INFINITE);
  CloseHandle(server->accept_thread);
#else
  pthread_join(server->accept_thread, NULL);
#endif
  if (waker != INVALID_SOCKET_HANDLE) {
    close_socket(waker);
  }
  close_socket(server->listener);

  lock_server(server);
  server->held_released = true;
  for (size_t index = 0; index < open_connection_capacity; index += 1) {
    if (server->connections[index] != INVALID_SOCKET_HANDLE) {
      (void)shutdown(server->connections[index], SHUTDOWN_BOTH);
    }
  }
  broadcast_server(server);
  while (server->open_connections > 0) {
    wait_server(server);
  }
  unlock_server(server);

#if defined(_WIN32)
  DeleteCriticalSection(&server->mutex);
#else
  pthread_cond_destroy(&server->changed);
  pthread_mutex_destroy(&server->mutex);
#endif
  free(server->requests);
  free(server);
}

uint16_t mln_test_http_server_port(const mln_test_http_server* server) {
  return server->port;
}

void mln_test_http_server_url(
  const mln_test_http_server* server, const char* path, char* out,
  size_t capacity
) {
  const int written =
    snprintf(out, capacity, "http://127.0.0.1:%u%s", server->port, path);
  TEST_ASSERT_TRUE_MESSAGE(
    written > 0 && (size_t)written < capacity, "the test URL does not fit"
  );
}

int mln_test_http_server_requests(
  mln_test_http_server* server, const char* path
) {
  int count = 0;
  lock_server(server);
  for (size_t index = 0; index < server->request_count; index += 1) {
    if (strcmp(server->requests[index].path, path) == 0) {
      count += 1;
    }
  }
  unlock_server(server);
  return count;
}

typedef struct request_count_target {
  mln_test_http_server* server;
  const char* path;
  int count;
} request_count_target;

static bool request_count_reached(void* context) {
  const request_count_target* target = context;
  return mln_test_http_server_requests(target->server, target->path) >=
         target->count;
}

bool mln_test_http_server_wait_for_requests(
  mln_test_http_server* server, const char* path, int count
) {
  request_count_target target = {
    .server = server, .path = path, .count = count
  };
  return mln_test_await(
    request_count_reached, &target, mln_test_deadline_default(),
    "a request to the test HTTP server"
  );
}

bool mln_test_http_server_request_header(
  mln_test_http_server* server, const char* path, int index, const char* name,
  char* out, size_t capacity
) {
  if (capacity > 0) {
    out[0] = '\0';
  }
  bool found = false;
  int seen = 0;
  lock_server(server);
  for (size_t entry = 0; entry < server->request_count; entry += 1) {
    if (strcmp(server->requests[entry].path, path) != 0) {
      continue;
    }
    if (seen == index) {
      found = find_header(server->requests[entry].head, name, out, capacity);
      break;
    }
    seen += 1;
  }
  unlock_server(server);
  return found;
}

void mln_test_http_server_release_held(mln_test_http_server* server) {
  lock_server(server);
  server->held_released = true;
  broadcast_server(server);
  unlock_server(server);
}
