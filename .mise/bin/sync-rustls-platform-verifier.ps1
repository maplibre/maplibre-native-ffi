$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Invoke-Git {
  param(
    [Parameter(Mandatory = $true)]
    [string[]] $Arguments
  )

  & git @Arguments
  if ($LASTEXITCODE -ne 0) {
    throw "git exited with code $LASTEXITCODE"
  }
}

$repoRoot = (Invoke-Git -Arguments @(
    "-C",
    $PSScriptRoot,
    "rev-parse",
    "--show-toplevel"
  )).Trim()
$dependencyParent = Join-Path $repoRoot "build/dependencies"
$dependencyDir = Join-Path $dependencyParent "rustls-platform-verifier"
$upstreamUrl = "https://github.com/rustls/rustls-platform-verifier.git"
$upstreamTag = "v/0.6.2"
$upstreamCommit = "1099f161bfc5e3ac7f90aad88b1bf788e72906cb"
$patches = @(
  (Join-Path $repoRoot "patches/rustls-platform-verifier/0001-namespace-android-verifier.patch"),
  (Join-Path $repoRoot "patches/rustls-platform-verifier/0002-use-android-revocation-policy.patch"),
  (Join-Path $repoRoot "patches/rustls-platform-verifier/0003-add-modification-notices.patch")
)

New-Item -ItemType Directory -Force -Path $dependencyParent | Out-Null
$stagingDir = Join-Path $dependencyParent (
  ".rustls-platform-verifier.{0}" -f [guid]::NewGuid().ToString("N")
)
New-Item -ItemType Directory -Path $stagingDir | Out-Null

try {
  Invoke-Git -Arguments @("-C", $stagingDir, "init", "--quiet")
  Invoke-Git -Arguments @(
    "-C",
    $stagingDir,
    "remote",
    "add",
    "origin",
    $upstreamUrl
  )
  Invoke-Git -Arguments @(
    "-C",
    $stagingDir,
    "fetch",
    "--quiet",
    "--depth",
    "1",
    "origin",
    "refs/tags/$upstreamTag"
  )
  Invoke-Git -Arguments @(
    "-C",
    $stagingDir,
    "checkout",
    "--quiet",
    "--detach",
    $upstreamCommit
  )

  $actualCommit = (
    Invoke-Git -Arguments @("-C", $stagingDir, "rev-parse", "HEAD")
  ).Trim()
  if ($actualCommit -ne $upstreamCommit) {
    throw "$upstreamTag resolved to $actualCommit, expected $upstreamCommit"
  }

  foreach ($patch in $patches) {
    Invoke-Git -Arguments @("-C", $stagingDir, "apply", "--check", $patch)
    Invoke-Git -Arguments @("-C", $stagingDir, "apply", $patch)
  }

  if (Test-Path -LiteralPath $dependencyDir) {
    $previousDir = Join-Path $dependencyParent (
      ".rustls-platform-verifier.previous.{0}" -f
        [guid]::NewGuid().ToString("N")
    )
    Move-Item -LiteralPath $dependencyDir -Destination $previousDir
    try {
      Move-Item -LiteralPath $stagingDir -Destination $dependencyDir
      $stagingDir = $null
    } catch {
      if (-not (Test-Path -LiteralPath $dependencyDir)) {
        Move-Item -LiteralPath $previousDir -Destination $dependencyDir
      }
      throw
    }
    Remove-Item -LiteralPath $previousDir -Recurse -Force
  } else {
    Move-Item -LiteralPath $stagingDir -Destination $dependencyDir
    $stagingDir = $null
  }
} finally {
  if ($null -ne $stagingDir -and (Test-Path -LiteralPath $stagingDir)) {
    Remove-Item -LiteralPath $stagingDir -Recurse -Force
  }
}
