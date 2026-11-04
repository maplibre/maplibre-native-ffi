#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>
#import <react/renderer/components/MaplibreExampleHostSpec/ComponentDescriptors.h>
#import <react/renderer/components/MaplibreExampleHostSpec/EventEmitters.h>
#import <react/renderer/components/MaplibreExampleHostSpec/Props.h>
#import <react/renderer/components/MaplibreExampleHostSpec/RCTComponentViewHelpers.h>

#import "MLNExampleMapSurfaceView.h"

#include "Surfaces.h"

using namespace facebook;
using namespace facebook::react;

namespace {

class MetalSurface final : public mln::example::Surface {
 public:
  explicit MetalSurface(CAMetalLayer* layer) : layer_(layer) {}

  jsi::Object describe(jsi::Runtime& runtime) override {
    auto handles = jsi::Object(runtime);
    handles.setProperty(runtime, "kind", "metal");
    handles.setProperty(
      runtime, "layer",
      jsi::BigInt::fromUint64(
        runtime, static_cast<uint64_t>(
                   reinterpret_cast<uintptr_t>((__bridge void*)layer_)
                 )
      )
    );
    return handles;
  }

 private:
  // The registry holds the layer until the session stops using it.
  CAMetalLayer* layer_;
};

}  // namespace

@interface MLNExampleMetalView : UIView
@end

@implementation MLNExampleMetalView
+ (Class)layerClass {
  return [CAMetalLayer class];
}
@end

@implementation MLNExampleMapSurfaceView {
  MLNExampleMetalView* _metalView;
  int32_t _surface;
  BOOL _background;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider {
  return concreteComponentDescriptorProvider<
    MapSurfaceViewComponentDescriptor>();
}

// A recycled view would lend a layer that a session from its last mount may
// still use.
+ (BOOL)shouldBeRecycled {
  return NO;
}

- (instancetype)initWithFrame:(CGRect)frame {
  if (self = [super initWithFrame:frame]) {
    static const auto defaultProps =
      std::make_shared<const MapSurfaceViewProps>();
    _props = defaultProps;
    _metalView = [[MLNExampleMetalView alloc] initWithFrame:self.bounds];
    _metalView.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    // A plain UIView takes one touch at a time. Touches go to this view
    // instead, which takes several, so a pinch reaches JavaScript.
    _metalView.userInteractionEnabled = NO;
    self.contentView = _metalView;
    _background = UIApplication.sharedApplication.applicationState ==
                  UIApplicationStateBackground;
    auto center = NSNotificationCenter.defaultCenter;
    [center addObserver:self
               selector:@selector(didEnterBackground)
                   name:UIApplicationDidEnterBackgroundNotification
                 object:nil];
    [center addObserver:self
               selector:@selector(willEnterForeground)
                   name:UIApplicationWillEnterForegroundNotification
                 object:nil];
  }
  return self;
}

- (void)dealloc {
  [NSNotificationCenter.defaultCenter removeObserver:self];
  [self withdraw];
}

// The view reports its surface through its event emitter, which Fabric may
// set after the view enters a window.
- (void)updateEventEmitter:(const EventEmitter::Shared&)eventEmitter {
  [super updateEventEmitter:eventEmitter];
  [self update];
}

- (void)didMoveToWindow {
  [super didMoveToWindow];
  [self update];
}

- (void)didEnterBackground {
  _background = YES;
  [self update];
}

- (void)willEnterForeground {
  _background = NO;
  [self update];
}

- (void)update {
  if (self.window != nil && !_background) {
    [self lend];
  } else {
    [self withdraw];
  }
}

- (void)lend {
  if (_surface != 0 || _eventEmitter == nullptr) return;
  auto* layer = (CAMetalLayer*)_metalView.layer;
  layer.contentsScale = self.window.screen.scale;
  _surface = mln::example::surfaces::add(std::make_shared<MetalSurface>(layer));
  std::static_pointer_cast<const MapSurfaceViewEventEmitter>(_eventEmitter)
    ->onSurfaceAvailable({.surface = _surface});
}

- (void)withdraw {
  if (_surface == 0) return;
  auto surface = _surface;
  _surface = 0;
  mln::example::surfaces::withdraw(surface);
  if (_eventEmitter != nullptr) {
    std::static_pointer_cast<const MapSurfaceViewEventEmitter>(_eventEmitter)
      ->onSurfaceLost({.surface = surface});
  }
}

@end
