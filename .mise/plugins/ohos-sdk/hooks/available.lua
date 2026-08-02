local http = require("http")

local INDEX = "https://repo.huaweicloud.com/openharmony/os/"

local function components(version)
  local parts = {}
  for part in version:gmatch("%d+") do
    table.insert(parts, tonumber(part))
  end
  return parts
end

-- Newest first, which is the order mise presents and resolves `latest` from.
local function newer(left, right)
  local first, second = components(left), components(right)
  for index = 1, math.max(#first, #second) do
    local one, other = first[index] or 0, second[index] or 0
    if one ~= other then
      return one > other
    end
  end
  return false
end

function PLUGIN:Available(ctx)
  local response, err = http.get({ url = INDEX })
  if err ~= nil then
    error("could not list OpenHarmony releases: " .. err)
  end
  if response.status_code ~= 200 then
    error("could not list OpenHarmony releases: HTTP " .. response.status_code)
  end

  -- The release index is a directory listing, so each SDK version appears as a
  -- link to its own directory.
  local versions = {}
  local seen = {}
  for version in response.body:gmatch('href="([%d%.]+)%-Release/"') do
    if not seen[version] then
      seen[version] = true
      table.insert(versions, version)
    end
  end
  if #versions == 0 then
    error("no OpenHarmony releases found at " .. INDEX)
  end

  table.sort(versions, newer)

  local result = {}
  for _, version in ipairs(versions) do
    table.insert(result, { version = version })
  end
  return result
end
