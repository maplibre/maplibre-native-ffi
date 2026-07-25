#import <AppKit/AppKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <stdbool.h>
#include <stdint.h>

// Small test-only Objective-C helpers used by the direct Vala binding smoke
// tests. They create host-owned backend objects so the public Vala wrappers can
// exercise borrowed texture and surface attachment paths without exposing raw
// Objective-C APIs in Vala.

typedef struct MlnValaMetalWindowLayer {
  void* window;
  void* layer;
} MlnValaMetalWindowLayer;

bool mln_vala_metal_test_window_layer_create(
  uint32_t width, uint32_t height, MlnValaMetalWindowLayer* out_layer
) {
  if (out_layer == NULL || width == 0 || height == 0) {
    return false;
  }
  out_layer->window = NULL;
  out_layer->layer = NULL;

  NSWindow* window = nil;
  @try {
    [NSApplication sharedApplication];

    const NSRect frame = NSMakeRect(0.0, 0.0, (CGFloat)width, (CGFloat)height);
    window = [[NSWindow alloc] initWithContentRect:frame
                                         styleMask:NSWindowStyleMaskBorderless
                                           backing:NSBackingStoreBuffered
                                             defer:YES];
    if (window == nil) {
      return false;
    }

    [window setReleasedWhenClosed:NO];
    NSView* content_view = [window contentView];
    CAMetalLayer* layer = [CAMetalLayer layer];
    if (content_view == nil || layer == nil) {
      [window release];
      return false;
    }

    [content_view setWantsLayer:YES];
    [content_view setLayer:layer];
    out_layer->window = window;
    out_layer->layer = layer;
    return true;
  } @catch (NSException* exception) {
    (void)exception;
    [window release];
    return false;
  }
}

void mln_vala_metal_test_window_layer_destroy(
  MlnValaMetalWindowLayer* window_layer
) {
  if (window_layer == NULL) {
    return;
  }
  [(NSWindow*)window_layer->window release];
  window_layer->window = NULL;
  window_layer->layer = NULL;
}

void* mln_vala_metal_test_texture_create(
  void* device, uint32_t width, uint32_t height
) {
  if (device == NULL || width == 0 || height == 0) {
    return NULL;
  }
  MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
    texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                 width:(NSUInteger)width
                                height:(NSUInteger)height
                             mipmapped:NO];
  descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
  return [(id<MTLDevice>)device newTextureWithDescriptor:descriptor];
}

void mln_vala_metal_test_object_release(void* object) { [(id)object release]; }
