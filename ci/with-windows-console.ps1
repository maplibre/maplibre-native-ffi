param(
  [Parameter(Mandatory, ValueFromRemainingArguments)]
  [string[]] $Command
)

$ErrorActionPreference = "Stop"
$tools = Join-Path $env:RUNNER_TEMP "windows-console-pstools"
$psexec = Join-Path $tools "PsExec64.exe"
if (-not (Test-Path $psexec)) {
  $archive = Join-Path $env:RUNNER_TEMP "windows-console-pstools.zip"
  Invoke-WebRequest "https://download.sysinternals.com/files/PSTools.zip" -OutFile $archive
  $expected = "C26C6BA44997E650A733DF257C8FEB83898D3FAEF2D2E391869D107E40863553"
  if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $expected) {
    throw "PSTools archive hash mismatch"
  }
  Expand-Archive $archive -DestinationPath $tools -Force
}

Add-Type @'
using System.Runtime.InteropServices;
public static class ConsoleSession {
    [DllImport("kernel32.dll")]
    public static extern uint WTSGetActiveConsoleSessionId();
}
'@
$session = [ConsoleSession]::WTSGetActiveConsoleSessionId()
if ($session -eq [uint32]::MaxValue) {
  throw "No console session is attached"
}
Write-Output "Run in console session $session`: $($Command -join ' ')"

# The console process uses SYSTEM's token. Restore the runner's environment so
# mise, compiler tools, package caches, and job inputs keep their original paths.
$environment = @{}
Get-ChildItem Env: | ForEach-Object { $environment[$_.Name] = $_.Value }
if (-not $environment.ContainsKey("GRADLE_USER_HOME")) {
  $environment["GRADLE_USER_HOME"] = Join-Path $env:USERPROFILE ".gradle"
}
# A daemon from the runner's service session cannot launch windowed apps here.
$environment["GRADLE_OPTS"] = "$($environment['GRADLE_OPTS']) -Dorg.gradle.daemon=false"
$payload = Join-Path $env:RUNNER_TEMP "console-$([Guid]::NewGuid()).json"
$launcher = Join-Path $env:RUNNER_TEMP "console-$([Guid]::NewGuid()).ps1"
$completion = Join-Path $env:RUNNER_TEMP "console-$([Guid]::NewGuid()).exit"
$logDirectory = Join-Path $env:GITHUB_WORKSPACE "build/windows-console-logs"
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$log = Join-Path $logDirectory "console-$([Guid]::NewGuid()).log"
@{
  environment = $environment
  directory = (Get-Location).Path
  command = $Command
  output = $log
  completion = $completion
} | ConvertTo-Json -Depth 4 | Set-Content $payload
@'
param([string] $Payload)
$ErrorActionPreference = "Stop"
$data = Get-Content -Raw $Payload | ConvertFrom-Json -AsHashtable
foreach ($entry in $data.environment.GetEnumerator()) {
  [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, "Process")
}
Set-Location $data.directory
"Console command process session: $((Get-Process -Id $PID).SessionId)" |
  Set-Content $data.output
$result = 1
try {
  $executable, $arguments = $data.command
  & $executable @arguments *>> $data.output
  $result = $LASTEXITCODE
} catch {
  $_ | Out-String | Add-Content $data.output
} finally {
  # Publish only after closing the file, so the parent reads a complete status.
  $temporary = "$($data.completion).tmp"
  $result | Set-Content $temporary
  Move-Item $temporary $data.completion
}
exit $result
'@ | Set-Content $launcher

try {
  $powershell = (Get-Process -Id $PID).Path
  # Detached execution avoids waiting for handles inherited by build daemons.
  & $psexec -accepteula -nobanner -d -s -i $session -w (Get-Location).Path `
    $powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $launcher $payload
  # In detached mode PsExec returns the child's process ID, not an exit status.
  $consoleProcessId = $LASTEXITCODE
  Write-Output "Console process ID: $consoleProcessId"
  while (-not (Test-Path $completion)) {
    $process = Get-Process -Id $consoleProcessId -ErrorAction SilentlyContinue
    if (($null -eq $process -or $process.SessionId -ne $session) -and
        -not (Test-Path $completion)) {
      throw "Console process $consoleProcessId exited without a completion status"
    }
    Start-Sleep -Milliseconds 250
  }
  $result = [int](Get-Content -Raw $completion)
} finally {
  if (Test-Path $log) { Get-Content $log }
  Remove-Item $payload, $launcher -Force
  Remove-Item $completion -Force -ErrorAction SilentlyContinue
}
exit $result
