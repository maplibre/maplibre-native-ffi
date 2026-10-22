param(
  [Parameter(Mandatory, ParameterSetName = "Runner", ValueFromRemainingArguments)]
  [string[]] $Command,
  [Parameter(Mandatory, ParameterSetName = "Desktop")]
  [string] $RequestDirectory
)

# Blacksmith runs jobs in Windows service session 0. Mesa's Vulkan presenter
# needs a visible desktop, so run the command in the VM's logged-in session.
$ErrorActionPreference = "Stop"

# PsExec starts this same script in the desktop session with a saved request.
if ($PSCmdlet.ParameterSetName -eq "Desktop") {
  $logPath = Join-Path $RequestDirectory "output.log"
  $exitCodePath = Join-Path $RequestDirectory "exit-code"
  $exitCode = 1
  try {
    $request = Get-Content -Raw (Join-Path $RequestDirectory "request.json") |
      ConvertFrom-Json -AsHashtable
    foreach ($entry in $request.environment.GetEnumerator()) {
      [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, "Process")
    }
    Set-Location $request.directory
    $executable, $arguments = $request.command
    & $executable @arguments *>> $logPath
    $exitCode = $LASTEXITCODE
  } catch {
    $_ | Out-String | Add-Content $logPath
  } finally {
    # Rename after closing the file so the runner reads a complete exit code.
    $exitCode | Set-Content "$exitCodePath.tmp"
    Move-Item "$exitCodePath.tmp" $exitCodePath
  }
  exit $exitCode
}

$toolsDirectory = Join-Path $env:RUNNER_TEMP "windows-console-pstools"
$psexec = Join-Path $toolsDirectory "PsExec64.exe"
if (-not (Test-Path $psexec)) {
  $archive = Join-Path $env:RUNNER_TEMP "windows-console-pstools.zip"
  Invoke-WebRequest "https://download.sysinternals.com/files/PSTools.zip" -OutFile $archive
  $expectedHash = "C26C6BA44997E650A733DF257C8FEB83898D3FAEF2D2E391869D107E40863553"
  if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $expectedHash) {
    throw "PSTools archive hash mismatch"
  }
  Expand-Archive $archive -DestinationPath $toolsDirectory -Force
}

Add-Type @'
using System.Runtime.InteropServices;
public static class ConsoleSession {
    [DllImport("kernel32.dll")]
    public static extern uint WTSGetActiveConsoleSessionId();
}
'@
$sessionId = [ConsoleSession]::WTSGetActiveConsoleSessionId()
if ($sessionId -eq [uint32]::MaxValue) {
  throw "No console session is attached"
}

$RequestDirectory = Join-Path $env:RUNNER_TEMP "windows-console-$([Guid]::NewGuid())"
New-Item -ItemType Directory -Path $RequestDirectory | Out-Null
$logPath = Join-Path $RequestDirectory "output.log"
$exitCodePath = Join-Path $RequestDirectory "exit-code"

# PsExec switches user accounts; preserve the runner's tool paths and job inputs.
$environment = @{}
Get-ChildItem Env: | ForEach-Object { $environment[$_.Name] = $_.Value }
try {
  @{
    environment = $environment
    directory = (Get-Location).Path
    command = $Command
  } | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $RequestDirectory "request.json")

  $powershell = (Get-Process -Id $PID).Path
  Write-Output "Run in desktop session $sessionId`: $($Command -join ' ')"
  # Detach so build daemons cannot hold PsExec open. It returns the child's PID.
  & $psexec -accepteula -nobanner -d -s -i $sessionId -w (Get-Location).Path `
    $powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File $PSCommandPath -RequestDirectory $RequestDirectory
  $desktopProcessId = $LASTEXITCODE

  while (-not (Test-Path $exitCodePath)) {
    $process = Get-Process -Id $desktopProcessId -ErrorAction SilentlyContinue
    if (($null -eq $process -or $process.SessionId -ne $sessionId) -and
        -not (Test-Path $exitCodePath)) {
      throw "Desktop process $desktopProcessId exited without reporting its exit code"
    }
    Start-Sleep -Milliseconds 250
  }
  $exitCode = [int](Get-Content -Raw $exitCodePath)
} finally {
  if (Test-Path $logPath) { Get-Content $logPath }
  Remove-Item $RequestDirectory -Recurse -Force
}
exit $exitCode
