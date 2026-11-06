import { MapController, Status } from '../map/MapController';
import { MapGestures } from '../map/gestures';

// Reports the XComponent's surface to the map controller. The surface's size
// is in physical pixels, and the map's is in virtual pixels.
class MapSurfaceController extends XComponentController {
  private readonly map: MapController;
  private readonly context: UIContext;

  constructor(map: MapController, context: UIContext) {
    super();
    this.map = map;
    this.context = context;
  }

  onSurfaceCreated(surfaceId: string): void {
    this.map.surfaceAvailable(BigInt(surfaceId));
  }

  onSurfaceChanged(surfaceId: string, rect: SurfaceRect): void {
    this.map.setSize({
      width: Math.max(1, Math.round(this.context.px2vp(rect.surfaceWidth))),
      height: Math.max(1, Math.round(this.context.px2vp(rect.surfaceHeight))),
    });
  }

  onSurfaceDestroyed(surfaceId: string): void {
    this.map.surfaceLost(BigInt(surfaceId));
  }
}

@Entry
@Component
struct Index {
  @State status: Status = { message: '', error: false };
  private map: MapController = new MapController(this.getUIContext().vp2px(1), (status: Status) => {
    this.status = status;
  });
  private surface: MapSurfaceController = new MapSurfaceController(this.map, this.getUIContext());
  private gestures: MapGestures = new MapGestures(() => this.map.currentMap);

  aboutToAppear(): void {
    this.map.start();
  }

  aboutToDisappear(): void {
    this.map.dispose();
  }

  build() {
    Stack({ alignContent: Alignment.Bottom }) {
      XComponent({ type: XComponentType.SURFACE, controller: this.surface })
        .width('100%')
        .height('100%')
        .onTouch((event: TouchEvent) => this.gestures.onTouch(event))
        .gesture(GestureGroup(GestureMode.Parallel,
          TapGesture({ count: 2 }).onAction((event: GestureEvent) => this.gestures.doubleTap(event)),
          PanGesture()
            .onActionStart((event: GestureEvent) => this.gestures.panStart(event))
            .onActionUpdate((event: GestureEvent) => this.gestures.pan(event))
            .onActionEnd((event: GestureEvent) => this.gestures.panEnd(event))
            .onActionCancel(() => this.gestures.cancel()),
          PinchGesture()
            .onActionStart((event: GestureEvent) => this.gestures.pinchStart(event))
            .onActionUpdate((event: GestureEvent) => this.gestures.pinch(event)),
          RotationGesture()
            .onActionStart((event: GestureEvent) => this.gestures.rotateStart(event))
            .onActionUpdate((event: GestureEvent) => this.gestures.rotate(event))))
      if (this.status.message !== '') {
        Text(this.status.message)
          .fontSize(15)
          .fontColor(this.status.error ? '#a1261b' : '#222222')
          .backgroundColor(this.status.error ? '#fdecea' : '#ebffffff')
          .borderRadius(8)
          .padding({ left: 14, right: 14, top: 8, bottom: 8 })
          .margin({ bottom: 48 })
          .constraintSize({ maxWidth: '90%' })
          .hitTestBehavior(HitTestMode.None)
      }
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#f2efe9')
  }
}
