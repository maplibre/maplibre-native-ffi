import { type TurboModule, TurboModuleRegistry } from "react-native";

// The module has no methods. Loading it installs the binding's JSI bindings
// and the surface registry on the JavaScript thread.
export interface Spec extends TurboModule {}

export default TurboModuleRegistry.getEnforcing<Spec>("MaplibreHost");
