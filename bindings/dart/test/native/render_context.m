#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

__attribute__((visibility("default"))) void* mln_dart_test_metal_device_create(
  void
) {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  return device == nil ? NULL : (void*)[device retain];
}

__attribute__((visibility("default"))) void mln_dart_test_metal_object_release(
  void* object
) {
  if (object != NULL) {
    [(id)object release];
  }
}
