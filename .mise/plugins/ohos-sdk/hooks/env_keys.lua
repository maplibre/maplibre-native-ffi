function PLUGIN:EnvKeys(ctx)
  -- The names the OpenHarmony toolchain and the wider ecosystem use: native builds
  -- read OHOS_SDK_NATIVE, whole-SDK tools read OHOS_NDK_HOME, and device tasks find
  -- hdc on PATH.
  return {
    { key = "OHOS_SDK_NATIVE", value = ctx.path .. "/native" },
    { key = "OHOS_NDK_HOME", value = ctx.path },
    { key = "PATH", value = ctx.path .. "/toolchains" },
  }
end
