# HACK:
# This script runs a command with a hard timeout on Windows and kills the launched
# process tree when the deadline expires. It also finds and kills project-related
# processes outside the process tree. It's necessary as an aid for AI agents until
# I figure out this orphaned / wedged process issue on Windows.

param(
    [int]$TimeoutSeconds = 30,
    [Parameter(Mandatory = $true)]
    [string]$CommandLine
)

$pwsh = @((Get-Command -CommandType Application -Name pwsh -ErrorAction Stop))[0].Source
$arguments = @("-NoProfile", "-Command", $CommandLine)

$stdout = New-TemporaryFile
$stderr = New-TemporaryFile
$workspace = (Resolve-Path -LiteralPath ".").Path
$knownProcesses = @{}
foreach ($process in Get-CimInstance Win32_Process -ErrorAction SilentlyContinue) {
    $knownProcesses[[int]$process.ProcessId] = $true
}

$projectProcessNames = @(
    "bash.exe",
    "cmake.exe",
    "mise.exe",
    "ninja.exe",
    "test.exe",
    "zig.exe",
    "zig-map.exe",
    "zig-readback.exe"
)

function Get-ProcessTreeIds([int[]]$RootIds) {
    $known = @{}
    foreach ($id in $RootIds) {
        $known[$id] = $true
    }

    $processes = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $changed = $true
    while ($changed) {
        $changed = $false
        foreach ($process in $processes) {
            if ($known.ContainsKey([int]$process.ParentProcessId) -and -not $known.ContainsKey([int]$process.ProcessId)) {
                $known[[int]$process.ProcessId] = $true
                $changed = $true
            }
        }
    }

    return @($known.Keys)
}

function Stop-ProcessTree([int[]]$ProcessIds) {
    foreach ($id in ($ProcessIds | Select-Object -Unique)) {
        $process = Get-Process -Id $id -ErrorAction SilentlyContinue
        if ($null -ne $process) {
            & taskkill.exe /PID $id /T /F *> $null
        }

        $cimProcess = Get-CimInstance Win32_Process -Filter "ProcessId = $id" -ErrorAction SilentlyContinue
        if ($null -ne $cimProcess) {
            Invoke-CimMethod -InputObject $cimProcess -MethodName Terminate *> $null
        }
    }
}

function Test-ProjectProcess($Process) {
    if ($knownProcesses.ContainsKey([int]$Process.ProcessId)) {
        return $false
    }

    $commandLine = [string]$Process.CommandLine
    $path = [string]$Process.ExecutablePath
    return $commandLine.Contains($workspace) -or
        $path.Contains($workspace) -or
        $Process.Name -in $projectProcessNames
}

function Get-StartedProjectProcessIds {
    return @(
        Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
            Where-Object { Test-ProjectProcess $_ } |
            ForEach-Object { [int]$_.ProcessId }
    )
}

function Write-FileIfNotEmpty([string]$Path, [switch]$AsError) {
    $file = Get-Item -LiteralPath $Path -ErrorAction SilentlyContinue
    if ($null -eq $file -or $file.Length -eq 0) {
        return
    }

    $content = Get-Content -LiteralPath $Path -Raw -ErrorAction SilentlyContinue
    if ($AsError) {
        Write-Error $content
    } else {
        Write-Output $content
    }
}

$process = Start-Process -FilePath $pwsh -ArgumentList $arguments -RedirectStandardOutput $stdout.FullName -RedirectStandardError $stderr.FullName -PassThru -ErrorAction Stop
$deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
$seen = @{}
$exitCode = $null

while ($true) {
    $treeIds = @(Get-ProcessTreeIds @($process.Id))
    foreach ($id in $treeIds) {
        $seen[$id] = $true
    }

    $liveIds = @($seen.Keys | Where-Object { $null -ne (Get-Process -Id $_ -ErrorAction SilentlyContinue) })
    if ($process.HasExited) {
        $exitCode = $process.ExitCode
        if ($liveIds.Count -eq 0) {
            break
        }
    }

    if ([DateTime]::UtcNow -ge $deadline) {
        $allIds = @($seen.Keys) + @($process.Id) + @(Get-StartedProjectProcessIds)
        Stop-ProcessTree $allIds
        Start-Sleep -Milliseconds 250
        Stop-ProcessTree @(Get-StartedProjectProcessIds)
        Write-FileIfNotEmpty $stdout.FullName
        Write-FileIfNotEmpty $stderr.FullName -AsError
        Write-Error "command timed out after $TimeoutSeconds seconds; killed process tree rooted at PID $($process.Id)"
        Remove-Item -LiteralPath $stdout.FullName, $stderr.FullName -Force -ErrorAction SilentlyContinue
        exit 124
    }

    Start-Sleep -Milliseconds 100
}

Write-FileIfNotEmpty $stdout.FullName
Write-FileIfNotEmpty $stderr.FullName -AsError
Remove-Item -LiteralPath $stdout.FullName, $stderr.FullName -Force -ErrorAction SilentlyContinue

if ($null -eq $exitCode) {
    exit 0
}
exit $exitCode
