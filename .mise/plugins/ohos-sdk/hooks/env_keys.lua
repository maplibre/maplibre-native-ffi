function PLUGIN:EnvKeys(ctx)
  -- The names the OpenHarmony toolchain and the wider ecosystem use: the CMake
  -- toolchain file and the Rust cross build read OHOS_SDK_NATIVE, and OHOS_NDK_HOME
  -- is what tools that expect the whole SDK look for.
  return {
    { key = "OHOS_SDK_NATIVE", value = ctx.path .. "/native" },
    { key = "OHOS_NDK_HOME", value = ctx.path },
  }
end
