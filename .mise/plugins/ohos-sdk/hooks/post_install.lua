local cmd = require("cmd")
local file = require("file")

-- The archive holds one directory per host, each a set of component archives, and
-- mise leaves whatever prefix the publisher used above them in place. Reduce that
-- to the components for this host, then unpack the native, device-tool, and ArkTS
-- components, so the install directory looks the same whichever archive produced
-- it. The ArkTS component carries `es2abc`, which compiles an application's
-- modules into the bytecode that a HAP holds.

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
  -- The ArkTS component names its compiler directory for the host it targets.
  local ark = RUNTIME.osType == "darwin" and "build-mac" or "build"
  local es2abc = root .. "/ets/build-tools/ets-loader/bin/ark/" .. ark .. "/bin/es2abc"
  if not file.exists(es2abc) then
    error("the installed SDK has no ArkTS bytecode compiler at " .. es2abc)
  end
end
