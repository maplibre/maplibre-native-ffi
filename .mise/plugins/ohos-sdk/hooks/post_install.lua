local cmd = require("cmd")
local file = require("file")

-- The archive holds one directory per host, each a set of component archives, and
-- mise leaves whatever prefix the publisher used above them in place. Reduce that
-- to the components for this host, then unpack the components below, so the
-- install directory looks the same whichever archive produced it.
--
-- `ets` carries the ArkTS compiler and declarations, which is what lets an
-- application module be built here. DevEco Studio drives that through hvigor,
-- which is not published anywhere public, but hvigor is an orchestrator: the
-- tools it calls — the ArkTS compiler, `app_packing_tool.jar`,
-- `hap-sign-tool.jar`, and the debug signing material — all ship in this SDK.

local function quote(path)
  return "'" .. path:gsub("'", "'\\''") .. "'"
end

local function run(command)
  return cmd.exec("set -e; " .. command)
end

local function lines(text)
  local result = {}
  for line in tostring(text):gmatch("[^\r\n]+") do
    table.insert(result, line)
  end
  return result
end

-- The shallowest match, which is the host directory itself rather than anything
-- of the same name nested inside a component.
local function host_directory(root)
  local name = RUNTIME.osType == "darwin" and "darwin" or "linux"
  local found = lines(
    run("find " .. quote(root) .. " -maxdepth 4 -type d -name " .. quote(name))
  )
  local shallowest = nil
  for _, candidate in ipairs(found) do
    if shallowest == nil or #candidate < #shallowest then
      shallowest = candidate
    end
  end
  if shallowest == nil then
    error("no " .. name .. " directory in the OpenHarmony SDK archive")
  end
  return shallowest
end

function PLUGIN:PostInstall(ctx)
  local root = ctx.rootPath
  local host = host_directory(root)
  local staging = root .. "/.host"

  run(
    "mv "
      .. quote(host)
      .. " "
      .. quote(staging)
      .. "; find "
      .. quote(root)
      .. " -mindepth 1 -maxdepth 1 ! -name .host -exec rm -rf {} +"
      .. "; mv "
      .. quote(staging)
      .. "/* "
      .. quote(root)
      .. "/; rmdir "
      .. quote(staging)
  )

  local archives = {}
  for _, component in ipairs({ "native", "toolchains", "ets" }) do
    local matches = lines(
      run(
        "find "
          .. quote(root)
          .. " -maxdepth 1 -name "
          .. quote(component .. "-*.zip")
      )
    )
    if #matches ~= 1 then
      error("expected one " .. component .. " component archive, found " .. #matches)
    end
    table.insert(archives, matches[1])
  end

  -- lib/extract_zip.py explains why the built-in extractor is not used here.
  for _, archive in ipairs(archives) do
    run(
      "python3 "
        .. quote(RUNTIME.pluginDirPath .. "/lib/extract_zip.py")
        .. " "
        .. quote(archive)
        .. " "
        .. quote(root)
    )
  end
  run("rm -f " .. quote(root) .. "/*.zip")

  local toolchain = root .. "/native/build/cmake/ohos.toolchain.cmake"
  if not file.exists(toolchain) then
    error("the installed SDK has no CMake toolchain file at " .. toolchain)
  end
  local hdc = root .. "/toolchains/hdc"
  if not file.exists(hdc) then
    error("the installed SDK has no device connector at " .. hdc)
  end
  -- What an application module is packed and signed with, so a missing
  -- component is reported at install rather than at the first build.
  for _, tool in ipairs({ "app_packing_tool.jar", "hap-sign-tool.jar" }) do
    local path = root .. "/toolchains/lib/" .. tool
    if not file.exists(path) then
      error("the installed SDK has no " .. tool .. " at " .. path)
    end
  end
end
