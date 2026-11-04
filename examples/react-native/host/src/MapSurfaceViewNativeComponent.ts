import {
  type CodegenTypes,
  codegenNativeComponent,
  type HostComponent,
  type ViewProps,
} from "react-native";

export type SurfaceEvent = Readonly<{ surface: CodegenTypes.Int32 }>;

export interface NativeProps extends ViewProps {
  onSurfaceAvailable?: CodegenTypes.DirectEventHandler<SurfaceEvent>;
  onSurfaceLost?: CodegenTypes.DirectEventHandler<SurfaceEvent>;
}

export default codegenNativeComponent<NativeProps>(
  "MapSurfaceView",
) as HostComponent<NativeProps>;
