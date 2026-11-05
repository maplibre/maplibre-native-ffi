#include <errno.h>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "dma-buf.h"

int example_dma_buf_dup(int fd) { return fcntl(fd, F_DUPFD_CLOEXEC, 3); }

int example_dma_buf_export_sync_file(int fd) {
  // A sync file for writing waits for the buffer's readers as well as its
  // writers.
  struct dma_buf_export_sync_file data = {
    .flags = DMA_BUF_SYNC_WRITE,
    .fd = -1,
  };
  int result;
  do {
    result = ioctl(fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &data);
  } while (result == -1 && (errno == EINTR || errno == EAGAIN));
  return result == 0 ? data.fd : -1;
}

void example_dma_buf_wait(int fd) {
  // A dmabuf polls writable once every reader and writer is done with it.
  struct pollfd poll_fd = {.fd = fd, .events = POLLOUT};
  while (poll(&poll_fd, 1, -1) == -1 && errno == EINTR) {
  }
}
