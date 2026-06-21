#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <jni.h>

@interface ComposeMapSkikoMetalDevice : NSObject
@property(strong, nonatomic) id<MTLDevice> adapter;
@property(strong, nonatomic) id<MTLCommandQueue> queue;
@end

extern "C" {

JNIEXPORT jlong JNICALL
Java_org_maplibre_nativeffi_examples_composemap_surface_MacMetalBridgeNative_createMetalTexture(
  JNIEnv* env, jclass, jlong metalDevicePtr, jlong oldTexturePtr, jint width,
  jint height
) {
  @autoreleasepool {
    ComposeMapSkikoMetalDevice* skikoDevice =
      (__bridge ComposeMapSkikoMetalDevice*)(void*)metalDevicePtr;
    id<MTLTexture> oldTexture =
      (__bridge_transfer id<MTLTexture>)(void*)oldTexturePtr;
    if (
      oldTexture != nil && oldTexture.width == width &&
      oldTexture.height == height
    ) {
      return (jlong)(__bridge_retained void*)oldTexture;
    }

    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
      texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                   width:width
                                  height:height
                               mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    descriptor.storageMode = MTLStorageModePrivate;
    id<MTLTexture> texture =
      [skikoDevice.adapter newTextureWithDescriptor:descriptor];
    return (jlong)(__bridge_retained void*)texture;
  }
}

JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_examples_composemap_surface_MacMetalBridgeNative_disposeMetalTexture(
  JNIEnv*, jclass, jlong texturePtr
) {
  @autoreleasepool {
    id<MTLTexture> texture =
      (__bridge_transfer id<MTLTexture>)(void*)texturePtr;
    (void)texture;
  }
}

JNIEXPORT jlong JNICALL
Java_org_maplibre_nativeffi_examples_composemap_surface_MacMetalBridgeNative_texturePixelFormat(
  JNIEnv*, jclass, jlong texturePtr
) {
  @autoreleasepool {
    id<MTLTexture> texture = (__bridge id<MTLTexture>)(void*)texturePtr;
    return texture == nil ? 0 : static_cast<jlong>(texture.pixelFormat);
  }
}

}  // extern C
