#import <React/RCTViewComponentView.h>

// Lends its CAMetalLayer to JavaScript through the surface registry while the
// view is in a window and the app is in the foreground, since iOS doesn't let
// a background app render.
@interface MLNExampleMapSurfaceView : RCTViewComponentView
@end
