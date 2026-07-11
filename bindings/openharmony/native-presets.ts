import type { HvigorNode, HvigorTask } from "@ohos/hvigor";
import { execFileSync } from "node:child_process";
import { copyFileSync, cpSync, mkdirSync, rmSync } from "node:fs";
import path from "node:path";

function nativeTask(backend: "egl" | "vulkan"): HvigorTask {
  const preset = `ohos-arm64-${backend}`;
  return {
    name: `buildNative${backend[0].toUpperCase()}${backend.slice(1)}`,
    run: () => {
      const sdk = process.env.OHOS_SDK_NATIVE!;
      execFileSync("cmake", ["--workflow", "--preset", preset], {
        cwd: path.resolve(process.cwd(), "../.."),
        env: process.env,
        stdio: "inherit",
      });
      const packageRoot = path.resolve(process.cwd(), "library");
      const installRoot = path.resolve(
        process.cwd(),
        "../..",
        "build",
        preset,
        "install",
      );
      const libs = path.join(packageRoot, "libs", "arm64-v8a");
      mkdirSync(libs, { recursive: true });
      copyFileSync(
        path.join(installRoot, "lib", "libmaplibre-native-c.so"),
        path.join(libs, "libmaplibre-native-c.so"),
      );
      copyFileSync(
        path.join(sdk, "llvm", "lib", "aarch64-linux-ohos", "libc++_shared.so"),
        path.join(libs, "libc++_shared.so"),
      );
      const include = path.join(packageRoot, "include");
      rmSync(include, { recursive: true, force: true });
      cpSync(path.join(installRoot, "include"), include, { recursive: true });
    },
  };
}

export function registerNativePresets(node: HvigorNode) {
  node.registerTask(nativeTask("egl"));
  node.registerTask(nativeTask("vulkan"));
}
