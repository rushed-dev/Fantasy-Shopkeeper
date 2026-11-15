param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'FantasyShopkeeper.uproject'
$buildTool = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$runId = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$reportPath = Join-Path $projectRoot "Saved/Automation/Interaction-$runId"
$logPath = Join-Path $projectRoot "Saved/Logs/Interaction-$runId.log"

if (-not $SkipBuild) {
    & $buildTool FantasyShopkeeperEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) { throw "Editor build failed with exit code $LASTEXITCODE" }
}

& $editor $projectFile '-ExecCmds=Automation RunTests FantasyShopkeeper.' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$reportPath" -unattended -nullrhi -nosound -nosplash -NoSourceControl "-abslog=$logPath"
if ($LASTEXITCODE -ne 0) { throw "Unreal test process failed with exit code $LASTEXITCODE. See $logPath" }

# Unreal may exit zero even when individual automation tests fail.
$report = Get-Content -LiteralPath (Join-Path $reportPath 'index.json') -Raw | ConvertFrom-Json
$passed = $report.succeeded + $report.succeededWithWarnings
if ($report.failed -gt 0 -or $report.notRun -gt 0 -or $passed -ne 19) {
    throw "Interaction tests incomplete or failed. See $reportPath and $logPath"
}
Write-Host "All nineteen interaction, brewing, customer, supply, till and sale tests passed. Report at $reportPath"
if ($report.succeededWithWarnings -gt 0) {
    Write-Warning "$($report.succeededWithWarnings) passing test(s) logged warnings. Review the report."
}
