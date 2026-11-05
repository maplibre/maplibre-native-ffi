// The dmabuf calls that the example makes, which Vala's Posix bindings can't
// spell: a close-on-exec duplicate, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, and a wait
// for a dmabuf's readers.

#ifndef EXAMPLE_DMA_BUF_H
#define EXAMPLE_DMA_BUF_H

// Returns a close-on-exec duplicate of `fd`, or -1.
int example_dma_buf_dup(int fd);

// Returns a sync file that signals once every reader and writer of the dmabuf
// that `fd` names is done with it, or -1 when the kernel can't export one.
int example_dma_buf_export_sync_file(int fd);

// Waits until every reader and writer of the dmabuf that `fd` names is done
// with it.
void example_dma_buf_wait(int fd);

#endif
