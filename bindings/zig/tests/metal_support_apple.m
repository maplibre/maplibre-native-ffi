#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <TargetConditionals.h>

#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct mln_zig_test_window_metal_layer {
  void* window;
  void* layer;
} mln_zig_test_window_metal_layer;

@interface MLNZigTestCountingMetalLayer : CAMetalLayer
@property(nonatomic) uint32_t nextDrawableCount;
@end

@implementation MLNZigTestCountingMetalLayer
- (id<CAMetalDrawable>)nextDrawable {
  _nextDrawableCount += 1;
  return [super nextDrawable];
}
@end

extern void* objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void* pool);

void* mln_zig_test_autorelease_pool_push(void) {
  return objc_autoreleasePoolPush();
}

void mln_zig_test_autorelease_pool_pop(void* pool) {
  objc_autoreleasePoolPop(pool);
}

bool mln_zig_test_create_counting_window_metal_layer(
  uint32_t width, uint32_t height, mln_zig_test_window_metal_layer* out_layer
) {
  if (out_layer == NULL || width == 0 || height == 0) {
    return false;
  }

#if TARGET_OS_OSX
  NSWindow* window = nil;
  @try {
    [NSApplication sharedApplication];
    const NSRect frame = NSMakeRect(0.0, 0.0, (CGFloat)width, (CGFloat)height);
    window = [[NSWindow alloc] initWithContentRect:frame
                                         styleMask:NSWindowStyleMaskBorderless
                                           backing:NSBackingStoreBuffered
                                             defer:YES];
    NSView* content_view = [window contentView];
    MLNZigTestCountingMetalLayer* layer = [MLNZigTestCountingMetalLayer layer];
    if (window == nil || content_view == nil || layer == nil) {
      [window release];
      return false;
    }
    [window setReleasedWhenClosed:NO];
    [content_view setWantsLayer:YES];
    [content_view setLayer:layer];
    layer.drawableSize = CGSizeMake(1.0, 1.0);
    out_layer->window = window;
    out_layer->layer = layer;
    return true;
  } @catch (NSException* exception) {
    (void)exception;
    [window release];
    return false;
  }
#else
  MLNZigTestCountingMetalLayer* layer =
    [[MLNZigTestCountingMetalLayer alloc] init];
  if (layer == nil) {
    return false;
  }
  layer.frame = CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height);
  layer.drawableSize = CGSizeMake(1.0, 1.0);
  out_layer->window = layer;
  out_layer->layer = layer;
  return true;
#endif
}

uint32_t mln_zig_test_metal_layer_next_drawable_count(void* layer) {
  id object = (id)layer;
  if (![object isKindOfClass:[MLNZigTestCountingMetalLayer class]]) {
    return 0;
  }
  return [(MLNZigTestCountingMetalLayer*)object nextDrawableCount];
}

bool mln_zig_test_metal_layer_has_device(void* layer) {
  id object = (id)layer;
  return [object isKindOfClass:[CAMetalLayer class]] &&
         [(CAMetalLayer*)object device] != nil;
}

bool mln_zig_test_metal_layer_drawable_size(
  void* layer, uint32_t* out_width, uint32_t* out_height
) {
  id object = (id)layer;
  if (
    ![object isKindOfClass:[CAMetalLayer class]] || out_width == NULL ||
    out_height == NULL
  ) {
    return false;
  }
  const CGSize size = [(CAMetalLayer*)object drawableSize];
  *out_width = (uint32_t)size.width;
  *out_height = (uint32_t)size.height;
  return true;
}

void mln_zig_test_destroy_window_metal_layer(
  mln_zig_test_window_metal_layer* window_layer
) {
  if (window_layer == NULL) {
    return;
  }
#if TARGET_OS_OSX
  [(NSWindow*)window_layer->window release];
#else
  [(id)window_layer->window release];
#endif
  window_layer->window = NULL;
  window_layer->layer = NULL;
}
