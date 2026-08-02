local cmd = require("cmd")
local file = require("file")

-- The archive holds one directory per host, each a set of component archives, and
-- mise leaves whatever prefix the publisher used above them in place. Reduce that
-- to the components for this host, then unpack the one component a native build
-- needs, so the install directory looks the same whichever archive produced it.

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

  local archives = lines(
    run("find " .. quote(root) .. " -maxdepth 1 -name 'native-*.zip'")
  )
  if #archives ~= 1 then
    error("expected one native component archive, found " .. #archives)
  end

  -- lib/extract_zip.py explains why the built-in extractor is not used here.
  run(
    "python3 "
      .. quote(RUNTIME.pluginDirPath .. "/lib/extract_zip.py")
      .. " "
      .. quote(archives[1])
      .. " "
      .. quote(root)
      .. "; rm -f "
      .. quote(root)
      .. "/*.zip"
  )

  local toolchain = root .. "/native/build/cmake/ohos.toolchain.cmake"
  if not file.exists(toolchain) then
    error("the installed SDK has no CMake toolchain file at " .. toolchain)
  end
end
