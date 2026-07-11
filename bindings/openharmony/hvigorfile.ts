import { registerNativePresets } from "./native-presets";
import { hvigor } from "@ohos/hvigor";
import { appTasks } from "@ohos/hvigor-ohos-plugin";

registerNativePresets(hvigor.getRootNode());

export default { system: appTasks, plugins: [] };
