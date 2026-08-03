local cmd = require("cmd")
local http = require("http")
local strings = require("strings")

local RELEASES = "https://repo.huaweicloud.com/openharmony/os/"

-- One archive serves Windows and Linux; macOS gets one per architecture.
local function archive_name()
  if RUNTIME.osType == "darwin" then
    if RUNTIME.archType == "arm64" then
      return "L2-SDK-MAC-M1-PUBLIC.tar.gz"
    end
    return "ohos-sdk-mac-public.tar.gz"
  end
  -- The Linux toolchain is published for x86_64 alone.
  if RUNTIME.archType ~= "amd64" and RUNTIME.archType ~= "x86_64" then
    error("the OpenHarmony SDK is published for x86_64 Linux, not " .. RUNTIME.archType)
  end
  return "ohos-sdk-windows_linux-public.tar.gz"
end

function PLUGIN:PreInstall(ctx)
  -- The post-install step prunes and unpacks with POSIX tools. Windows hosts are
  -- not otherwise served differently, so lifting this means porting that step.
  if RUNTIME.osType == "windows" then
    error("the OpenHarmony SDK plugin supports Linux and macOS hosts")
  end

  -- The post-install step unpacks a component with Python. Reporting that here
  -- rather than there keeps a missing interpreter from costing a multi-gigabyte
  -- download first.
  if
    strings.trim_space(cmd.exec("command -v python3 >/dev/null 2>&1 && echo yes || echo no"))
    ~= "yes"
  then
    error("python3 is required to unpack the OpenHarmony SDK; install it and retry")
  end

  local url = RELEASES .. ctx.version .. "-Release/" .. archive_name()

  -- The publisher's digest, so mise rejects a truncated or corrupted download. It
  -- is not a pin: the lockfile records the archive URL for a plugin-provided tool
  -- but no digest of its own, so the archive is trusted the way its host is.
  local response, err = http.get({ url = url .. ".sha256" })
  if err ~= nil then
    error("could not fetch the SDK checksum: " .. err)
  end
  if response.status_code ~= 200 then
    error(
      "no OpenHarmony SDK for version "
        .. ctx.version
        .. ": HTTP "
        .. response.status_code
        .. " for "
        .. url
        .. ".sha256"
    )
  end

  return {
    version = ctx.version,
    url = url,
    sha256 = strings.trim_space(response.body),
  }
end
